/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_ctype.c - Basic ctype operations for abstraction layer.
 *              The ARM compiler (V 1.0.1) has broken ctype functions.
 */

/*
 * Dependencies
 */

#include <extras/cddb/gn_ctype.h>

gn_size_t gn_isalnum(gn_size_t c)
{
	return(gn_isalpha(c) || gn_isdigit(c));
}

gn_size_t gn_isalpha(gn_size_t c)
{
	return(gn_isupper(c) || gn_islower(c));
}

gn_size_t gn_iscntrl(gn_size_t c)
{
	return(((c >= 0x00) && (c <= 0x2f)) || (c == 0x7f));
}

gn_size_t gn_isdigit(gn_size_t c)
{
	return((c >= '0') && (c <= '9'));
}

gn_size_t gn_isgraph(gn_size_t c)
{
	return((c >= 33) && (c <= 126));
}

gn_size_t gn_islower(gn_size_t c)
{
	return((c >= 'a') && (c <= 'z'));
}

gn_size_t gn_isprint(gn_size_t c)
{
	return((c >= 0x20) && (c <= 0x7e));
}

gn_size_t gn_ispunct(gn_size_t c)
{
	return(gn_isprint(c) && !gn_isspace(c) && !gn_isalpha(c) && !gn_isdigit(c));
}

gn_size_t gn_isspace(gn_size_t c)
{
	return(((c >= 9) && (c <= 13)) || (c == 32));
}

gn_size_t gn_isupper(gn_size_t c)
{
	return((c >= 'A') && (c <= 'Z'));
}

gn_size_t gn_isxdigit(gn_size_t c)
{
	return(gn_isdigit(c) || ((c >= 'a') && (c <='f')) || ((c >= 'A') && (c <= 'F')));
}
