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

#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_memory.h>
#include GN_STRING_H
#include <extras/cddb/gn_fs.h>

gn_str_t
gn_strdup(gn_cstr_t str)
{
	gn_str_t copy = 0;

	if (str == 0) {
		return 0;
	}

	copy = (gn_str_t)gnmem_malloc(strlen(str) + 1);

	if (copy != 0) {
		strcpy(copy, str);
	}

	return copy;
}

gn_str_t
gn_bufdup(gn_cstr_t buf, gn_size_t buf_len)
{
	gn_str_t copy = 0;

	if (buf == 0) {
		return 0;
	}

	copy = (gn_str_t)gnmem_malloc(buf_len);

	if (copy != 0) {
		gnmem_memcpy((void*)copy, (void*)buf, buf_len);
	}

	return copy;
}

gn_error_t
gn_copy_to_expandable_buffer(gn_cstr_t src, gn_size_t src_len,
                             gn_str_t* dest, gn_size_t* dest_len)
{
	gn_error_t error = 0;

	*dest = (*dest == 0)
	      ? gnmem_malloc(src_len)
	      : gnmem_realloc(*dest, *dest_len + src_len);

	if (*dest != 0)
	{
		gnmem_memcpy(*dest + *dest_len, (void*)src, src_len);
		*dest_len += src_len;
	}
	else
		error = GNERR_NoMemory;

	return GNERR_ERR_CODE(error);
}

gn_str_t
gn_makestr(gn_cstr_t buf, gn_size_t buf_len)
{
	gn_str_t copy = 0;

	if (buf == 0) {
		return 0;
	}

	copy = (gn_str_t)gnmem_malloc(buf_len + 1);

	if (copy != 0) {
		gnmem_memcpy((void*)copy, (void*)buf, buf_len);
		copy[buf_len] = 0;
	}

	return copy;
}

gn_error_t
gn_set_str(gn_cstr_t* str, gn_cstr_t new_value)
{
	if (str == 0) {
		return GNERR_ERR_CODE(UTILERR_InvalidArg);
	}

	if (*str != 0) {
		gnmem_free((void*)*str);
		*str = 0;
	}

	if (new_value != 0) {

		*str = gn_strdup(new_value);

		if (*str == 0) {
			return GNERR_ERR_CODE(UTILERR_NoMemory);
		}
	}

	return GNERR_ERR_CODE(UTILERR_NoError);
}

gn_bool_t
gn_str_eq(gn_cstr_t a, gn_cstr_t b)
{
	if (a == 0 || b == 0) {
		return GN_FALSE;
	}

	return strcmp(a, b) == 0;
}

gn_int32_t
gn_bufcmp(gn_cstr_t a, gn_size_t a_len, gn_cstr_t b, gn_size_t b_len)
{
	gn_size_t n = a_len < b_len ? a_len : b_len;
	gn_size_t i;

	for (i = 0; i < n; i++) {
		if (a[i] < b[i]) {
			return -1;
		} else if (a[i] > b[i]) {
			return +1;
		}
	}

	if (a_len < b_len) {
		return -1;
	} else if (a_len > b_len) {
		return +1;
	}

	return 0;
}

/* gn_byte2hex
 *
 * Utility routines for converting single byte to hexadecimal ascii representation.
 */

void
gn_byte2hex(gn_uchar_t byte, gn_str_t buffer)
{
	gn_uchar_t		nib1, nib2;

	nib1 = (byte & 0xF0) >> 4;
	nib2 = (byte & 0xF);
	if (nib1 < 10)
		nib1 += '0';
	else {
		nib1 -= 10;
		nib1 += 'A';
	}
	if (nib2 < 10)
		nib2 += '0';
	else {
		nib2 -= 10;
		nib2 += 'A';
	}
	*buffer++ = nib1;
	*buffer = nib2;
}

gn_uint32_t
parse_digits_to_uint32(gn_cstr_t buf, gn_size_t len, gn_bool_t is_hex)
{
	gn_uint32_t value = 0;
	gn_size_t i = 0;

	for (i = 0; i < len; i++) {

		char ch = buf[i];

		value *= is_hex ? 16 : 10;

		if (ch >= '0' && ch <= '9') {

			value += ch - '0';

		} else if (is_hex) {

			if (ch >= 'A' && ch <= 'F')
				value += ch - 'A' + 10;
			else if (ch >= 'a' && ch <= 'f')
				value += ch - 'a' + 10;
		}
	}

	return value;
}


gn_cstr_t
parse_uint32_to_buff(gn_uint32_t num, gn_str_t buf, gn_size_t len, gn_uint32_t radix)
{
	gn_uint32_t		temp = num;
	gn_int32_t		cnt = 0;
	gn_uchar_t		c;

	*buf = 0;

	/* calculate size of string */
	while (temp) {
		temp /= radix;
		cnt++;
	}

	temp = num;
	if (cnt > (gn_int32_t)(len + 1))
		return buf;

	*(buf + cnt--) = 0;
	while (cnt >= 0) {
		c = (gn_uchar_t)(temp % radix);
		if (c < 10)
			c += '0';
		else {
			c += 'A' - (c - 10);
		}
		temp /= radix;
		*(buf + cnt) = c;
		cnt--;
		if (temp == 0)
			break;
	}

	return buf;
}


