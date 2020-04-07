// rbuf_type.h: types for rbuf structures
// danc@iobjects.com 07/05/01
// (c) Interactive Objects

#ifndef __RBUF_TYPE_H__
#define __RBUF_TYPE_H__

#include <util/rbuf/_rbuf.h>

//#define RBUF_NUM_READERS 3

typedef struct rbuf_s 
{
    struct rbuf_writer_s* writer; // pointer to the writer
    unsigned int writei;   // write index
  
    struct rbuf_reader_s* readers[ RBUF_NUM_READERS ];   // array of people reading
    unsigned int writelim; // write limit (effectively the read index for the slowest reader)
    unsigned int limcnt;   // number of items at the read index
  
    unsigned int size;     // size of the buffer
    void* data;            // buffer pointer
  
    unsigned char flags;   // buffer flags
#define RB_FULL          0x01     // the buffer is full
#define RB_EMPTY         0x02     // the buffer is empty
#define RB_EOF           0x04     // the source is dry
} rbuf_t;

typedef struct rbuf_reader_s 
{
    rbuf_t* rbuf;          // buffer we are reading from
    unsigned int readi;    // read index
    unsigned char flags;   // read handle flags
#define RR_WAITING       0x01     // waiting for more data
} rbuf_reader_t;

typedef struct rbuf_writer_s
{
    rbuf_t* rbuf;          // buffer we are writing to
} rbuf_writer_t;

#define RBUF_STRUCTURES_DEFINED

#endif // __RBUF_TYPE_H__
