#define HA 3
#define HB 3
#define HC 6


//#define PROF
#ifdef PROF
#include "TinyProfile.h"
#endif

#include <stdlib.h>

#define ALPHA -0.0081222
#define CUT 2
//#define CUT 20000

#define ALLOCATE_BITS_BY_SIDE
#define NO_BITS_HISTORY

#define QFAC (4.0f/3.0f)

#define MIN_GAIN 150
#define MAX_GAIN 255

#include <stdio.h>

#include <math.h>
#define pow2(x) powf(2.0f, x)

#include "types.h"
#include "count.h"
#include "reservoir.h"
#include "psy.h"
#include "quant.h"
#include "memory.h"


void init_quant(void)
{
	int i;
    for (i = 0; i < 256; i++) {
		qq0tab[i] = ((((long long)qtab[0]+1) << 19) - 1) / gaintab[i];
        if ( i < 142 )
            qq0tab[i] = 0;
    }
}


static int correct_scale(int band, struct granule_t *gi)
{
	int i, j;
	fixed31 fac;

	if (gi->scalefac_scale)
		fac = 0x40000000; // 0.5
	else
		fac = 0x5a827999; // sqrt(0.5);

	for (j = 0; band >= sbo[j + 1]; j++) ;
	if (!FASTVAR(scalefac)[j])
		return 0;

	FASTVAR(scalefac)[j]--;

	for (i = sbo[j]; i < sbo[j+1]; i++)
		FASTVAR(xr)[i] = fm16_31_16(FASTVAR(xr)[i], fac);

	return 1;
}


static int quantize(int gain, struct granule_t *gi, int ch, int maxband)
{
	int i;
	fixed16 step;
	fixed13 x;
	int q;
	fixed13 qq0, q0 = qtab[0], q1 = qtab[1], q2 = qtab[2], q3 = qtab[3];
	short *ixi;
	fixed13 *xri;
	int max;

//{ static int counter; counter++; if (ch) fprintf(stderr, "\t\t"); fprintf(stderr, "%d (%d)\n", counter, gain); }

	max = maxband < maxs[ch] ? maxband : maxs[ch];

#ifdef PROF
static profile_data pd;
profile_start(&pd);
fprintf(stderr, "[ %3d ]  ", gain);
#endif

#if 0
	{
		static int need_init = 1;
		if (need_init) {
			int gain;

			need_init = 0;
			for (gain = 0; gain < 256; gain++) {
				gaintab[gain] = ftof16(pow2(-0.25f * (gain - 210.0f)) * 0.75f);
fprintf(stderr, "0x%08x, ", gaintab[gain]);
if((gain&3)==3)fprintf(stderr, "\n");
			}
		}
	}
#endif

	FASTVAR(limit_nonzero) = -max;
	FASTVAR(limit_big) = -max;

_F_	step = gaintab[gain];
				//qq0 = ((((long long)q0+1) << 19) - 1) / step;
				qq0 = qq0tab[gain];


	ixi = FASTVAR(ix) + max;
	xri = FASTVAR(xr) + max;
	i = -max;
_F_	while(i < 0) {	// PER: Changed loop structure to fix crash when called with maxband == 0
_F_		x = xri[i];
_F_
_F_		if (x <= qq0) {
_F_			ixi[i++] = 0;
_F_		} else {
_F_			x = fm16_16_13(x, step);
_F_			if (x <= q1) {
_F_				ixi[i++] = 1;
_F_				FASTVAR(limit_nonzero) = i;
_F_			} else if (x <= q2) {
_F_				ixi[i++] = 2;
_F_				FASTVAR(limit_big) = i;
_F_			} else if (x <= q3) {
_F_				ixi[i++] = 3;
_F_				FASTVAR(limit_big) = i;
_F_			} else {
_F_				for (q = 4; x > qtab[q]; q++) ;
_F_					if (q >= 8192) goto fail;
_F_				ixi[i++] = q;
_F_				FASTVAR(limit_big) = i;
_F_			}
_F_		}
_F_	}

	FASTVAR(limit_nonzero) += max;
	FASTVAR(limit_big) += max;
	if (FASTVAR(limit_big) > FASTVAR(limit_nonzero))
		FASTVAR(limit_nonzero) = FASTVAR(limit_big);

#ifdef PROF
profile_end(&pd, "quantize");
#endif

	return 0;

fail:

#ifdef PROF
profile_end(&pd, "quantize FAILED");
#endif

	return correct_scale(i + max, gi) ? quantize(gain, gi, ch, 576) : 1;  // 576 to be careful
}


static int quant_and_count(int gain, struct granule_t *gi, int ch, int maxband)
{
	int r;

	r = quantize(gain, gi, ch, maxband);

	if (r)
		return 100000;
	else
		return count_bits(gi);
}


