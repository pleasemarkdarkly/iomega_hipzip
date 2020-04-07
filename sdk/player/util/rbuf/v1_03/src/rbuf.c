// rbuf.c: ring buffer implementation
// danc@iobjects.com 07/05/01
// (c) Interactive Objects

#include <string.h>              // memcpy, memset
#include <stdlib.h>              // malloc
#include <util/debug/debug.h>    // debug hooks

// order is important on these includes
#include "rbuf_type.h"           // rbuf structures
#include <util/rbuf/rbuf.h>      // rbuf API


//
// Specify a debugging module
//

DEBUG_MODULE(RBUF);
DEBUG_USE_MODULE(RBUF);

//
// Ringbuf routines
//

static void rb_synclimit( rbuf_t* rbuf );

//
// rb_new(): create a new ring buffer
// inputs: size for the new buffer
// outputs: NULL if no memory available
//          ptr to a rbuf_t if one was allocated
// notes: this could optionally be implemented to alloc from a pool
//
rbuf_t* rb_new( unsigned int size ) 
{
    rbuf_t* rbuf;

    rbuf = (rbuf_t*) malloc( sizeof(rbuf_t) );

    DBASSERT(RBUF, rbuf != NULL, "No free memory for rbuf_t structure\n");

    memset( (void*) rbuf, 0, sizeof( rbuf_t ) );

    if( size > 0 ) {
        rbuf->data = (void*) malloc( size );

        DBASSERT(RBUF, rbuf->data != NULL, "No free memory for rbuf_t data (%d req)\n", size);
    } else {
        rbuf->data = NULL;
    }

    rbuf->size  = size;
    rbuf->flags = RB_EMPTY;
  
    return rbuf;
}

//
// rb_resize(): resize a 0 length buffer
// inputs: buffer, size
//
void rb_resize( rbuf_t* rbuf, unsigned int size )
{
    DBASSERT(RBUF, rbuf != NULL, "Null buffer pointer\n");
    if( ( rbuf->size != 0 ) ) {
        DBASSERT(RBUF, rbuf->flags & RB_EMPTY, "Resize called on non-empty buffer\n");

        free( rbuf->data );
    }

    rbuf->data = (void*) malloc( size );
    DBASSERT(RBUF, rbuf->data != NULL, "No free memory for rbuf_t data (%d req)\n", size);

    rbuf->size = size;

    //	return rbuf;
}

//
// rb_free(): release a ring buffer
// inputs: rbuf to release
//
void rb_free( rbuf_t* rbuf ) 
{
    // dont run this loop unless we are actually debugging
#if DEBUG_LEVEL > 0
    int i;
  
    DBASSERT(RBUF, rbuf != NULL, "Releasing a null rbuf\n");
    
    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        DBASSERT(RBUF, rbuf->readers[i] == NULL, "Releasing rbuf that has readers !!\n");
    }
    DBASSERT(RBUF, rbuf->writer == NULL, "Releasing rbuf that has a writer !!\n");
#endif // DEBUG_LEVEL > 0

    if( rbuf->size > 0 ) {
        free( rbuf->data );
    }
    free( (void*) rbuf );
}

//
// rbuf_synclimit: sync the writelim variable based on the reader set
// inputs: rbuf to perform the operation on
//
static void rb_synclimit( rbuf_t* rbuf ) 
{
    unsigned int min = rbuf->size * 2, count = 0;
    int i;

    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        // the tricky part here is that the min reader index
        // can be either before or after the write index. so
        // we solve this by adding rbuf->size to the read indexes
        // that are less than the write index, then adjusting the min
        // against size when we are done
        int index;

        if( rbuf->readers[i] ) {
            index = rbuf->readers[i]->readi;
            if( index < rbuf->writei || (index == rbuf->writei && (rbuf->readers[i]->flags & RR_WAITING)) ) {
                index += rbuf->size;
            }
            // do the comparison. if it is less, then it is the new
            // min. if it is equal, increment the reference count
            if( index < min ) {
                min = index;
                count = 1;
            }
            else if( index == min ) {
                count++;
            }
        }
    }
    if( min >= rbuf->size ) {
        min -= rbuf->size;
    }
    rbuf->writelim = min;
    rbuf->limcnt = count;

    rbuf->flags &= ~(RB_FULL);
    if( rbuf->writelim == rbuf->writei ) {
        rbuf->flags |= RB_EMPTY;
    }
}

