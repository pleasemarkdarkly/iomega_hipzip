//#define PROF
#ifdef PROF
#include "TinyProfile.h"
#endif

#include <stdio.h>
#include <math.h>
#define pow2(x) powf(2.0f, x)
#define PI M_PI

#include "codec.h"
#include "types.h"
#include "psy.h"
#include "memory.h"


#define max(a, b) ((a) > (b) ? (a) : (b))


void start_scalefacs(struct granule_t *gi)
{
	int i;

	for (i = 0; i < 21; i++) {
		FASTVAR(scalefac)[i] = 0;
		FASTVAR(sf)[i] = 0;
	}
	FASTVAR(slen0) = FASTVAR(slen1) = 0;
	gi->scalefac_compress = 0;
	gi->scalefac_scale = 0;
	gi->part2_length = 0;
}

/* eb: energy band? tmp gets sum of squares of xr's within a
 scalefactor band. FASTVAR(heb)[ch][i] stores previous granule's sum of squares.
 (historical energy band). Current and previous sum of squares within
 a band are combined, then weighted by sbew. (Inverse fletcher-munson?)
 */
void calc_eb(int ch)
{
	int i, j;
	fixed64_32 sum;
	fixed64_32 tmp;

	for (i = 0; i < maxpb[ch]; i++) {
		tmp = 0;
		for (j = sbo[i]; j < sbo[i+1]; j++) {
			tmp += fm16_16_64_32(FASTVAR(xr)[j], FASTVAR(xr)[j]);
		}
		sum = FASTVAR(heb)[ch][i] + tmp;
		FASTVAR(heb)[ch][i] = tmp;
		FASTVAR(eb)[i] = fm16_64_32_64_32(sbew[i], sum);
	}
}


void calc_leb(int ch)
{
	int i;

	for (i = 0; i < 21-2*ch; i++) {  // maxpb[ch], not 21-2*ch
		long long x;
		int s;

		x = FASTVAR(eb)[i];
		s = 0;
		while (x >= 256) {
			x >>= 1;
			s += 1 << 16;
		}
		FASTVAR(leb)[i] = afac + fm16(mfac, s + l2[x]);
	}
}


void calc_lebs(int ch)
{
	int i;
	fixed16 l;
	int m = maxpb[ch];

//for (i = 0; i < m; i++) fprintf(stderr, "FASTVAR(leb)[%2d] = 0x%08x\n", i, FASTVAR(leb)[i]);

	FASTVAR(lebs)[0] = -1000*65536;
	l = FASTVAR(leb)[0] - 9 * (bark[1] - bark[0]) / 2;

	for (i = 1; i < m - 1; i++) {
		FASTVAR(lebs)[i] = l;

		if (FASTVAR(leb)[i] > l)
			l = FASTVAR(leb)[i];

		l -= 9 * (bark[i+1] - bark[i]) / 2          *2;
	}
	FASTVAR(lebs)[m-1] = l;

//for (i = 0; i < m; i++) fprintf(stderr, "FASTVAR(lebs)[%2d] = 0x%08x\n", i, FASTVAR(lebs)[i]);

	l = FASTVAR(leb)[m-1] - 9 * (bark[m-1] - bark[m-2]);

	for (i = m - 2; i > 0; i--) {
		if (l > FASTVAR(lebs)[i])
			FASTVAR(lebs)[i] = l;

		if (FASTVAR(leb)[i] > l)
			l = FASTVAR(leb)[i];

		l -= 9 * (bark[i] - bark[i-1])       *2;
	}
	FASTVAR(lebs)[0] = l;

//for (i = 0; i < m; i++) fprintf(stderr, "FASTVAR(lebs)[%2d] = 0x%08x\n", i, FASTVAR(lebs)[i]);
}


void calc_lthr(int ch)
{
	int i;
	fixed16 lt;

	for (i = 0; i < maxpb[ch]; i++) {
		lt = FASTVAR(lebs)[i] - 0x90000;

/* absolute threshold */
///		if (db_bands[i] > lt)
///			lt = db_bands[i];

		FASTVAR(lthr)[i] = lt;
	}

}


