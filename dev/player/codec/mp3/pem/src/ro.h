#ifndef __RO_H__
#define __RO_H__

//#include "tables.h"
#include "huffman.h"

extern const int bitrates[15];
extern const int framesizes[15];
extern const fixed13 qtab[8193];
extern /*const*/ unsigned long t13HB[256];
extern /*const*/ unsigned long t15HB[256];
extern fixed40 enwindow[512];
extern const short wwin_init[512];

extern const struct ro_sram {
	unsigned long enen[33];
	int maxs[2];
	int maxpb[2];
	int sbo[23];
	fixed16 sbew[21];
	fixed16 bark[22];
	int afac, mfac;
	int l2[256];
	struct subdv_table {
		int region0_count;
		int region1_count;
	} subdv_table[23];
	int huftab0[15];
	int huftab1[15];
	int huftab2[15];
	int iwinn[18];
	int itan0[9];
	int trig[7];
	fixed16 ca[8];
	fixed16 cs[8];
	fixed16 costab[31];
	fixed16 enfac[144];
	int slen[16];
	int sf0[16];
	int sf1[16];
	fixed16 gaintab[256];
	fixed13 qq0tab[256];
	unsigned long t1HB[4];
	unsigned long t2HB[9];
	unsigned long t3HB[9];
	unsigned long t5HB[16];
	unsigned long t6HB[16];
	unsigned long t7HB[36];
	unsigned long t8HB[36];
	unsigned long t9HB[36];
	unsigned long t10HB[64];
	unsigned long t11HB[64];
	unsigned long t12HB[64];
	unsigned long t16HB[256];
	unsigned long t24HB[256];
	unsigned long t32HB[16];
	unsigned long t33HB[16];
	unsigned char t1l[4];
	unsigned char t2l[9];
	unsigned char t3l[9];
	unsigned char t5l[16];
	unsigned char t6l[16];
	unsigned char t7l[36];
	unsigned char t8l[36];
	unsigned char t9l[36];
	unsigned char t10l[64];
	unsigned char t11l[64];
	unsigned char t12l[64];
	unsigned char t13l[256];
	unsigned char t15l[256];
	unsigned char t16l[256];
	unsigned char t24l[256];
	unsigned char t32l[16];
	unsigned char t33l[16];
	struct huffcodetab ht[34];
//	char *hlen[34];
} rom_ro_sram;

#endif//__RO_H__
