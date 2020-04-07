/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_ctype.h - Character Manipulation functions
 */

#ifndef	_GN_CTYPE_H_
#define _GN_CTYPE_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>

#ifdef __cplusplus
extern "C"{
#endif 

#define isalnum		gn_isalnum
#define isalpha		gn_isalpha
#define iscntrl		gn_iscntrl
#define isdigit		gn_isdigit
#define isgraph		gn_isgraph
#define islower		gn_islower
#define isprint		gn_isprint
#define ispunct		gn_ispunct
#define isspace		gn_isspace
#define isupper		gn_isupper
#define isxdigit	gn_isxdigit

/*
 * Prototypes.
 */

gn_size_t gn_isalnum(gn_size_t c);
gn_size_t gn_isalpha(gn_size_t c);
gn_size_t gn_iscntrl(gn_size_t c);
gn_size_t gn_isdigit(gn_size_t c);
gn_size_t gn_isgraph(gn_size_t c);
gn_size_t gn_islower(gn_size_t c);
gn_size_t gn_isprint(gn_size_t c);
gn_size_t gn_ispunct(gn_size_t c);
gn_size_t gn_isspace(gn_size_t c);
gn_size_t gn_isupper(gn_size_t c);
gn_size_t gn_isxdigit(gn_size_t c);


#ifdef __cplusplus
}
#endif 

#endif /* _GN_CTYPE_H_ */