//
// Writer routines
//

//
// rb_new_writer(): allocate a writer for the specified ring buffer
// inputs: rbuf to assign a writer to
// outputs: NULL if no memory available or if writer already exists
//          

rbuf_writer_t* rb_new_writer( rbuf_t* rbuf ) 
{
    rbuf_writer_t* writer;

    DBASSERT(RBUF, rbuf != NULL, "Assigning a writer to a null rbuf\n");
    DBASSERT(RBUF, rbuf->writer == NULL, "Assigning a writer to a rbuf that already has one\n");

    writer = (rbuf_writer_t*) malloc( sizeof( rbuf_writer_t ) );

    memset( (void*) writer, 0, sizeof( rbuf_writer_t ) );

    DBASSERT(RBUF, writer != NULL, "Failed to allocate rbuf_writer_t\n");

    writer->rbuf = rbuf;
    rbuf->writer = writer;

    return writer;
}

//
// rb_free_writer(): free a writer
// inputs: writer to release
//

void rb_free_writer( rbuf_writer_t* writer ) 
{
    DBASSERT(RBUF, writer != NULL, "Releasing a null writer\n");

    // remove the rbuf reference to this writer
    writer->rbuf->writer = NULL;
  
    free( (void*) writer );
}

//
// rb_write_data: start a write sequence to the buffer
// inputs:  writer - the writer we are using
//          amt    - the amount of data we want to copy
//          actual - ptr to storage
// outputs: actual - the amount of data that can be copied
//     ret: RBUF_SUCCESS - the call succeeded
//          RBUF_SPLIT   - the write must be split (refer to actual)
//          RBUF_NOSPACE - there is no space for the write
//          

ERESULT rb_write_data( rbuf_writer_t* writer, unsigned int amt, unsigned int* actual ) 
{
    ERESULT res = RBUF_SUCCESS;
    rbuf_t* rbuf;
    int write_space = 0;

    DBASSERT(RBUF, writer != NULL, "Writing to a null write handle\n");
    DBASSERT(RBUF, actual != NULL, "Null pointer for actual byte count\n");

    rbuf = writer->rbuf;

    if( rbuf->flags & RB_FULL ) {
        *actual = 0;
        return RBUF_NOSPACE;
    }
    // realign the write index if the readers have progressed
    if( rbuf->writei == rbuf->size && rbuf->writelim > 0 ) {
        rbuf->writei = 0;
    }
  
    
    // determine how much space is available to write into
    // basically we take the writelim as being the cap, use the
    // writei as our base, then subtract the base from the cap
    write_space = rbuf->writelim - rbuf->writei;

    // at this point, write_space can be negative if the writelim
    // has looped around to the start of the buffer again. in this
    // situation, we have the total space less the write_space
    // since write_space is negative, this is actually subtraction :>
    if( write_space <= 0 ) {
        write_space = rbuf->size + write_space;

        // additionally, we need to note if this is a fragmented write now
        if( (rbuf->size - rbuf->writei) < amt ) {
            res = RBUF_SPLIT;
        }
    }
  
    if( (unsigned)write_space < amt ) {
        // we can't handle the amount they want to write
        *actual = (unsigned)write_space;
        return RBUF_NOSPACE;
    }

    if( res == RBUF_SPLIT ) {
        // *actual is implicitly always less than amt here
        *actual = (rbuf->size - rbuf->writei);
    }
    else {
        *actual = amt;
    }
    return res;
}

//
// rb_write_ptr: get the pointer to use for a write
// inputs:  writer - the writer we are using
// outputs: a void* to the write destination
//

void* rb_write_ptr( rbuf_writer_t* writer ) 
{
    void* ret;
  
    DBASSERT(RBUF, writer != NULL, "Referencing a null write handle\n");

    ret = writer->rbuf->data + writer->rbuf->writei;
  
    return ret;
}

