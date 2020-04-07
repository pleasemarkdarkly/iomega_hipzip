#ifndef _BOOT_TYPES_H
#define _BOOT_TYPES_H

#include <cyg/infra/diag.h>
#define DEBUG_BOOTLOADER diag_printf
//#define DEBUG_BOOTLOADER(x,y...)
// Decompression support


typedef int BOOL;
#define TRUE 1
#define FALSE 0


typedef struct _pipe {
    unsigned char* in_buf;              // only changed by producer
    int in_avail;                       // only changed by producer
    unsigned char* out_buf;             // only changed by consumer (init by producer)
    int out_size;                       // only changed by consumer (init by producer)
    const char* msg;                    // message from consumer
    void* priv;                         // handler's data
} _pipe_t;

typedef int _decompress_fun_init(_pipe_t*);
typedef int _decompress_fun_inflate(_pipe_t*);
typedef int _decompress_fun_close(_pipe_t*, int);

int strlen(const char *s);
//int strcmp(const char *s1, const char *s2);
char *strncpy(char *s1, const char *s2, unsigned long n);
int memcmp(const void *_dst, const void *_src, size_t len);
char *strcpy(char *s1, const char *s2);
char *strcat(char *s1, const char *s2);
int strcmpci(const char *s1, const char *s2);
#endif
