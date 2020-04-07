#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__

struct huffcodetab {
  int len;
  int linbits;
  int linmax;
//  /*const*/ unsigned long *table;
};

void init_hbits(void);

#endif//__HUFFMAN_H__