//
// rb_write_done: complete a write
// inputs:  writer - the writer we are using
//          amt    - the number of bytes actually written
// ouputs:  RBUF_SUCCESS - the operation succeeded
//          RBUF_NOSPACE - good job, you just overran the readers
//

ERESULT rb_write_done( rbuf_writer_t* writer, unsigned int amt ) 
{
    rbuf_t* rbuf;
    int i;
  
    DBASSERT(RBUF, writer != NULL, "Referencing a null write handle\n");
  
    rbuf = writer->rbuf;

    DBASSERT(RBUF, (rbuf->writei + amt) <= rbuf->size, "Write handle buffer overflow\n");

    rbuf->writei += amt;

    // loop the buffer around, but only if the reader is actually moving
    if( rbuf->writei == rbuf->size && rbuf->writelim != 0 ) {
        rbuf->writei = 0;
    }

    if( amt && (rbuf->flags & RB_EMPTY) ) {
        rbuf->flags &= ~(RB_EMPTY);
    }

    if( rbuf->writei > rbuf->size ) {
        // if this actually happens, you probably overran memory
        return RBUF_NOSPACE;
    }
    if( rbuf->writei == rbuf->size ) {
        rbuf->writei = 0;
    }
    if( rbuf->writei == rbuf->writelim ) {
        rbuf->flags |= RB_FULL;
    }
  
    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        if( rbuf->readers[i] ) {
            rbuf->readers[i]->flags &= ~(RR_WAITING);
        }
    }
  
    return RBUF_SUCCESS;
}

//
// rb_copy_write: copy data from a buffer into the ring buffer
// inputs:  writer - the writer we are using
//          source - the source to copy from
//          amt    - the number of bytes to copy
// outputs: kbSuccess - the copy succeeded
//          kbNoSpace - insufficient space for the copy
//

ERESULT rb_copy_write( rbuf_writer_t* writer, const void* source, unsigned int amt ) 
{
    ERESULT res;
    unsigned int actual;
    rbuf_t* rbuf;

    DBASSERT(RBUF, writer != NULL, "Referencing a null write handle\n");
    DBASSERT(RBUF, source != NULL, "Copying from a null source\n");

    rbuf = writer->rbuf;

    res = rb_write_data( writer, amt, &actual );

    if( res == RBUF_NOSPACE ) {
        return res;
    }

    memcpy( rb_write_ptr( writer ), source, actual );
  
    // in the event of a split, the next block of code will execute
    // because of the nature of splits and the checking that has already
    // occurred, we are certain we dont need to look at return values here
    if( actual < amt ) {
        source += actual;
        rb_write_done( writer, actual );
    
        res = rb_write_data( writer, (amt-actual), &actual );

        memcpy( rb_write_ptr( writer ), source, actual );
    }

    rb_write_done( writer, actual );
  
    return res;
}

//
// rb_write_avail: check the available write space
// inputs:  writer - the writer we are using
// outputs: number of bytes that can be written
//
unsigned int rb_write_avail( rbuf_writer_t* writer ) 
{
    int res;

    DBASSERT(RBUF, writer != NULL, "Referencing a null write handle\n");

    if( writer->rbuf->flags & RB_FULL ) {
        return 0;
    }
  
    res = writer->rbuf->writelim - writer->rbuf->writei;
    if( res <= 0 ) {
        res = writer->rbuf->size + res;
    }

    return res;
}

//
// rb_write_eof: signal eof to the stream
// inputs: the writer we are signaling this on
//
void rb_write_eof( rbuf_writer_t* writer ) 
{
    writer->rbuf->flags |= RB_EOF;
}

//
// rb_write_rbuf: get a handle to the rbuf for this writer
// inputs: the writer
// outputs: the rbuf
//
rbuf_t* rb_write_rbuf( rbuf_writer_t* writer ) 
{
    return writer->rbuf;
}



//
// Reader routines
//

//
// rb_new_reader: allocate a reader
// inputs:  rbuf - the rbuf to associate this reader with
// ouputs:  a pointer to the allocated reader
//

