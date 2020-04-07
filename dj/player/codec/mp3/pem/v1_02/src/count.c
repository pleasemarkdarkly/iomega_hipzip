//#define PROF_PARTS

//#include "TinyProfile.h"
#include <stdio.h>

#include <math.h>

#include "memory.h"
#include "huffman.h"
#include "quant.h"


int ix_max(int begin, int end) // heb-em nodig in show_scalefacs
{
	int i;
	int x;
	short *pix;
	int max = 0;
	
	
	for (i = end - begin, pix = FASTVAR(ix) + begin; i; i--) { 
		x = *pix++;
		if (x > max)
			max=x;
	}
	
	return max;
}


static void calc_runlen(struct granule_t *gi)
{
	gi->pos2 = (FASTVAR(limit_big) + 3) & ~3;
	gi->pos3 = (FASTVAR(limit_nonzero) + 3) & ~3;
}



static int count1_bitcount(struct granule_t *gi)
{
	int p, i;
	int sum0 = 0, sum1 = 0;
	unsigned char c[16];
	short *ixi;

	for (i = 0; i < 16; i++)
		c[i] = 0;

	for (ixi = FASTVAR(ix) + gi->pos2; ixi < FASTVAR(ix) + gi->pos3; ixi += 4) {
		p = ixi[0];
		p = ixi[1] + (p << 1);
		p = ixi[2] + (p << 1);
		p = ixi[3] + (p << 1);

		c[p]++;
	}

	for (i = 0; i < 16; i++) {
		sum0 += (int)c[i] * FASTVAR(hbits)[32][0][i];
		sum1 += (int)c[i] * FASTVAR(hbits)[33][0][i];
	}

	if (sum0 < sum1) {
		gi->count1table_select = 0;
		return sum0;
	} else {
		gi->count1table_select = 1;
		return sum1;
	}
}


static void subdivide(struct granule_t *gi)
{
	int j;
	int big;
	int c, i;

	if (gi->pos2 == 0) {
		gi->region0_count = 0;
		gi->region1_count = 0;
		return;
	}

	big = gi->pos2;
	for (j = 0; sbo[j] < big; j++)
		;
	gi->region0_count = subdv_table[j].region0_count;
	gi->region1_count = subdv_table[j].region1_count;

	i = gi->region0_count + 1;
	c = gi->region0_count;
	for ( ; c && sbo[i] > big; c--, i--)
		;
	gi->region0_count = c;

	i = gi->region0_count + gi->region1_count + 2;
	c = gi->region1_count;
	for ( ; c && sbo[i] > big; c--, i--)
		;
	gi->region1_count = c;

	gi->pos0 = sbo[gi->region0_count + 1];
	gi->pos1 = sbo[gi->region0_count + gi->region1_count + 2];
}


static int count_bit(int start, int end, int table)
{
	int i, sum;
	const struct huffcodetab *h;

	if (table == 0)
		return 0;

	h = &(ht[table]);
	sum = 0;

	for (i = start; i < end; i += 2) {
		int x, y;

		x = FASTVAR(ix)[i];
		y = FASTVAR(ix)[i + 1];

		sum += FASTVAR(hbits)[table][x][y];
	}
//{ static int max[34] = { 0, }; if (sum > max[table]) { max[table] = sum; if (sum >= 2048) fprintf(stderr, "*** %d: %d ***\n", table, max[table]); } }

	return sum;
}


static int count_bit_big(int start, int end, int table)
{
	int i, sum;
	const struct huffcodetab *h;

	h = &(ht[table]);
	sum = 0;

	for (i = start; i < end; i += 2) {
		int x, y;

		x = FASTVAR(ix)[i];
		y = FASTVAR(ix)[i + 1];

		sum += FASTVAR(hbits)[table][x > 14 ? 15 : x][y > 14 ? 15 : y];
	}

	return sum;
}


static int choose_table(int begin, int end, int *bits)
{
	int i, max;
	int s0, s1, s2;

	max = ix_max(begin, end);

	if (!max) {
		*bits = 0;
		return 0;
	}

	if (max < 15) {
		s0 = count_bit(begin, end, huftab0[max]);
		s1 = huftab1[max] ? count_bit(begin, end, huftab1[max]) : 999999;
		s2 = huftab2[max] ? count_bit(begin, end, huftab2[max]) : 999999;

		if (s0 <= s1) {
			if (s0 <= s2) {
				*bits = s0;
				return huftab0[max];
			} else {
				*bits = s2;
				return huftab2[max];
			}
		} else {
			if (s1 <= s2) {
				*bits = s1;
				return huftab1[max];
			} else {
				*bits = s2;
				return huftab2[max];
			}
		}
	} else {
		int choice[2];
		int sum[2];

		max -= 15;
	
		for (i = 16; i < 24; i++)
			if (ht[i].linmax >= max) {
				choice[0] = i;
				break;
			}

		for (i = 24; i < 32; i++)
			if (ht[i].linmax >= max) {
				choice[1] = i;
				break;
			}
	
		sum[0] = count_bit_big(begin, end, choice[0]);
		sum[1] = count_bit_big(begin, end, choice[1]);

		if (sum[1] < sum[0]) {
			*bits = sum[1];
			return choice[1];
		} else {
			*bits = sum[0];
			return choice[0];
		}
	}
}


static int bigv_tab_select(struct granule_t *gi)
{
	int ac, x;

	gi->table_select[0] = 0;
	gi->table_select[1] = 0;
	gi->table_select[2] = 0;

	ac = 0;

	if (gi->pos0 > 0) {
		gi->table_select[0] = choose_table(0, gi->pos0, &x);
		ac += x;
	}
	if (gi->pos1 > gi->pos0) {
		gi->table_select[1] = choose_table(gi->pos0, gi->pos1, &x);
		ac += x;
	}
	if (gi->pos2 > gi->pos1) {
		gi->table_select[2] = choose_table(gi->pos1, gi->pos2, &x);
		ac += x;
	}

	return ac;
}


int count_bits(struct granule_t *gi)
{
	int bits;

///static profile_data pd; profile_start(&pd);
#ifdef PROF_PARTS
{static profile_data pd; profile_start(&pd);
#endif

	calc_runlen(gi);

#ifdef PROF_PARTS
profile_end(&pd, "calc_runlen");}
{static profile_data pd; profile_start(&pd);
#endif

	bits = count1_bitcount(gi);

#ifdef PROF_PARTS
profile_end(&pd, "count1_bitcount");}
{static profile_data pd; profile_start(&pd);
#endif

	subdivide(gi);

#ifdef PROF_PARTS
profile_end(&pd, "subdivide");}
{static profile_data pd; profile_start(&pd);
#endif

	bits += bigv_tab_select(gi);

#ifdef PROF_PARTS
profile_end(&pd, "bigv_tab_select+count");}
#endif
///profile_end(&pd, "count_bits");


	return bits;
}

