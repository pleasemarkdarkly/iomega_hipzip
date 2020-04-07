/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * validate.c - Main for alignment/data size validation
 */

#include "validate_alignment.h"

int main(int argc, char **argv)
{
	print_alignment_header();
	validate_alignment();
	validate_sizes();
	validate_endian();

	return(0);
}