gn_int32_t
parse_buf_to_int32(gn_cstr_t buf, gn_size_t len)
{
	gn_size_t index = 0;
	gn_bool_t is_negative = GN_FALSE;
	gn_bool_t is_hex = GN_FALSE;
	gn_int32_t value = 0;

	if (index < len) {
		char ch = buf[index];

		if (ch == '-') {
			is_negative = GN_TRUE;
			index++;
		} else if (ch == '+') {
			index++;
		} else if (ch == '0' && (index + 1) < len && buf[index + 1] == 'x') {
			is_hex = GN_TRUE;
			index += 2;
		}
	}

	value = parse_digits_to_uint32(buf + index, len - index, is_hex);

	if (is_negative) {
		value = -value;
	}

	return value;
}

gn_size_t
write_UTF8(gn_uint32_t value, unsigned char* buf)
{
	if (buf != 0) {

		unsigned char* orig_buf = buf;
		int count = 0;

		/* do quick processing for 7 bit characters */
		if (value < 0x00000080) {
			*buf = (unsigned char)value;
			return 1;
		}
		
		/* write the header byte */
		if (value < 0x00000800) {
			*buf++ = (unsigned char)((value >> 6) | 0xC0);
			count = 1;
		} else if (value < 0x00010000) {
			*buf++ = (unsigned char)((value >> 12) | 0xE0);
			count = 2;
		} else if (value < 0x00200000) {
			*buf++ = (unsigned char)((value >> 18) | 0xF0);
			count = 3;
		} else if (value < 0x04000000) {
			*buf++ = (unsigned char)((value >> 24) | 0xF8);
			count = 4;
		} else if (value < 0x80000000) {
			*buf++ = (unsigned char)((value >> 30) | 0xFC);
			count = 5;
		} else {
			return 0;
		}
		
		/* write the rest of the bytes */
		while (--count >= 0) {
			*buf++ = (unsigned char)(((value >> (6 * count)) & 0x3F) | 0x80);
		}

		/* return the number of characters written */
		return buf - orig_buf;

	} else {

		/* just return how many characters we would write if we had a buffer */

		if (value < 0x00000080) {
			return 1;
		} else if (value < 0x00000800) {
			return 2;
		} else if (value < 0x00010000) {
			return 3;
		} else if (value < 0x00200000) {
			return 4;
		} else if (value < 0x04000000) {
			return 5;
		} else if (value < 0x80000000) {
			return 6;
		} else {
			return 0;
		}

	}
}

void
gn_smart_free(void** p)
{
	if (p && *p)
	{
		gnmem_free(*p);
		*p = 0;
	}
}

gn_cstr_t
get_filename_from_url(gn_cstr_t url)
{
	if (strstr(url, "http://") == url || strstr(url, "ftp://") == url) {
		gn_cstr_t slash = strrchr(url, '/');
		return slash + 1;
	} else {
		return url;
	}
}

gn_error_t
create_unique_filename(gn_cstr_t base_name, gn_cstr_t* unique_name)
{
	int count = 0;
	gn_str_t candidate_name = 0;
	int root_len = 0;
	int ext_len = 0;
	gn_cstr_t ext_str = 0;

	if (base_name == 0 || unique_name == 0) {
		return GNERR_ERR_CODE(UTILERR_InvalidArg);
	}

	*unique_name = 0;

	/* figure out where the extenstion begins */
	ext_str = strrchr(base_name, '.');

	/* if there is no extension, pretend it is an empty string at the end of base_name */
	if (ext_str == 0) {
		ext_str = base_name + strlen(base_name);
	}

	/* get the lengths of the root name and the extension */
	root_len = ext_str - base_name;
	ext_len = strlen(ext_str);

	/* create an initial candidate name */
	candidate_name = gn_strdup(base_name);

	if (candidate_name == 0) {
		return GNERR_ERR_CODE(UTILERR_NoMemory);
	}

	/* while the candidate filename exists, keep coming up with new candidate names */
	/* TBD REC: Potential, but unlikely, endless loop. */
	while (gnfs_exists(candidate_name)) {

		char count_str[16];

		gnmem_free((void*)candidate_name);

		sprintf(count_str, "%d", ++count);

		candidate_name = gnmem_malloc(root_len + strlen(count_str) + ext_len + 1);

		if (candidate_name == 0) {
			return GNERR_ERR_CODE(UTILERR_NoMemory);
		}

		strncpy(candidate_name, base_name, root_len);
		strcpy(candidate_name + root_len, count_str);
		strcat(candidate_name, ext_str);
	}

	*unique_name = candidate_name;

	return GNERR_ERR_CODE(UTILERR_NoError);
}