void calc_sf(int ch)
{
	int i;

	for (i = 0; i < maxpb[ch]; i++)
//		FASTVAR(sf)[i] = fm16(16000, FASTVAR(lthr)[i] - 3 * FASTVAR(leb)[i]);
//		FASTVAR(sf)[i] = fm16(20000, FASTVAR(lthr)[i] - 2 * FASTVAR(leb)[i]);  // original
		FASTVAR(sf)[i] = fm16(25000, FASTVAR(lthr)[i] - 2 * FASTVAR(leb)[i]);  // original
//		FASTVAR(sf)[i] = fm16(24000, FASTVAR(lthr)[i] - FASTVAR(leb)[i]);
//		FASTVAR(sf)[i] = fm16(10000, 4 * FASTVAR(lthr)[i] - 3 * FASTVAR(leb)[i]);
}


void norm_sf(struct granule_t *gi, int ch)
{
	int i;
	fixed16 max1, min1, max2, min2, max;
	fixed16 x;
	fixed16 sum = 0;

	gi->scalefac_scale = 0;
	max1 = max2 = ftof16(-1000.0f);
	min1 = min2 = ftof16(1000.0f);
	for (i = 0; i < 11; i++) {
		x = FASTVAR(sf)[i] - ftof16(30.0f);
		sum += FASTVAR(sf)[i];
		if (x > max1)
			max1 = x;
		if (FASTVAR(sf)[i] < min1)
			min1 = FASTVAR(sf)[i];
	}
	for (i = 11; i < maxpb[ch]; i++) {
		x = FASTVAR(sf)[i] - ftof16(14.0f);
		sum += FASTVAR(sf)[i];
		if (x > max2)
			max2 = x;
		if (FASTVAR(sf)[i] < min2)
			min2 = FASTVAR(sf)[i];
	}

	sum = fm16(sum, ftof16(1.0f/21.0f));
	max = sum;

	for (i = 0; i < maxpb[ch]; i++)
		FASTVAR(sf)[i] = (FASTVAR(sf)[i] - sum) / 2 + ftof16(1.2f);

//fprintf(stderr, "wanted gain: %12.6f\n", max);
gi->global_gain = max/65536 + 170; // adjustment for bitrate && a safe margin

//fprintf(stderr, "%3d ", gi->global_gain);
}


void calc_scalefacs(int ch, struct granule_t *gi)
{
	int i;
	

	calc_eb(ch);
	
	FASTVAR(eeb)[ch] = 0;
	for (i = 0; i < maxpb[ch]; i++)
		FASTVAR(eeb)[ch] += FASTVAR(eb)[i];
	
	
	calc_leb(ch);
	calc_lebs(ch);
	calc_lthr(ch);
	
	
	if (FASTVAR(mpeg).ms_stereo) {
		int i;
		
		
		
		if (!ch) {	// mid channel
			for (i = 0; i < 144; i++)
				FASTVAR(enm)[i] = max(FASTVAR(xr)[2*i], FASTVAR(xr)[2*i+1]);
		} else {	// side channel
			for (i = 0; i < 144; i++) {
				fixed16 ens = max(FASTVAR(xr)[2*i], FASTVAR(xr)[2*i+1]);
				if (ens < fm16(enfac[i], FASTVAR(enm)[i]))
					FASTVAR(xr)[2*i] = FASTVAR(xr)[2*i+1] = 0;
			}
			for ( ; i < 288; i++)
				FASTVAR(xr)[2*i] = FASTVAR(xr)[2*i+1] = 0;
		}
	}
	
	
	calc_sf(ch);
	norm_sf(gi, ch);
	{
		int i, j;
		for (i = 0; i < maxpb[ch]; i++) {
			int f = 1;
			for (j = sbo[i]; j < sbo[i+1]; j++)
				if (FASTVAR(xr)[j]) {
					f = 0;
					break;
				}
				if (f)
					FASTVAR(sf)[i] = -10000 * 65536;
		}
	}
}


