/*
 *   base64 - base64 encoding/decoding
 *
 *   Copyright (C) 1996-1998  CDDB Inc. All rights reserved.
 *   Author: Steve Scherf
 *   Email: steve@cddb.com
 *
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H
#include GN_STDLIB_H
#include <extras/cddb/port.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_defines.h>


/* Global variables. */

const i32_t b2amap[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 52, 53, 54,
	55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3,
	4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20,
	21, 22, 23, 24, 25, -1, -1, -1, -1, -1, -1, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
	49, 50, 51, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};


const i8_t a2bmap[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N',
	'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b',
	'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
	'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3',
	'4', '5', '6', '7', '8', '9', '+', '/'
};


/* Prototypes. */

void out_octet(i32_t, i32_t, ui8_t **);


i32_t
base64_decode(const ui8_t *buf, i32_t len, ui8_t **nbuf, i32_t *nlen, i32_t null)
{
	i32_t i;
	i32_t m;
	i32_t r;
	i32_t val;
	i32_t size;
	ui8_t *obuf;
	ui8_t *p;

	size = len * 4 / 3;
        if((len * 4) % 3)
                size += 1;
	if(null)
		size += 1;

	obuf = (ui8_t *)gnmem_malloc(size);

	if(obuf == NULL)
		return -3;

	r = 0;
	val = 0;
	p = obuf;

	for(i = 0; i < len; i++, buf++) {
		/* Special end-processing. */
		if(*buf == '=') {
			switch(r) {
			case 0:
			case 1:
				/* Input is corrupt. */
				gnmem_free(obuf);
				return -1;

			case 2:
			case 3:
				out_octet(val, (r - 1), &p);
				i = len;
				continue;

			default:
				/* Internal error. */
				gnmem_free(obuf);
				return -2;
			}
		}

		m = b2amap[(i32_t)*buf];

		/* Check for invalid character. */
		if(m == -1) {
			gnmem_free(obuf);
			return -1;
		}

		val |= (m << ((3 - r) * 6));

		if(r == 3) {
			out_octet(val, 3, &p);
			r = 0;
			val = 0;
		}
		else
			r++;
	}

	if(null) {
		*p = '\0';
		p++;
	}

	*nbuf = obuf;
	*nlen = p - obuf;

	return 0;
}



void
out_octet(i32_t val, i32_t cnt, ui8_t **p)
{
	i32_t i;

	for(i = 0; i < cnt; i++, (*p)++)
		**p = (val >> ((2 - i) * 8)) & 0xFF;
}


i32_t
base64_encode(const ui8_t *buf, i32_t len, ui8_t **nbuf, i32_t *nlen, i32_t null)
{
	i32_t i;
	i32_t j;
	i32_t k;
	i32_t r;
	i32_t val;
	i32_t size;
	ui8_t *obuf;
	ui8_t *p;
	
	size = (len * 4) / 3;
	if((len * 4) % 3)
		size += 1;
	if(len % 3)
		size += 3 - (len % 3);
	if(null)
		size += 1;

	obuf = (ui8_t *)gnmem_malloc(size);
	if(obuf == NULL)
		return -3;

	r = 0;
	val = 0;
	p = obuf;

	for(i = 0; i < len; i++, buf++) {
		val |= (*buf << ((2 - r) * 8));

		r++;

		if((r == 3) || ((i + 1) == len)) {
			k = (r * 4) / 3;
			if((r * 4) % 3)
				k++;

			for(j = 0; j < k; j++, p++)
				*p = a2bmap[(val >> ((3 - j) * 6)) & 0x3F];

			if(r == 3)
				r = 0;

			val = 0;
		}
	}

	if(r % 3)
		for(i = 0; i < (3 - r); i++, p++)
			*p = '=';

	if(null) {
		*p = '\0';
		p++;
	}

	*nlen = p - obuf;
	*nbuf = obuf;

	return 0;
}
