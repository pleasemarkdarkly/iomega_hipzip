#include <stdio.h>
#include <string.h>

#if defined(DESKTOP) || defined(STANDALONE)
#include "codec.h"
#else
#include <codec/mp3/pem/codec.h>
#endif

#include "types.h"
#include "huffman.h"
#include "out.h"
#include "memory.h"

FAST_FUNC ERESULT pem_do_out(const unsigned char *x, int n)
{
//	fprintf(stderr,"0x%08x, %d\n", sram_p->outbuf, n);
	memcpy(sram_p->outbuf, x, n);
	sram_p->outbuf += n;
	sram_p->outcount +=n;
	return PEM_NO_ERROR;
}

FAST_FUNC void new_sbitter(unsigned long *buf)
{
	FASTVAR(sbuffer) = buf;
	FASTVAR(sbits_left) = 32;
}

FAST_FUNC void new_dbitter(unsigned long *buf)
{
	FASTVAR(dbuffer) = buf;
	FASTVAR(dbits_left) = 32;
}

FAST_FUNC unsigned long SWAP(unsigned long x)
{
#ifdef BIGENDIAN
	return x;
#else
#if 0
	__asm__ ("bswapl %0" : "=r"(x) : "0"(x));
	return x;
#else
	return ((x & 0xff000000) >> 24) | ((x & 0xff0000) >> 8) | ((x & 0xff00) << 8) | ((x & 0xff) << 24);
#endif
#endif
}


FAST_FUNC void sbitter(unsigned long x, int n)
{
	if (n < FASTVAR(sbits_left)) {
		FASTVAR(sbufferword) = (FASTVAR(sbufferword) << n) | x;
		FASTVAR(sbits_left) -= n;
	} else {
		*FASTVAR(sbuffer)++ = SWAP((FASTVAR(sbufferword) << FASTVAR(sbits_left)) | (x >> (n - FASTVAR(sbits_left))));
		FASTVAR(sbufferword) = x & enen[n - FASTVAR(sbits_left)];
		FASTVAR(sbits_left) += 32 - n;
	}
}


FAST_FUNC void dbitter(unsigned long x, int n)
{
	if (n < FASTVAR(dbits_left)) {
		FASTVAR(dbufferword) = (FASTVAR(dbufferword) << n) | x;
		FASTVAR(dbits_left) -= n;
	} else {
		*FASTVAR(dbuffer)++ = SWAP((FASTVAR(dbufferword) << FASTVAR(dbits_left)) | (x >> (n - FASTVAR(dbits_left))));
		FASTVAR(dbufferword) = x & enen[n - FASTVAR(dbits_left)];
		FASTVAR(dbits_left) += 32 - n;
	}
}


FAST_FUNC void done_sbitter(void)
{
	if (FASTVAR(sbits_left) != 32) 
		longjmp(error_jbuf, PEM_SBITTER_LEFTOVER_BITS);
}


FAST_FUNC int tell_dbitter(unsigned long *buf)
{
	return 32 * (FASTVAR(dbuffer) - buf + 1) - FASTVAR(dbits_left);
}


FAST_FUNC void pad_dbitter(int no)
{
if (no < 0) {
	longjmp(error_jbuf, PEM_PAD_DBITTER_NEGATIVE);
}

	if (no <= FASTVAR(dbits_left))
		dbitter(enen[no], no);
	else {
		no -= FASTVAR(dbits_left);
		dbitter(enen[FASTVAR(dbits_left)], FASTVAR(dbits_left));
		while (no >= 32) {
			no -= 32;
			*FASTVAR(dbuffer)++ = 0xffffffff;
		}
		dbitter(enen[no], no);
	}
}


FAST_FUNC int flush_dbitter(unsigned long *buf)
{
	int no;

	dbitter(enen[FASTVAR(dbits_left) & 7], FASTVAR(dbits_left) & 7);
	no = FASTVAR(dbits_left) >> 3;
	dbitter(enen[FASTVAR(dbits_left)], FASTVAR(dbits_left));
	no = 4 * (FASTVAR(dbuffer) - buf) - no;
	return no;
}


#define sxs2(rx0, rx1) { \
	if (FASTVAR(sxsn) == 0) { \
		FASTVAR(sxss) = FASTVAR(sxs)[FASTVAR(sxsi)++]; \
		FASTVAR(sxsn) = 32; \
	} \
	rx0 = (FASTVAR(sxss) >> 0) & 1; \
	rx1 = (FASTVAR(sxss) >> 1) & 1; \
	FASTVAR(sxss) >>= 2; \
	FASTVAR(sxsn) -= 2; \
}


#define sxs4(rx0, rx1, rx2, rx3) { \
	if (FASTVAR(sxsn) == 0) { \
		FASTVAR(sxss) = FASTVAR(sxs)[FASTVAR(sxsi)++]; \
		FASTVAR(sxsn) = 32; \
	} \
	rx0 = (FASTVAR(sxss) >> 0) & 1; \
	rx1 = (FASTVAR(sxss) >> 1) & 1; \
	rx2 = (FASTVAR(sxss) >> 2) & 1; \
	rx3 = (FASTVAR(sxss) >> 3) & 1; \
	FASTVAR(sxss) >>= 4; \
	FASTVAR(sxsn) -= 4; \
}


