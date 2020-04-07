/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_string.h - String Manipulation functions
 */

#ifndef	_GN_STRING_H_
#define _GN_STRING_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_memory.h>

#ifdef __cplusplus
extern "C"{
#endif 

 #define strlen   gn_strlen
 #define strdup   gn_strdupe
 #define strcpy   gn_strcpy
 #define strncpy  gn_strncpy
 #define strcat   gn_strcat
 #define strset   gn_strset
 #define strnset  gn_strnset
 #define strstr   gn_strstr
 #define stristr  gn_stristr
 #define strupr   gn_strupr
 #define strlwr   gn_strlwr
 #define strcmp   gn_strcmp
 #define stricmp  gn_stricmp
 #define strncmp  gn_strncmp
#if !defined(PLATFORM_WIN32)
#define strnicmp gn_strnicmp
#endif
 #define strtok   gn_strtok
 #define strchr   gn_strchr
 #define strrchr  gn_strrchr
#if !defined(PLATFORM_WIN32)
 #define strcmpi  gn_stricmp
#endif

 #define _strdup   gn_strdupe
 #define _strset   gn_strset
 #define _strnset  gn_strnset
 #define _strupr   gn_strupr
 #define _strlwr   gn_strlwr
 #define _stricmp  gn_stricmp
 #define _strnicmp gn_strnicmp
 
 #define memcpy     gnmem_memcpy
 #define memmove    gnmem_memmove
 #define memset     gnmem_memset
 #define memcmp     gnmem_memcmp

/*
 * Prototypes.
 */

/* Convert the character to its upper case */
gn_int16_t gn_toupper(gn_int16_t ch);

/* Convert the character to its lower case */
gn_int16_t gn_tolower(gn_int16_t ch);

/* Length of the string */
gn_size_t gn_strlen(gn_cstr_t string);

/* Convert the string to upper case */
gn_str_t gn_strupr(gn_str_t string);

/* Convert the string to lower case */
gn_str_t gn_strlwr(gn_str_t string);

/* Copy the string from source to destination */
gn_str_t gn_strcpy(gn_str_t to, gn_cstr_t from);

/* Copy the first n characters of source to destination */
gn_str_t gn_strncpy(gn_str_t to, gn_cstr_t from,gn_size_t count);

/* Concatenate the string */
gn_str_t gn_strcat(gn_str_t to, gn_cstr_t from);

/* Initialize characters of a string to a given format */
gn_str_t gn_strset(gn_str_t string,gn_int16_t val);

/* Set first n characters of string to specified characters */
gn_str_t gn_strnset(gn_str_t string,gn_int16_t val,gn_size_t count);

/* Duplicate the string */
gn_str_t gn_strdupe(gn_cstr_t string);

/* Finds the pattern in the string */
gn_str_t gn_strstr(gn_cstr_t string, gn_cstr_t pattern);

/* Finds the pattern in the string (case insensitive) */
gn_str_t gn_stristr(gn_cstr_t string, gn_cstr_t pattern);

/* Compare the strings */
gn_int16_t gn_strcmp(gn_cstr_t stringa, gn_cstr_t stringb);

/* Compare the strings (case insensitive) */
gn_int16_t gn_stricmp(gn_cstr_t stringa, gn_cstr_t stringb);

/* Compare the strings to specified size */
gn_int16_t gn_strncmp(gn_cstr_t stringa, gn_cstr_t stringb, gn_size_t size);

/* Compare the strings to specified size (case insensitive)*/
gn_int16_t gn_strnicmp(gn_cstr_t stringa, gn_cstr_t stringb, gn_size_t size);

/* Tokenise the string */
gn_str_t gn_strtok(gn_str_t string,gn_cstr_t token);

/* Find first occurrence of specified character in string */
gn_str_t gn_strchr(gn_cstr_t string, gn_int16_t c );

/* Find last occurrence of specified character in string */
gn_str_t gn_strrchr(gn_cstr_t string, gn_int16_t c );

#ifdef __cplusplus
}
#endif 

#endif /* _GN_FS_H_ */
