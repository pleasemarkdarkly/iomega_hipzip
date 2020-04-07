#include "cd9660_bio.h"
#include "cd9660_node.h"
#include "cd9660_internal.h"
#include <stdlib.h>
#include <cyg/io/io.h>
#include <util/debug/debug.h>

DEBUG_MODULE(ISOFS);
DEBUG_USE_MODULE(ISOFS);


#define BLSIZE 2048

#define USE_METHOD_ONE
//#define USE_METHOD_TWO

#ifdef USE_METHOD_TWO
//
// block caching method two: two-dimensional sector cache
//
//  the structure is as follows: the cached_block_t represents a single sector from the data cd
//  the cached entries are stored in a table that has _WIDTH * _DEPTH total entries. _WIDTH must
//  be a power of two, such that _WIDTH - 1 generates a mask. when a lookup occurs, the sector
//  number of the lookup is masked with _WIDTH - 1 to generate an index to the width of the table,
//  and the depth of the table is traversed manually

#define BLOCK_CACHE_WIDTH  16
#define BLOCK_CACHE_DEPTH   2
#define READAHEAD_DIST      4

//#define ISOFS_READ_AHEAD

#define SECNO_TO_INDEX(x) ((x) & (BLOCK_CACHE_WIDTH-1))
typedef struct cached_block_s 
{
    cyg_uint32 _blockno;
    char       _data[BLSIZE];
    cyg_uint8  _used;
} cached_block_t;

typedef struct block_cache_s
{
    cached_block_t cache[BLOCK_CACHE_WIDTH][BLOCK_CACHE_DEPTH];
#ifdef ISOFS_READ_AHEAD
    char       _readahead_block[BLSIZE * READAHEAD_DIST];
#endif
    cyg_mutex_t cache_lock;
} block_cache_t;

static block_cache_t block_cache;
static cyg_uint8 initialized = 0;

// initialize: make sure the cache is available to be used
static void
initialize(void)
{
    if( !initialized )
    {
        initialized = 1;
        memset( (void*)&block_cache, 0, sizeof( block_cache_t ) );
        cyg_mutex_init( &(block_cache.cache_lock) );
    }
}
static inline void lock()   { cyg_mutex_lock(&block_cache.cache_lock);   }
static inline void unlock() { cyg_mutex_unlock(&block_cache.cache_lock); }

// bread_cached: attempt to read a cached entry. if not found, issue a hw read and cache
//               the found block
static int
bread_cached( cyg_io_handle_t ioh, void* data, int bytes, int blkno ) 
{
    int i, error = ENOERR;
    int sectors = bytes / BLSIZE;
    int bytes_left = bytes;
    
    lock();
    DEBUGP( ISOFS, DBGLEV_CHATTER, " bread_cached enter, ioh = 0x%08x, data = %p, bytes = %d, blkno = %d\n",
        ioh, data, bytes, blkno );
    // loop on the sectors we need to read
    for( i = 0; i < sectors && bytes_left > 0; i++ ) {
        cached_block_t* row;
        int z, ideal = 0;
        
        row = &(block_cache.cache[ SECNO_TO_INDEX( blkno+i ) ][0]);
        for( z = 0; z < BLOCK_CACHE_DEPTH && row[z]._used; z++ ) {
            if( row[z]._blockno == (blkno+i) ) {
                // cache hit
                DEBUGP(ISOFS, DBGLEV_CHATTER, "isofs cache hit block %d\n", row[z]._blockno);
                break;
            } else if( row[z]._blockno < (blkno+i) ) {
                // not found yet, track an ideal location to perform our insertion
                ideal = z;
            }
        }
        // on a cache hit, copy from the buffer and continue
        if( row[z]._blockno == (blkno+i) ) {
            memcpy( data + (i*BLSIZE), &( row[z]._data[0] ), BLSIZE );
            bytes_left -= BLSIZE;
        } else if( z == BLOCK_CACHE_DEPTH || !row[z]._used ) {
            // on a miss, we need to read. at this point, assume no future blocks are cached,
            //  and issue a read for the entire chunk
            // TODO when issuing the read, populate all read sectors into the cache
            DEBUGP( ISOFS, DBGLEV_CHATTER, "isofs cache miss block %d\n", (blkno+i) );
            // cache miss, perform read
            if( z == BLOCK_CACHE_DEPTH ) {
                // the cache is full, use our previously discovered ideal insertion point
                z = ideal;
            }
            // issue a read for the entire chunk
            error = cyg_io_bread( ioh, data + (i*BLSIZE), &bytes_left, blkno + i );
            
            if( error == ENOERR ) {
                // feed the first block back into the cache
                memcpy( &( row[z]._data[0] ), data + (i*BLSIZE), BLSIZE );
                row[z]._used = 1;
                row[z]._blockno = (blkno+i);
                
                bytes_left = 0;
            }
            else {
                DEBUG(ISOFS, DBGLEV_ERROR, "isofs failed to read err %d\n", error);
            }
        }
    }
    unlock();
    
    return error;
}

// invalidate the block cache
void
binvalidate(void)
{
    int i,z;
    initialize();
    lock();
    for( i = 0; i < BLOCK_CACHE_WIDTH; i++ ) {
        for( z = 0; z < BLOCK_CACHE_DEPTH; z++ ) {
            block_cache.cache[i][z]._used = 0;
        }
    }
    unlock();
}
#endif

#ifdef USE_METHOD_ONE
// block caching method one: single expanding buffer
static cyg_uint32 _mru_blkno = 0;
static cyg_uint32 _mru_bcount = 0;
static char* _mru_data = 0;

