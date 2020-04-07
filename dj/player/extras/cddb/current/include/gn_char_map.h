/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_translator.h>

/*
 * gn_char_map.h - The type definitions used by the gracenote 
 * application character mapper.
 */

#ifndef	_GN_CHAR_MAP_H_
#define _GN_CHAR_MAP_H_

#ifdef __cplusplus
extern "C" {
#endif

/* gn_map_chars
 *
 * Converts characters from the embedded database encoding (typically UTF-8)
 * to a platform-specific encoding. This function is typically called by the
 * embedded application after the raw XML has been parsed into a C struct,
 * and just before that data needs to be displayed. Because we do not want
 * to include a general purpose text encoding converter in an embedded system,
 * we leave it up to the system integrator or port engineer to implement this
 * function. Furthermore, since both the embedded database encoding and the
 * device display encoding are presumably known ahead of time, we do not need
 * to specify the input or output encoding in this function--they can be
 * reasonably hard-coded. In devices where this is inappropriate, the system
 * integrator or port engineer can extend the character converter API as
 * necessary.
 */
gn_error_t
gn_map_chars(gn_cstr_t src, gn_size_t src_len, gn_str_t* dest, gn_size_t* dest_len);

#ifdef __cplusplus
}
#endif

#endif
