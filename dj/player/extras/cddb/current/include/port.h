/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/* Typedefs for portability. Change these as needed for your architecture. */

#ifndef	_PORT_H_
#define _PORT_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>

#ifdef __cplusplus
extern "C"{
#endif 

typedef gn_char_t		i8_t;		/* Signed 8-bit integer. */
typedef gn_uchar_t		ui8_t;		/* Unsigned 8-bit integer. */
typedef gn_int16_t		i16_t;		/* Signed 16-bit integer. */
typedef gn_uint16_t		ui16_t;		/* Unsigned 16-bit integer. */
typedef gn_int32_t		i32_t;		/* Signed 32-bit integer. */
typedef gn_uint32_t		ui32_t;		/* Unsigned 32-bit integer. */

typedef gn_size_t sz_t;	/* Generic size type */

#ifdef __cplusplus
}
#endif 

#endif  /* _PORT_H_ */
