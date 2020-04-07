/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * hid.h - Simple hashing algorithm for Embedded Cddb Prototype.
 */

#include <extras/cddb/crossplatform.h>

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H
#include GN_STRING_H
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_utils.h>
#include "hid.h"
#include <extras/cddb/gn_memory.h>

/*
 * gen_hid()
 *
 * Description:
 *
 *	Convert a TOC to a hash value suitable for use as an index value.
 *
 * Args:
 *	offset: pointer to an array of TOC offsets in frames, with the lead-out
 *		at the end.
 *	noff: number of offsets in the TOC, including the lead-out.
 *
 * Returns:
 *	The new hash ID is returned in "hid".
 */


void
gen_hid(gn_uint32_t *offset, gn_uint32_t noff, hid_t *hid)
{
    gn_uint32_t i;
    gn_uint32_t n;
    gn_uint32_t os;
	gn_uint32_t temp;
	gn_uint32_t hash = 0;

	/* Initialize the hash and feed in the seed. */
    for (i = 0; i < noff; i++) {
		/* Hash the offset, 7 bits at a time. */
        for(os = offset[i], n = 0; os > 0; os >>= 7, n++) {
			hash = (hash << 4) + (os & 0x7F);
			temp = hash & 0xF0000000L;
			if (temp)
				hash ^= temp >> 24;
			hash &= ~temp;
		}
	}

	/* Reserve 0 & -1 for invalid hash values. */
	if (!hash)
		hash = 1;
	else if (hash == (unsigned long)-1)
		hash--;

	/* ?? byte-ordering?? - should we put in one byte at a time */
	gnmem_memcpy(hid, &hash, HIDSZ);
}


/*
 * hid2asc()
 *
 * Description:
 *	Convert a hid_t to an ASCII hex string.
 *
 * Arguments:
 *	mid: Pointer to the hid_t to be converted.
 *	asc: The buffer into which the string will be stored. Must be at
 *		least HIDASCSZ bytes in size. The string will not be
 *		null-terminated.
 *
 * Returns:
 *	None.
 */

void
hid2asc(hid_t *hid, gn_str_t asc)
{
	gn_uint32_t	i;
	gn_uchar_t*	p;
	gn_char_t	buf[HIDASCSZ + 1];

	p = (gn_uchar_t*)hid;

	for(i = 0; i < HIDSZ; i++) {
		gn_byte2hex(*p, &buf[i*2]);
		p++;
	}

	strncpy(asc, (gn_cstr_t)buf, HIDASCSZ);
}


/*
 * asc2hid()
 *
 * Description:
 *	Convert an ASCII hex hash ID string to a hid_t.
 *
 * Arguments:
 *	asc: Pointer to the ASCII hex string to be converted. Must be
 *		HIDASCSZ bytes in length, and need not be null-terminated.
 *	mid: Pointer to the hid_t to be filled.
 *
 * Returns:
 *	None.
 */

void
asc2hid(gn_str_t asc, hid_t *hid)
{
	gn_uint32_t		i;
	gn_uint32_t		x;
	gn_str_t		p;

	p = (gn_str_t)*hid;

	for(i = 0; i < HIDSZ; i++) {
		(void)sscanf(&asc[i * 2], "%02X", (int *)&x);
		p[i] = (gn_char_t)x;
	}
}