int quant(int ch, int mean_bits, struct granule_t *gi)
{
	int min_gain = MIN_GAIN;
	int top, bot, next, last, prev;
	int bit = 0;
	int wil;
	fixed16 ff = 1 << 16;
	int hunt;
	int desired_rate = 0;

//fprintf(stderr, "----------------------------------------\n");
#ifdef PROF
fprintf(stderr, "----------------------------------------\n");
#endif

	last = next = FASTVAR(last_gain)[ch];
	if (last < min_gain) last = next = min_gain;

//fprintf(stderr, "START AT %d\n", last);
//fprintf(stderr, "%d ", last);
	if (quantize(last, gi, ch, 576)) {
		hunt = HC;
		desired_rate = reservoir_get(mean_bits, mean_bits / 2);
	} else {
		bit = count_bits(gi);
		wil = mean_bits / 2;

//fprintf(stderr, "GOT = %d\n", bit);
//fprintf(stderr, "%d ", bit);

if (ch && FASTVAR(mpeg).ms_stereo)
{
	fixed16 ratio;

	if (FASTVAR(eeb)[0] + FASTVAR(eeb)[1] >= 0x10000) {
		ratio = fd16(FASTVAR(eeb)[1] >> 16, (FASTVAR(eeb)[0] + FASTVAR(eeb)[1]) >> 16);
		ratio = fm16(ratio, ratio);
		ratio = fm16(ratio, 0x59999); // 5.6


if (ratio < 0x6666) ratio = 0x6666; // 0.4
if (ratio > 0x80000) ratio = 0x80000; // 8.0

		ff = fm16(ff, ratio);
	}
}


{
	int t;

	for (t = 1 ; t <= maxs[ch]; t++)
		if (FASTVAR(xr)[maxs[ch] - t])
			break;
	ff = fm16(ff, 0x10000 - (t << 6));
	ff = fm16(ff, ff); // better this way?
}


	wil = ((long long)wil * ff) >> 16;


		if (wil < FASTVAR(last_bits)[ch]) wil = (3 * wil + 7 * FASTVAR(last_bits)[ch]) / 10;

		desired_rate = reservoir_get(mean_bits, wil);



//fprintf(stderr, "%d => ", last);
//////////////if (FASTVAR(last_bits)[ch]) { last += ALPHA * (desired_rate - FASTVAR(last_bits)[ch]); if (last < min_gain) last = min_gain; if (last > MAX_GAIN) last = MAX_GAIN; next = last; }
//fprintf(stderr, "%d\n", last);

	
	
	
	
	
	
//fprintf(stderr, "bits was %d, now want %d\t", FASTVAR(last_bits)[ch], desired_rate);
		FASTVAR(last_bits)[ch] = desired_rate;

		if (bit > desired_rate)
			hunt = HA;
		else
			hunt = -HB;
	}

//fprintf(stderr, "WANT = %d\n", desired_rate);
//fprintf(stderr, "%d ", desired_rate);

//try to fixup
#if 0
if (bit > desired_rate || (CUT + 1) * bit <= CUT * desired_rate) {
	int delta;
	delta = (bit - desired_rate + 30) / 50;
fprintf(stderr, "*****  DELTA: %d\n", delta);
	if (delta && last + delta >= min_gain && last + delta <= MAX_GAIN) {
		last += delta;
fprintf(stderr, "*****  WILL TRY %d\n", last);
		bit = quant_and_count(last, gi, ch, delta > 0 ? FASTVAR(limit_nonzero) : 576);
		if (bit > desired_rate)
			hunt = 1;
		else
			hunt = -1;
		next = last;
	}
}
#endif

if (bit > desired_rate || (CUT + 1) * bit <= CUT * desired_rate) {

	if (hunt > 0) {
		do {
			prev = last;
			last = next;
			next = prev + hunt;
			if (next > MAX_GAIN) {
				next = MAX_GAIN;
				break;
			}
			bit = quant_and_count(next, gi, ch, FASTVAR(limit_nonzero));
			hunt *= 2;
		} while (bit > desired_rate);
		top = last;
		bot = next;
	} else {
		do {
			prev = last;
			last = next;
			next = prev + hunt;
			if (next < min_gain) {
				next = min_gain;
				break;
			}
			bit = quant_and_count(next, gi, ch, 576);
			hunt *= 2;
		} while ((CUT + 1) * bit < CUT * desired_rate);
		top = next;
		bot = last;
	}


	for (;;) {
	        last = next;
	        next = (top + bot + 1) >> 1;
		if (next == last)
			break;

		if (bit <= desired_rate && (CUT + 1) * bit > CUT * desired_rate)
			break;

		bit = quant_and_count(next, gi, ch, next > last ? FASTVAR(limit_nonzero) : 576);
	        if (bit > desired_rate) {
			top = next;
		} else
			bot = next;
	}

}

//{ static int last_des[2];
//	fprintf(stderr, "%d %d %d %d\n", desired_rate, last_des[ch], last, FASTVAR(last_gain)[ch]);
//	last_des[ch] = desired_rate;
//}

//fprintf(stderr, "last = %d, this = %d\n", FASTVAR(last_gain)[ch], last);
	FASTVAR(last_gain)[ch] = last;
	gi->global_gain = last;

//fprintf(stderr, "DID = %d\n", last);
//fprintf(stderr, "%d\n", last);

	return bit;
}

