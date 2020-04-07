#ifndef GN_UTILS_H
#define GN_UTILS_H

/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * Gracenote Embedded Database Utility Functions
 *
 * This file declares useful routines that are written against the
 * abstract layer and whose source can be distributed to customers.
 *
 * If a routine is useful across several other source files and is
 * not big enough to deserve a file of its own, put it here. If you
 * begin to see a lot of related routines appearing here, consider
 * moving them to their own file.
 */

/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Prototypes
 */

/* gn_strdup
 *
 * Duplicates a C string, using gnmem_malloc to do the allocation.
 * Returns a pointer to a copy of the given string or null if the
 * allocation failed or if the given string is null.
 */
gn_str_t
gn_strdup(gn_cstr_t str);

/* gn_bufdup
 *
 * Duplicates a buffer, using gnmem_malloc to do the allocation.
 * Returns a pointer to a copy of the given buffer or null if the
 * allocation failed or if the given buffer is null.
 */
gn_str_t
gn_bufdup(gn_cstr_t buf, gn_size_t buf_len);

/* gn_copy_to_expandable_buffer
 *
 * Used for appending src_len bytes of data at src to a buffer. 
 * The dest and dest_len parameters are assumed to be valid or 
 * null. If they are valid, *dest points to a buffer containing
 * *dest_len bytes, and *dest can be safely passed to gnmem_realloc.
 * If they are null, then this function will allocate a buffer
 * to hold src_len bytes of data, copy the data, and save the 
 * address of it in *dest. This function performs the same thing
 * as a gn_dyn_buf, but it does an allocation every time you add
 * data to it.
 */
gn_error_t
gn_copy_to_expandable_buffer(gn_cstr_t src, gn_size_t src_len,
                             gn_str_t* dest, gn_size_t* dest_len);

/* gn_makestr
 *
 * Creates a C string, using gnmem_malloc to do the allocation,
 * and copying the data in the given buffer to the string. Returns
 * a pointer to the new string, or null if the allocation failed.
 */
gn_str_t
gn_makestr(gn_cstr_t buf, gn_size_t buf_len);

/* gn_set_str
 *
 * Copies a C string into an existing pointer variable, replacing
 * any string that may currently be there. Uses gnmem_free to free
 * the existing value and uses gnmem_malloc to allocate the copy.
 * If you pass null for new_value, any contents of *str are freed
 * and *str is set to null.
 */
gn_error_t
gn_set_str(gn_cstr_t* str, gn_cstr_t new_value);

/* gn_str_eq
 *
 * Returns GN_TRUE if strings a and b are equal, using a case-
 * sensitive comparison. If strings a or b are null, returns GN_FALSE.
 */
gn_bool_t
gn_str_eq(gn_cstr_t a, gn_cstr_t b);

/* gn_bufcmp
 *
 * Compares two ranges of memory. Returns < 0 if a < b, 0 if a == b
 * and a_len == b_len, or > 0 otherwise.
 */
gn_int32_t
gn_bufcmp(gn_cstr_t a, gn_size_t a_len, gn_cstr_t b, gn_size_t b_len);

/* gnfs_readln
 *
 * Read a single line from the passed file, null-terminating and
 * leaving the file pointer in the right place.
 */
gn_str_t
gnfs_readln(gn_handle_t handle, gn_str_t buffer, gn_size_t size);

/* gnfs_gets
 *
 * Read a single line from the passed file, null-terminating,
 * leaving the newline at the end, and leaving the file pointer
 * in the correct place.
 */
gn_str_t gnfs_gets(gn_handle_t handle, gn_char_t* buffer, gn_size_t size);

/* base64
 *
 * Utility routines for decoding and encoding from and to base64.
 */
gn_int32_t base64_decode(const gn_uchar_t *, gn_int32_t , gn_uchar_t **, gn_int32_t *, gn_int32_t );
gn_int32_t base64_encode(const gn_uchar_t *, gn_int32_t , gn_uchar_t **, gn_int32_t *, gn_int32_t );

/* gn_byte2hex
 *
 * Utility routines for converting single byte to hexadecimal ascii representation.
 */
void
gn_byte2hex(gn_uchar_t byte, gn_str_t buffer);

/* parse_digits_to_uint32
 *
 * Parses numeric characters to an unsigned integer. The string can look
 * like any of these: 123, 1A2B3C. If the characters are for a hex number,
 * you must set is_hex. Characters encountered that are not in [0-9] or,
 * if is_hex, [a-f] and [A-F] are ignored.
 */
gn_uint32_t
parse_digits_to_uint32(gn_cstr_t buf, gn_size_t len, gn_bool_t is_hex);

/* parse_uint32_to_buff
 *
 * Render unsigned integer into ASCII form, depending upon specified radix.
 * 
 * Returns pointer to passed buf, for convenience
 */
gn_cstr_t
parse_uint32_to_buff(gn_uint32_t num, gn_str_t buf, gn_size_t len, gn_uint32_t radix);

/* parse_buf_to_int32
 *
 * Parses numeric characters to an integer. The string can look like
 * any of these: 123, +123, -123, 0xABC, 0xabc. Characters encountered
 * that are not in [0-9] are ignored. Also, if the string starts with
 * "0x", characters in [a-f] and [A-F] are ignored as well.
 */
gn_int32_t
parse_buf_to_int32(gn_cstr_t buf, gn_size_t len);

/* write_UTF8
 *
 * Writes the given value to buf in UTF-8 fashion. Since it does not
 * check the length of the buffer, you must ensure there is room.
 * Returns the number of characters written to buf. If you pass null
 * for buf, no characters are actually written, but it still returns
 * the number of characters that would be written.
 */
gn_size_t
write_UTF8(gn_uint32_t value, unsigned char* buf);

/* gn_smart_free
 *
 * If p and *p are not null, calls gnmem_free on *p and sets *p to null.
 * Useful for freeing a pointer and setting its value to null in one step.
 */
void gn_smart_free(void** p);

/* get_filename_from_url
 *
 * Attempts to extract the filename from a URL. If the given URL begins 
 * with "http://" or "ftp://", this functions returns a pointer to the
 * character in the URL after the final '/' character. This might end up 
 * being the terminating null. If the URL string does not begin with 
 * "http://" or "ftp://", this function simply returns its argument.
 */
gn_cstr_t
get_filename_from_url(gn_cstr_t url);

/* create_unique_filename
 *
 * Creates a unique filename based on base_name. If base_name is foo.txt
 * and a file called foo.txt exists, then this function will return a new
 * string called foo1.txt. If foo1.txt exists also, it will return foo2.txt,
 * etc. If base_name does not have an extension, the digit is appended to
 * then end of the name: foo -> foo1, foo2, etc. Space for the unique 
 * filename is allocated by this function, and it is the caller's
 * responsibility to release that memory.
 */
gn_error_t
create_unique_filename(gn_cstr_t base_name, gn_cstr_t* unique_name);

#ifdef __cplusplus
}
#endif

#endif
