/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_defines.h - The type definitions used by the gracenote appl 
 * and the definition of macros.
 */

#ifndef	_GN_DEFINES_H_
#define _GN_DEFINES_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDDEF_H

#ifdef _BLD_TH_
#include <extras/cddb/gn_thl.h>
#endif

#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

#define FAILURE (1)
#define SUCCESS (0)

#define GN_FALSE (0)
#define GN_TRUE (!GN_FALSE)

#define GN_NULL (0)

/*
 * Typedef
 */

typedef char gn_char_t;
typedef short gn_int16_t;
typedef long gn_int32_t;
typedef unsigned short gn_unichar_t;

typedef unsigned char gn_uchar_t;
typedef unsigned short gn_uint16_t;
typedef unsigned long gn_uint32_t;

typedef size_t gn_size_t;

typedef char* gn_str_t;
typedef const char*  gn_cstr_t;
typedef gn_unichar_t* gn_unistr_t;
typedef const gn_unichar_t* gn_cunistr_t;

typedef int gn_handle_t;

typedef float gn_flt32_t;
typedef double gn_flt64_t;

typedef char gn_bool_t;
typedef gn_uint32_t gn_error_t;

#ifndef LONGLONG_TYPE

typedef struct gn_uint64
{
	gn_uint32_t	low_part;
	gn_uint32_t	high_part;
}
gn_uint64_t;
#define LONGLONG_TYPE

#endif

#ifdef __cplusplus
}
#endif 

#endif /* _GN_DEFINES_H_ */
