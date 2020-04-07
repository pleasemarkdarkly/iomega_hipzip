#ifndef __IN_H__
#define __IN_H__

#include "types.h"

int in_open(char type);
void in_close(void);
void in_do(short *x, int n);


//need a better place to interface these
void swap2(unsigned short *x);
void swap4(unsigned long *x);

#endif//__IN_H__
