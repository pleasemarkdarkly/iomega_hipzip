// device_io.h
// temancl@fullplaymedia.com 
// (c) fullplay media 03/06/02
// 
// description:
// low level io routines, printf, string functions, types
#ifndef _DEVICE_IO_H_
#define _DEVICE_IO_H_

#include <stddef.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <termios.h>
#include <string.h>

#define MAX_LINE_LENGTH 100

#define DEBUG3(a,s...) if(verbosity >= 3) printf(a,##s);
#define DEBUG2(a,s...) if(verbosity >= 2) printf(a,##s);
#define DEBUG1(a,s...) if(verbosity >= 1) printf(a,##s);


void shell_io_init();
// pretty print
void pprint_start(void* buffer);
void pprint_end();
void pprint_byte(cyg_uint8 byte);
void pprint_halfword(cyg_uint16 half);
void pprint_word(cyg_uint32 word);
// input helpers
int isbreak();
// int isprint(int c);
unsigned long axtoi(char *hexStg);
char* getline();
char* strtoken(char* str, const char * toks);
int strncmpci(const char *s1, const char *s2, int len);
void reset_keypress(void);
void set_keypress(void);
#endif