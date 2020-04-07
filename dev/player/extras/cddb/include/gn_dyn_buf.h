#ifndef GN_DYN_BUF_H
#define GN_DYN_BUF_H

/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * Gracenote Embedded Database Dynamic Buffer Package
 *
 * This file declares the interface for a dynamic buffer object,
 * which is a memory-based object to which you can append data.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Typedefs
 */

typedef gn_error_t gn_dyn_buf_error_t;
typedef struct gn_dyn_buf_t* gn_dyn_buf_ref_t;


/*
 * Prototypes
 */

/* gn_dyn_buf_create
 *
 * Creates a dynamic buffer, preallocating initial_size bytes for
 * subsequent storage, which will grow by grow_by_size bytes as
 * needed.
 */
gn_dyn_buf_ref_t
gn_dyn_buf_create(gn_uint32_t initial_size, gn_uint32_t grow_by_size);

/* gn_dyn_buf_clone
 *
 * Duplicates the given dynamic buffer.
 */
gn_dyn_buf_ref_t
gn_dyn_buf_clone(gn_dyn_buf_ref_t dyn_buf);

/* gn_dyn_buf_dispose
 *
 * Releases all the memory used by the given dynamic buffer, both
 * the data buffer itself, and the object structure.
 */
void
gn_dyn_buf_dispose(gn_dyn_buf_ref_t dyn_buf);

/* gn_dyn_buf_dispose_and_return_buffer
 *
 * Releases the memory used by the given dynamic buffer object, but
 * returns a pointer the object's data buffer. You should call
 * gn_dyn_buf_get_length first to determine the length of the buffer.
 * You must free the buffer by calling gnmem_free.
 */
gn_str_t
gn_dyn_buf_dispose_and_return_buffer(gn_dyn_buf_ref_t dyn_buf);

/* gn_dyn_buf_append_buf
 *
 * Appends the given buffer to the end of the given dynamic buffer.
 */
gn_dyn_buf_error_t
gn_dyn_buf_append_buf(gn_dyn_buf_ref_t dyn_buf, gn_cstr_t buf, gn_uint32_t buf_len);

/* gn_dyn_buf_append_str
 *
 * Appends the given C string to the end of the given dynamic buffer.
 */
gn_dyn_buf_error_t
gn_dyn_buf_append_str(gn_dyn_buf_ref_t dyn_buf, gn_cstr_t str);

/* gn_dyn_buf_compact
 *
 * Releases padding beyond the end of a dynamic buffer. To prevent
 * allocations every time you append to a dynamic buffer, extra space
 * is kept beyond the end of a dynamic buffer's buffer. This function
 * removes that extra space.
 */
gn_dyn_buf_error_t
gn_dyn_buf_compact(gn_dyn_buf_ref_t dyn_buf);

/* gn_dyn_buf_get_data
 *
 * Returns a pointer to the beginning of the data that has been
 * written to the given dynamic buffer.
 */
gn_cstr_t
gn_dyn_buf_get_data(gn_dyn_buf_ref_t dyn_buf);

/* gn_dyn_buf_get_length
 *
 * Returns the length of the data that has been written to the given
 * dynamic buffer.
 */
gn_uint32_t
gn_dyn_buf_get_length(gn_dyn_buf_ref_t dyn_buf);


#ifdef __cplusplus
}
#endif

#endif
