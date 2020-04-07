#include <cyg/io/io.h>
#include <stdlib.h>
#include <errno.h>

#include <io/bio/bio.h>
#include <util/debug/debug.h>

DEBUG_MODULE(BIO);
DEBUG_USE_MODULE(BIO);

struct buf *
getblk(cyg_file * vp, cyg_int32 blkno, int size)
{
    DEBUG(BIO, DBGLEV_ERROR, "TODO: implement getblk\n");
    return NULL;
}

int
bread(cyg_file * vp,
      cyg_int32 blkno,
      int size,
      struct buf ** bpp)
{
    struct buf *bp;
    //    struct iso_node * ip;
    int error;

    diag_printf("bread vp %p blk %d size %d\n", vp, blkno, size);
    
    bp = *bpp = (struct buf *)malloc(sizeof(struct buf));
    if (bp == 0) {
        DEBUG(BIO, DBGLEV_ERROR, "Error on malloc\n");
        return -ENOMEM;
    }
    bp->b_data = (char *)malloc(size);
    if (bp->b_data == 0) {
        DEBUG(BIO, DBGLEV_ERROR, "Error on malloc\n");
        return -ENOMEM;
    }

    // init the buf object here
    bp->b_bufsize = size;
    bp->b_lblkno = bp->b_blkno = blkno;
    bp->b_vp = vp;
    bp->b_flags = 0;
    bp->b_error = 0;

    if( vp->f_type != CYG_FILE_TYPE_BLK ) {
        DEBUG(BIO, DBGLEV_ERROR, "bread called on non-block object\n");
    }
    if (vp->f_type == CYG_FILE_TYPE_BLK) {
        cyg_uint32 blk = bp->b_blkno;
        cyg_uint32 bcount = size;
#if 0
        // TODO: dont use dynamic memory
        error = ENOERR;
        if( !((blkno == _mru_blkno) && (bcount == _mru_bcount)) || !_mru_data ) {
            _mru_blkno = blkno;
            _mru_bcount = bcount;
            if( _mru_data ) {
                free( _mru_data );
                _mru_data = 0;
            }
            _mru_data = (char*) malloc( _mru_bcount );
            if( _mru_data == 0 ) {
                return -ENOMEM;
            }
            error = cyg_io_bread((cyg_io_handle_t)vp->f_data, &(_mru_data[0]), &_mru_bcount, _mru_blkno );
            if( error != ENOERR ) {
                free( _mru_data );
                _mru_data = 0;
                _mru_blkno = _mru_bcount = 0;
            }
        }
        if( error == ENOERR ) {
            memcpy( &(bp->b_data[0]), &(_mru_data[0]), _mru_bcount );
        }
        return error;
#else
        {
            struct CYG_IOVEC_TAG iov[2] = { { &(bp->b_data[0]), bcount }, {0,0}};
            struct CYG_UIO_TAG uio = { &(iov[0]), 1, blk, 0, UIO_SYSSPACE, UIO_READ };
            int res = vp->f_ops->fo_read( vp, &uio );

            if( res < 0 ) {
                bp->b_bcount = 0;
                bp->b_resid  = size;
                bp->b_flags |= B_ERROR | B_INVAL;
            } else {
                bp->b_bcount = size;
                bp->b_resid  = 0;
                bp->b_flags |= B_DONE;
            }
            
            return res;
        }
        //        return cyg_io_bread((cyg_io_handle_t)vp->f_data, &(bp->b_data[0]), &bcount, blkno);
#endif
    }
    
    return ENODEV;
}

int
bwrite(struct buf* bp)
{
    diag_printf("bwrite called with %p\n", bp);
    return 0;
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
