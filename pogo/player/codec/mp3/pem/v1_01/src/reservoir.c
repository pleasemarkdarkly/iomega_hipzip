#define MAX_GRAN 2000
//#define MAX_GRAN 4095

#include "types.h"
//#include "layer3.h"
#include "reservoir.h"
#include "memory.h"


#include <stdio.h>


void reservoir_start(int frame_bits)
{
	if (frame_bits > 7680)
		FASTVAR(ResvMax) = 0;
	else
		FASTVAR(ResvMax) = 7680 - frame_bits;

	if (FASTVAR(ResvMax) > 4088)
		FASTVAR(ResvMax) = 4088;
}


int reservoir_get(int mean_bits, int wanted_bits)
{
	int bits, over_bits;

	bits = mean_bits * 50 / 100;

	if (wanted_bits < bits)
		bits = wanted_bits;

	if (bits > MAX_GRAN)
		bits = MAX_GRAN;
	if (FASTVAR(ResvMax) == 0)
		return bits < FASTVAR(ResvSize) - 74 ? bits : (FASTVAR(ResvSize) > 74 ? FASTVAR(ResvSize) - 74 : 0);


	over_bits = 0;
	if (wanted_bits > bits) {
		over_bits = wanted_bits - bits;
		if (over_bits > FASTVAR(ResvSize) * 7 / 10)
			over_bits = FASTVAR(ResvSize) * 7 / 10;
		if (over_bits > FASTVAR(ResvSize))
			over_bits = FASTVAR(ResvSize);
		bits += over_bits;
		if (bits > MAX_GRAN)
			bits = MAX_GRAN;
	}


	over_bits = FASTVAR(ResvSize) - (FASTVAR(ResvMax) * 8 / 10)    - over_bits;
	if (over_bits > 0              && wanted_bits >= 50)
		bits += over_bits;
	if (bits > MAX_GRAN)
		bits = MAX_GRAN;

	if (bits > FASTVAR(ResvSize) - 74) {
		bits = FASTVAR(ResvSize) - 74;
		if (bits < 0)
			bits = 0;
	}

	return bits;
}


void reservoir_add(int bits)
{
	FASTVAR(ResvSize) += bits;
}


void reservoir_put2(struct granule_t *gi)
{
	FASTVAR(ResvSize) -= gi->part2_length;
}


void reservoir_put3(struct granule_t *gi)
{
	FASTVAR(ResvSize) -= gi->part3_length;
}


int reservoir_end(void)
{
	int gr, ch, stuffingBits, stuff;
	int over_bits;

	over_bits = FASTVAR(ResvSize) - FASTVAR(ResvMax);
	if (over_bits < 0)
		over_bits = 0;

	FASTVAR(ResvSize) -= over_bits;
	stuffingBits = over_bits;

	if ((over_bits = FASTVAR(ResvSize) % 8)) {
		stuffingBits += over_bits;
		FASTVAR(ResvSize) -= over_bits;
	}

stuff = stuffingBits;

	for (gr = 1; gr < 2; gr++)
		for (ch = 1; ch < 2; ch++) {
			int extraBits, bitsThisGr;
			struct granule_t *gi = &FASTVAR(granule)[gr][ch];
			if (stuffingBits == 0)
				break;
			extraBits = 4095 - gi->part2_length - gi->part3_length;
			bitsThisGr = extraBits < stuffingBits ? extraBits : stuffingBits;
			gi->part3_length += bitsThisGr;
			stuffingBits -= bitsThisGr;
		}
return stuff;
}


void reservoir_init(void)
{
	FASTVAR(ResvSize) = 0;
}