static void huffs(int tab, int a, int b)
{
	const struct huffcodetab *h;
	int i;
	int len;
	const unsigned long *table;

	if (tab == 0)
		return;

	h = &ht[tab];
	len = h->len;
	switch (tab) {
		case 1: table = t1HB; break;
		case 2: table = t2HB; break;
		case 3: table = t3HB; break;
		case 5: table = t5HB; break;
		case 6: table = t6HB; break;
		case 7: table = t7HB; break;
		case 8: table = t8HB; break;
		case 9: table = t9HB; break;
		case 10: table = t10HB; break;
		case 11: table = t11HB; break;
		case 12: table = t12HB; break;
		case 13: table = t13HB; break;
		case 15: table = t15HB; break;
		case 16: table = t16HB; break;
		case 17: table = t16HB; break;
		case 18: table = t16HB; break;
		case 19: table = t16HB; break;
		case 20: table = t16HB; break;
		case 21: table = t16HB; break;
		case 22: table = t16HB; break;
		case 23: table = t16HB; break;
		case 24: table = t24HB; break;
		case 25: table = t24HB; break;
		case 26: table = t24HB; break;
		case 27: table = t24HB; break;
		case 28: table = t24HB; break;
		case 29: table = t24HB; break;
		case 30: table = t24HB; break;
		case 31: table = t24HB; break;
		default: table = 0;  // this will trap errors ;-)
	};

	if (tab >= 16) {
		int linbits = h->linbits;

		for (i = a; i < b; i += 2) {
			int x = FASTVAR(ix)[i];
			int y = FASTVAR(ix)[i+1];
			int rx;
			int ry;
			int idx;

			sxs2(rx, ry);
			if (x >= 15) {
				if (y >= 15) {
					x -= 15;
					y -= 15;
					idx = 15 * len + 15;
					dbitter(table[idx] & 0xffffff, table[idx] >> 24);
					dbitter(x, linbits);
					dbitter(rx, 1);
					dbitter(y, linbits);
					dbitter(ry, 1);
				} else {
					x -= 15;
					idx = 15 * len + y;
					dbitter(table[idx] & 0xffffff, table[idx] >> 24);
					dbitter(x, linbits);
					dbitter(rx, 1);
					if (y)
						dbitter(ry, 1);
				}
			} else {
				if (y >= 15) {
					y -= 15;
					idx = x * len + 15;
					dbitter(table[idx] & 0xffffff, table[idx] >> 24);
					if (x)
						dbitter(rx, 1);
					dbitter(y, linbits);
					dbitter(ry, 1);
				} else {
					idx = x * len + y;
					dbitter(table[idx] & 0xffffff, table[idx] >> 24);
					if (x)
						dbitter(rx, 1);
					if (y)
						dbitter(ry, 1);
				}
			}
		}
	} else {
		for (i = a; i < b; i += 2) {
			int x = FASTVAR(ix)[i];
			int y = FASTVAR(ix)[i+1];
			int rx;
			int ry;
			int idx;

			sxs2(rx, ry);
			idx = x * len + y;
			dbitter(table[idx] & 0xffffff, table[idx] >> 24);
			if (x)
				dbitter(rx, 1);
			if (y)
				dbitter(ry, 1);
		}
	}
}


static void huffs1(int tab, int a, int b)
{
	const struct huffcodetab *h;
	const char *hl;
	const unsigned long *table;
	int i;

	h = &ht[tab + 32];
	if (!tab) {
		hl = t32l;
		table = t32HB;
	} else {
		hl = t33l;
		table = t33HB;
	}

	for (i = a; i < b; i += 4) {
		int idx;
		int x = FASTVAR(ix)[i];
		int y = FASTVAR(ix)[i+1];
		int z = FASTVAR(ix)[i+2];
		int w = FASTVAR(ix)[i+3];
		int rx;
		int ry;
		int rz;
		int rw;

		sxs4(rx, ry, rz, rw);
		idx = (x << 3) + (y << 2) + (z << 1) + w;
		dbitter(table[idx], hl[idx]);
		if (x)
			dbitter(rx, 1);
		if (y)
			dbitter(ry, 1);
		if (z)
			dbitter(rz, 1);
		if (w)
			dbitter(rw, 1);
	}
}


static void do_pad(int n)
{
	unsigned char pad = 0;
	while (n--)
		ASSERT_SUCCESS(pem_do_out(&pad, 1));
}