rbuf_reader_t* rb_new_reader( rbuf_t* rbuf ) 
{
    rbuf_reader_t* reader;
    int i;
  
    DBASSERT(RBUF, rbuf != NULL, "Referencing a null rbuf\n");

#if DEBUG_LEVEL > 0
    // if we're debugging, verify we can actually put a reader in there
    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        if( rbuf->readers[i] == NULL ) {
            break;
        }
    }
    DBASSERT(RBUF, i < RBUF_NUM_READERS, "Unable to find an available reader slot\n");
#endif
  
    reader = (rbuf_reader_t*) malloc( sizeof( rbuf_reader_t ) );

    DBASSERT(RBUF, reader != NULL, "Failed to allocate rbuf_reader_t\n");

    memset( (void*) reader, 0, sizeof( rbuf_reader_t ) );

    reader->rbuf = rbuf;

    // add the reader into the rbuf reader set
    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        if( rbuf->readers[i] == NULL ) {
            rbuf->readers[i] = reader;
            break;
        }
    }

    // set the read index to the writelim, which is the data start location
    // and increment the refcnt at that level
    reader->readi = rbuf->writelim;
    rbuf->limcnt++;

    return reader;
}

//
// rb_free_reader: release a reader
// inputs:  reader - the reader to release
//

void rb_free_reader( rbuf_reader_t* reader ) 
{
    rbuf_t* rbuf;
    int i;
  
    DBASSERT(RBUF, reader != NULL, "Referencing a null reader\n");

    rbuf = reader->rbuf;
  
    // find the reader in the rbuf reader set
    for( i = 0; i < RBUF_NUM_READERS; i++ ) {
        if( rbuf->readers[i] == reader ) {
            break;
        }
    }
  
    DBASSERT(RBUF, i < RBUF_NUM_READERS, "Unable to find reader in rbuf reader set\n");

    rbuf->readers[i] = NULL;

    // if we were one of the slow readers, then drop the reference count
    if( rbuf->writelim == reader->readi ) {
        rbuf->limcnt--;

        // if the reference count hits zero, sync the writelim var
        if( rbuf->limcnt == 0 ) {
            rb_synclimit( rbuf );
        }
    }

    free( (void*) reader );
}

//
// rb_read_data: read data from the buffer
// inputs:  reader - a reader to use
//          amt    - the number of bytes to read
// outputs: actual - the actual number of contiguous bytes to read
//          RBUF_SUCCESS - the read is ok
//          RBUF_NOSPACE - insufficient space for the read
//          RBUF_SPLIT   - the read requires two operations
//          RBUF_EOF     - end of stream
//

ERESULT rb_read_data( rbuf_reader_t* reader, unsigned int amt, unsigned int* actual ) 
{
    ERESULT res = RBUF_SUCCESS;
    rbuf_t* rbuf;
    int read_space = 0;

    DBASSERT(RBUF, reader != NULL, "Attempting to access a null reader\n");
    DBASSERT(RBUF, actual != NULL, "Attempting to reference a null pointer\n");

    rbuf = reader->rbuf;

    if( (rbuf->flags & RB_EMPTY) || (reader->flags & RR_WAITING) ) {
        *actual = 0;
        if( rbuf->flags & RB_EOF ) {
            return RBUF_EOF;
        }
        else {
            return RBUF_NOSPACE;
        }
    }
  
    // determine how much data is available to be read
    read_space = rbuf->writei - reader->readi;

    // at this point read_space can be negative, if the writer has looped around the ring
    // in which case the actual read space is rbuf->size - (reader->readi + rbuf->writei)
    if( read_space <= 0 ) {
        read_space = rbuf->size + read_space;

        // note here if this is a split
        if( (rbuf->size - reader->readi) < amt ) {
            res = RBUF_SPLIT;
        }
    }

    // verify we have enough data
    if( (unsigned)read_space < amt ) {
        *actual = (unsigned)read_space;
        return RBUF_NOSPACE;
    }

    if( res == RBUF_SPLIT ) {
        // *actual will be < amt here
        *actual = (rbuf->size - reader->readi);
    }
    else {
        *actual = amt;
    }

    if( read_space == *actual && (rbuf->flags & RB_EOF) ) {
        res = RBUF_EOF;
    }

    return res;
}

