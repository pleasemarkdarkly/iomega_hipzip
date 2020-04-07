// rbuf.h: interface to ring buffer system
// danc@iobjects.com 07/05/01
// (c) Interactive Objects

// This is a non-thread safe buffering mechanism designed
// to allow multiple readers

#ifndef __RBUF_H__
#define __RBUF_H__

#include <util/eresult/eresult.h>

#define RBUF_ERROR_ZONE   0x01

#define MAKE_RBUFRESULT( x, y )  MAKE_ERESULT( x, RBUF_ERROR_ZONE, y )

const ERESULT RBUF_SUCCESS  = MAKE_RBUFRESULT( SEVERITY_SUCCESS, 0x0000 );
const ERESULT RBUF_SPLIT    = MAKE_RBUFRESULT( SEVERITY_SUCCESS, 0x0001 );
const ERESULT RBUF_NOSPACE  = MAKE_RBUFRESULT( SEVERITY_FAILED,  0x0002 );
const ERESULT RBUF_EOF      = MAKE_RBUFRESULT( SEVERITY_FAILED,  0x0003 );

//
// Externally defined types
//
#ifndef RBUF_STRUCTURES_DEFINED
typedef struct rbuf_s          rbuf_t;
typedef struct rbuf_reader_s   rbuf_reader_t;
typedef struct rbuf_writer_s   rbuf_writer_t;
#endif

//
// Functions
//
#ifdef __cplusplus
extern "C" {
#endif // __cplusplus
#if 0
}
#endif // 0

//
// Allocate/release/resize an rbuf
//
rbuf_t* rb_new( unsigned int size );
void	rb_resize( rbuf_t* rbuf, unsigned int size );
void    rb_free( rbuf_t* rbuf );

//
// Allocate/release a writer
//
rbuf_writer_t* rb_new_writer( rbuf_t* rbuf );
void           rb_free_writer( rbuf_writer_t* writer );

//
// Data operations
//
ERESULT        rb_write_data(  rbuf_writer_t* writer, unsigned int amt, unsigned int* actual );
void*          rb_write_ptr(   rbuf_writer_t* writer );
ERESULT        rb_write_done(  rbuf_writer_t* writer, unsigned int amt );
ERESULT        rb_copy_write(  rbuf_writer_t* writer, const void* source, unsigned int amt );
unsigned int   rb_write_avail( rbuf_writer_t* writer );
void           rb_write_eof(   rbuf_writer_t* writer );
rbuf_t*        rb_write_rbuf(  rbuf_writer_t* writer );


//
// Allocate/release a reader
//
rbuf_reader_t* rb_new_reader( rbuf_t* rbuf );
void           rb_free_reader( rbuf_reader_t* reader );

//
// Data operations
//
ERESULT        rb_read_data( rbuf_reader_t* reader, unsigned int amt, unsigned int* actual );
void*          rb_read_ptr(  rbuf_reader_t* reader );
ERESULT        rb_read_done( rbuf_reader_t* reader, unsigned int amt );
ERESULT        rb_copy_read( rbuf_reader_t* reader, void* dest, unsigned int amt );
unsigned int   rb_read_avail( rbuf_reader_t* reader );
ERESULT        rb_read_eof(  rbuf_reader_t* reader );
rbuf_t*        rb_read_rbuf( rbuf_reader_t* reader );


#ifdef __cplusplus
};
#endif  // __cplusplus

#endif  // __RBUF_H__