void out_start(void)
{
	FASTVAR(mainlength)[FASTVAR(sideno)] = FASTVAR(mpeg).frame_size - 36;
	new_sbitter(sidebitter + 9 * FASTVAR(sideno));

	sbitter(0xfffb, 16);
	sbitter(FASTVAR(mpeg).bitrate_index, 4);
	sbitter(0, 4);
	if (FASTVAR(mpeg).ms_stereo) {
		sbitter(1, 2); // joint stereo
		sbitter(2, 2); // MS_LR
	} else {
//		sbitter(0, 2); // stereo
		sbitter(1, 2); // joint stereo
		sbitter(0, 2); // LR_LR
	}
	sbitter(0, 4);

	sbitter(FASTVAR(main_data_begin), 9);
	if (FASTVAR(main_data_begin) < 0 || FASTVAR(main_data_begin) > 511)
		longjmp(error_jbuf, PEM_MAIN_DATA_OUT_OF_RANGE);
	sbitter(0, 3);
	sbitter(0, 8);		// scfsi

	new_dbitter(mainbitter);
}


void out(struct granule_t *gi)
{
	int written;
	int i;

	if (gi->block_type != 2) {
		for (i = 0; i < 6; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen0));
		for (i = 6; i < 11; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen0));
		for (i = 11; i < 16; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen1));
		for (i = 16; i < 21; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen1));
	} else {
		for (i = 0; i < 18; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen0));
		for (i = 18; i < 36; i++)
			dbitter(FASTVAR(scalefac)[i], FASTVAR(slen1));
	}

	written = tell_dbitter(mainbitter);

	FASTVAR(sxsi) = 0;
	FASTVAR(sxsn) = 0;
	huffs(gi->table_select[0], 0, gi->pos0);
	huffs(gi->table_select[1], gi->pos0, gi->pos1);
	huffs(gi->table_select[2], gi->pos1, gi->pos2);

	huffs1(gi->count1table_select, gi->pos2, gi->pos3);

	written = tell_dbitter(mainbitter) - written;
	pad_dbitter(gi->part3_length - written);
}


void out_done(int padding)
{
	unsigned char *mm;
	int bytes_used;
	int i;
	int gr, ch;
	struct granule_t *gi;

	for (gr = 0; gr < 2; gr++) {
		for (ch = 0; ch < 2; ch++) {
			gi = &FASTVAR(granule)[gr][ch];

			sbitter(gi->part2_length + gi->part3_length, 12);
			sbitter(gi->pos2 >> 1, 9);
			sbitter(gi->global_gain, 8);
			sbitter(gi->scalefac_compress, 4);
if (gi->block_type == 0) {
			sbitter(0, 1);
			sbitter(gi->table_select[0], 5);
			sbitter(gi->table_select[1], 5);
			sbitter(gi->table_select[2], 5);
			sbitter(gi->region0_count, 4);
			sbitter(gi->region1_count, 3);
} else {
			sbitter(1, 1);
			sbitter(gi->block_type, 2);
			sbitter(0, 1);		// mixed block flag
			sbitter(gi->table_select[0], 5);
			sbitter(gi->table_select[1], 5);
			sbitter(gi->subblock_gain[0], 3);
			sbitter(gi->subblock_gain[1], 3);
			sbitter(gi->subblock_gain[2], 3);
}
			// (dd, 30 may 2002)
			// this looks like the preflag bit, spec section 2.4.1.7
			// still not quite sure what to do with it
			// see trackgear #411 for the unfolding drama
			sbitter(0, 1); 
			sbitter(gi->scalefac_scale, 1);
			sbitter(gi->count1table_select, 1);
		}
	}

	done_sbitter();
	FASTVAR(sideno)++;

pad_dbitter(padding);

	bytes_used = flush_dbitter(mainbitter);
	FASTVAR(skew) += FASTVAR(mpeg).frame_size - bytes_used - 36;
	mm = (unsigned char *)mainbitter;
	while (bytes_used > FASTVAR(bytes_to_do)) {
		ASSERT_SUCCESS(pem_do_out(mm, FASTVAR(bytes_to_do)));
		mm += FASTVAR(bytes_to_do);
		bytes_used -= FASTVAR(bytes_to_do);
		FASTVAR(bytes_to_do) = FASTVAR(mainlength)[0];
		ASSERT_SUCCESS(pem_do_out((unsigned char *)sidebitter, 36));
		for (i = 0; i < 9 * (FASTVAR(sideno) - 1); i++)
			sidebitter[i] = sidebitter[i + 9];
		for (i = 0; i < FASTVAR(sideno) - 1; i++)
			FASTVAR(mainlength)[i] = FASTVAR(mainlength)[i + 1];
		FASTVAR(sideno)--;
	}
	ASSERT_SUCCESS(pem_do_out(mm, bytes_used));
	FASTVAR(bytes_to_do) -= bytes_used;

	FASTVAR(main_data_begin) = FASTVAR(skew);
}


void out_init(void)
{
	FASTVAR(skew) = 0;

	FASTVAR(bytes_to_do) = 0;
	FASTVAR(sideno) = 0;

	FASTVAR(main_data_begin) = 0;
}


void out_fini(void)
{
	int i;

	do_pad(FASTVAR(bytes_to_do));

	for (i = 0; i < FASTVAR(sideno); i++) {
		ASSERT_SUCCESS(pem_do_out((unsigned char *)sidebitter + 9 * i, 36));
		FASTVAR(bytes_to_do) = FASTVAR(mainlength)[i];
		do_pad(FASTVAR(bytes_to_do));
	}
}