#define BL_BLKSTOBYTES(x) ((x)*BLSIZE)
#define BL_BYTESTOBLKS(x) (((x)+(BLSIZE-1))/BLSIZE)

void binvalidate(void) 
{
    if( _mru_data ) {
        free( _mru_data );
        _mru_data = 0;
    }
}
 
static int
bread_cached(cyg_io_handle_t ioh, void* data, int count, int blkno )
{
    // determine if any data needed by the upper level is currently in the cache
    //  if so, copy it out
    //  if not, perform a read
    // note: count is in bytes. obviously.
    
    int bcopied = 0;
    int error = ENOERR;
    DEBUGP( ISOFS, DBGLEV_CHATTER, " bread_cached enter, ioh = 0x%08x, data = %p, count = %d, blkno = %d\n",
        ioh, data, count, blkno );
    DEBUG( ISOFS, DBGLEV_CHATTER, " cache state, _mru_blkno = %d, _mru_bcount = %d, _mru_data = %p\n",
        _mru_blkno, _mru_bcount, _mru_data );
#if 0
    // mid-chunk salvage method (possibly broken)
    if( _mru_data && (_mru_blkno == blkno || (_mru_blkno < blkno && (_mru_blkno+BL_BYTESTOBLKS(_mru_bcount)) > blkno ) )) {
        int startidx = (blkno - _mru_blkno);
        int avail = _mru_bcount - ((blkno - _mru_blkno) * BLSIZE);
        bcopied = (avail > count ? count : avail);
        memcpy( &(data[0]), &(_mru_data[startidx]), bcopied );
        DEBUG( ISOFS, DBGLEV_CHATTER, "  cache hit  %d bytes from idx %d to buffer\n", bcopied, startidx );
    }
#else
    // start of chunk method
    if( _mru_data && (_mru_blkno == blkno) ) {
        bcopied = (_mru_bcount * BLSIZE);
        if( bcopied > count ) bcopied = count;
        memcpy( data, _mru_data, bcopied );
    }
#endif
#define MAX_RETRY 10
    if( bcopied < count) {
        // make sure bleft is a multiple of BLSIZE
        int bleft = ((count-bcopied) + (BLSIZE-1)) & ~(BLSIZE-1);
        if( _mru_bcount < bleft && _mru_data ) {
            free( (void*) _mru_data );
            _mru_data = (char*) malloc( bleft );
            DEBUG( ISOFS, DBGLEV_CHATTER, "  buffer resize from %d to %d\n", _mru_bcount, bleft );
        } else if( !_mru_data ) {
            _mru_data = (char*) malloc( bleft );
            DEBUG( ISOFS, DBGLEV_CHATTER, " buffer alloc %d bytes\n", bleft );
        }
        // this yields a number of bytes divisible by a sector
        _mru_bcount = bleft;
        _mru_blkno = blkno+(bcopied/BLSIZE);
        error = cyg_io_bread(ioh, &(_mru_data[0]), &_mru_bcount, _mru_blkno );
        if( error == ENOERR ) {
            DEBUG( ISOFS, DBGLEV_CHATTER, "  cache miss %d bytes to buffer idx %d from mru_blkno %d\n", count-bcopied, bcopied, _mru_blkno);
            memcpy( &(data[bcopied]), _mru_data, count-bcopied );
            bcopied += _mru_bcount;
        }
		else 
		{
			DEBUG(ISOFS, DBGLEV_ERROR, "isofs failed to read err %d\n", error);
			return error;
		}

    }
    DEBUG( ISOFS, DBGLEV_CHATTER, "  total copied %d\n", bcopied );
    return error;
}
#endif

int
bread(cyg_file * vp,
    cyg_int32 blkno,
    int size,
    struct buf ** bpp)
{
    struct buf *bp;
    struct iso_node * ip;
    int error;

    bp = *bpp = (struct buf *)malloc(sizeof(struct buf));
    if (bp == 0) {
        DEBUG( ISOFS, DBGLEV_ERROR, "%s %d Error on malloc\n", __PRETTY_FUNCTION__, __LINE__);
        return -ENOMEM;
    }
    bp->b_data = (char *)malloc(size);
    if (bp->b_data == 0) {
        DEBUG( ISOFS, DBGLEV_ERROR, "%s %d Error on malloc\n", __PRETTY_FUNCTION__, __LINE__);
        return -ENOMEM;
    }
    
    bp->b_lblkno = bp->b_blkno = blkno;
    bp->b_bcount = size;
    bp->b_vp = vp;

    if (vp->f_type != CYG_FILE_TYPE_BLK) {
        /* This is cd9660_strategy */
        ip = VTOI(vp);
        if (bp->b_blkno == bp->b_lblkno) {
            error = cd9660_bmap(vp, bp->b_lblkno, NULL, &bp->b_blkno, NULL);
            if (error) {
                bp->b_error = error;
                bp->b_flags |= B_ERROR;
                return (error);
            }
            if ((long)bp->b_blkno == -1) {
                clrbuf(bp);
            }
        }
        if ((long)bp->b_blkno == -1) {
            return (0);
        }
        vp = ip->i_devvp;
    }

    if (vp->f_type == CYG_FILE_TYPE_BLK) {
        cyg_uint32 blkno = bp->b_blkno / 4;
        cyg_uint32 bcount = bp->b_bcount;

        return bread_cached((cyg_io_handle_t)vp->f_data, &(bp->b_data[0]), bcount, blkno);
    }
    
    return ENODEV;
}

void
brelse(struct buf *bp)
{
    if (bp) {
        if (bp->b_data) {
            free(bp->b_data);
        }
        free(bp);
    }
}

