//#define PROF
#ifdef PROF
#include "TinyProfile.h"
#endif

#include <stdio.h>
#include <math.h>

#include "types.h"
#include "psy.h"
#include "quant.h"
#include "reservoir.h"
#include "memory.h"


void init_code(void)
{
	init_quant();
}

/* Makes all values in xr positive, saving the sign bits in sxs as
 * a packed boolean array (per) */
// FASTVAR: xr, sxs
static void make_sign(int ch)
{
	int i, j;
	unsigned int s;
	fixed16 *xx;
	fixed16 x;

	xx = FASTVAR(xr);
	for (i = 0; i < (maxs[ch] + 31) / 32; i++) {
		s = 1;
		for (j = 0; j < 32; j++) {
			x = *xx++;
			if (x < 0) {
				s = 0x80000000 | (s >> 1);
				xx[-1] = -x;
			} else
				s >>= 1;
		}
		FASTVAR(sxs)[i] = s;
	}
}

// FASTVAR: xr
static int any_xr_nonzero(int ch)
{
	int i;

	for (i = 0; i < maxs[ch]; i++)
		if (FASTVAR(xr)[i]) return 1;
	return 0;
}


void code(int ch, struct granule_t *gi, int mean_bits) 
{
	gi->part2_length      = 0;
	gi->part3_length      = 0;
	//gi->big_values        = 0;
	//gi->count1            = 0;
	gi->subblock_gain[0]  = 0;
	gi->subblock_gain[1]  = 0;
	gi->subblock_gain[2]  = 0;
	gi->table_select[0]   = 0;
	gi->table_select[1]   = 0;
	gi->table_select[2]   = 0;
	gi->region0_count     = 0;
	gi->region1_count     = 0;
	gi->global_gain       = 0;
	gi->count1table_select= 0;
	gi->block_type        = 0;
//	gi->block_type        = 2; // even pesten. huhuhuh.
	gi->pos0 = 0;
	gi->pos1 = 0;
	gi->pos2 = 0;
	gi->pos3 = 0;


	make_sign(ch);
	start_scalefacs(gi);	// zeros out sf and scalefacs
	if (any_xr_nonzero(ch)) {
		calc_scalefacs(ch, gi);
		compute_scalefacs(ch, gi);
		apply_scalefacs(ch, gi);
		gi->part3_length = quant(ch, mean_bits, gi);
		reservoir_put2(gi);
	}
	reservoir_put3(gi);
	reservoir_add(mean_bits / 2);
}