//
// rb_read_ptr: get the pointer to use for a read
// inputs:  reader - the reader to obtain the ptr from
// outputs: a void* to the buffer
//
void* rb_read_ptr( rbuf_reader_t* reader ) 
{
    void* ret;

    DBASSERT(RBUF, reader != NULL, "Referencing a null read handle\n");

    ret = reader->rbuf->data + reader->readi;

    return ret;
}


//
// rb_read_done: complete a read
// inputs:  reader - the reader we are using
//          amt    - the amount of data read
// outputs: RBUF_SUCCESS - the operation succeeded
//          RBUF_NOSPACE - you just read past the reader
//

ERESULT rb_read_done( rbuf_reader_t* reader, unsigned int amt ) 
{
    int old_index;
    rbuf_t* rbuf;

    DBASSERT(RBUF, reader != NULL, "Referencing a null read handle\n");

    rbuf = reader->rbuf;

    DBASSERT(RBUF, (reader->readi + amt) <= rbuf->size, "Read handle buffer overflow\n");

    old_index = reader->readi;
    reader->readi += amt;

    if( reader->readi == rbuf->size ) {
        reader->readi = 0;
    }

    // if we hit the write index, we are now waiting
    if( reader->readi == rbuf->writei ) {
        reader->flags |= RR_WAITING;
    }
  
    // sync the readlim
    if( old_index == rbuf->writelim ) {
        rbuf->limcnt--;

        if( rbuf->limcnt == 0 ) {
            rb_synclimit( rbuf );
        }
    }

    if( reader->readi > rbuf->size ) {
        // if this acutally happens, you read invalid data
        return RBUF_NOSPACE;
    }
    return RBUF_SUCCESS;
}

//
// rb_copy_read: copy data from the ring buffer into a mem buffer
// inputs:  reader - the reader we are using
//          dest   - where we copy the data
//          amt    - number of bytes to copy
// outputs: RBUF_SUCCESS - the copy was successfull
//          RBUF_NOSPACE - not enough data
//

ERESULT rb_copy_read( rbuf_reader_t* reader, void* dest, unsigned int amt ) 
{
    ERESULT res;
    unsigned int actual;
    rbuf_t* rbuf;

    DBASSERT(RBUF, reader != NULL, "Referencing a null read handle\n");
    DBASSERT(RBUF, dest != NULL, "Writing to a null pointer\n");

    rbuf = reader->rbuf;

    res = rb_read_data( reader, amt, &actual );

    if( res == RBUF_NOSPACE ) {
        return res;
    }

    memcpy( rb_read_ptr( reader ), dest, actual );

    if( actual < amt ) {
        dest += actual;

        rb_read_done( reader, actual );
        res = rb_read_data( reader, (amt-actual), &actual );
    
        memcpy( rb_read_ptr( reader ), dest, actual );
    }
    rb_read_done( reader, actual );
  
    return res;
}

//
// rb_read_avail: find out how much data is available
// inputs:  reader - the reader we are using
// outputs: number of bytes available to read
//
unsigned int rb_read_avail( rbuf_reader_t* reader ) 
{
    int res;

    DBASSERT(RBUF, reader != NULL, "Referencing a null read handle\n");

    if( (reader->rbuf->flags & RB_EMPTY) || (reader->flags & RR_WAITING) ) {
        return 0;
    }
  
    res = reader->rbuf->writei - reader->readi;

    if( res <= 0 ) {
        res = reader->rbuf->size + res;
    }

    return (unsigned) res;
}

//
// rb_read_eof: see if there is an eof condition
// inputs: reader - the reader we are using
// outputs:  RBUF_EOF or RBUF_SUCCESS
//

ERESULT rb_read_eof( rbuf_reader_t* reader ) {
    if( reader->rbuf->flags & RB_EOF ) {
        return RBUF_EOF;
    }
    return RBUF_SUCCESS;
}

//
// rb_read_rbuf: get a handle to the rbuf for this reader
// inputs: the reader
// outputs: the rbuf
//
rbuf_t* rb_read_rbuf( rbuf_reader_t* reader ) 
{
    return reader->rbuf;
}