void compute_scalefacs(int ch, struct granule_t *gi)
{
	int i;
	fixed16 mean;
	fixed16 o[21];
	fixed16 min;

//////FASTVAR(sf)[0] = 0.5f * (FASTVAR(sf)[1] + FASTVAR(sf)[2]);

	min = ftof16(9999);
	mean = 0;
	for (i = 0; i < maxpb[ch]; i++) {
		o[i] = FASTVAR(old_sf)[ch][i];
		FASTVAR(old_sf)[ch][i] = FASTVAR(sf)[i];
//		FASTVAR(sf)[i] = (3.0f * FASTVAR(sf)[i] + 7.0f * o[i]) / 10.0f;
///		FASTVAR(sf)[i] = FASTVAR(sf)[i] > o[i] ? FASTVAR(sf)[i] : o[i];
//!		FASTVAR(sf)[i] = (i * FASTVAR(sf)[i] + (25.0f - i) * o[i]) / 25.0f;
//!!		FASTVAR(sf)[i] = (i * o[i] + (25.0f - i) * FASTVAR(sf)[i]) / 25.0f;
		FASTVAR(sf)[i] = (FASTVAR(sf)[i] + o[i]) / 2;
	}


	for (i = 0; i < maxpb[ch]; i++) {
		if (FASTVAR(sf)[i] < min && FASTVAR(sf)[i] >= 0)
			min = FASTVAR(sf)[i];
		if (FASTVAR(sf)[i] >= 0)
			mean += FASTVAR(sf)[i];
	}

//	mean /= 90;
	mean = 0;

	for (i = 0; i < maxpb[ch]; i++) {
		FASTVAR(scalefac)[i] = (FASTVAR(sf)[i] - min - mean)/65536;
		if (FASTVAR(scalefac)[i] < 0)
			FASTVAR(scalefac)[i] = 0;
	}

//	/* try preemphasis */
//	gi->pre_emphasis = 1;
//	for (i = 0; i < 21; i++)
//		if (FASTVAR(scalefac)[i] < preemp[i])
//			gi->pre_emphasis = 0;
//	if (gi->pre_emphasis)
//		for (i = 0; i < 21; i++)
//			FASTVAR(scalefac)[i] -= preemp[i];

	for (i = 0; i < maxpb[ch]; i++) {
//		if (FASTVAR(scalefac)[i] < 0) FASTVAR(scalefac)[i] = 0;
		if (FASTVAR(scalefac)[i] > 15) FASTVAR(scalefac)[i] = 15;
///if (FASTVAR(scalefac)[i] > 7) FASTVAR(scalefac)[i] = 7;
		if (FASTVAR(scalefac)[i] > 7 && i >= 11) FASTVAR(scalefac)[i] = 7;
	}


//if (FASTVAR(scalefac)[0] < 4)
//	FASTVAR(scalefac)[0] = 4;


	// Compute best scalefac_compress
	FASTVAR(slen0) = FASTVAR(slen1) = 0;
	for (i = 0; i < 11; i++)
		if (FASTVAR(scalefac)[i] > FASTVAR(slen0))
			FASTVAR(slen0) = FASTVAR(scalefac)[i];
	for ( ; i < maxpb[ch]; i++)
		if (FASTVAR(scalefac)[i] > FASTVAR(slen1))
			FASTVAR(slen1) = FASTVAR(scalefac)[i];
	FASTVAR(slen0) = slen[FASTVAR(slen0)];
	FASTVAR(slen1) = slen[FASTVAR(slen1)];
	for (i = 0; i < 16; i++)
		if (sf0[i] >= FASTVAR(slen0) && sf1[i] >= FASTVAR(slen1))
			break;
	if (i > 15) 
		longjmp(error_jbuf, PEM_ILLEGAL_SCALEFAC_COMPRESS);
	FASTVAR(slen0) = sf0[i];
	FASTVAR(slen1) = sf1[i];
	gi->scalefac_compress = i;


	if (gi->block_type != 2)
		gi->part2_length = 11 * FASTVAR(slen0) + 10 * FASTVAR(slen1);
	else
		gi->part2_length = 18 * FASTVAR(slen0) + 18 * FASTVAR(slen1);
}


void apply_scalefacs(int ch, struct granule_t *gi)
{
	int i, j;

	int shift;

	if (gi->scalefac_scale) {
		for (i = 0; i < maxpb[ch]; i++) {
			shift = FASTVAR(scalefac)[i];
			for (j = sbo[i]; j < sbo[i+1]; j++)
				FASTVAR(xr)[j] = FASTVAR(xr)[j] << shift;
		}
	} else {
		for (i = 0; i < maxpb[ch]; i++) {
			shift = FASTVAR(scalefac)[i] >> 1;
			if (FASTVAR(scalefac)[i] & 1) {
				for (j = sbo[i]; j < sbo[i+1]; j++)
					FASTVAR(xr)[j] = fm16(FASTVAR(xr)[j] << shift, 0x16a09); // sqrt(2)
			} else {
				for (j = sbo[i]; j < sbo[i+1]; j++)
					FASTVAR(xr)[j] = FASTVAR(xr)[j] << shift;
			}
		}
	}
}

