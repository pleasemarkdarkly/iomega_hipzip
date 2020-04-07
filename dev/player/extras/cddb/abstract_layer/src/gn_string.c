/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_string.c - Basic string operations for abstraction layer.
 */

/*
 * Dependencies
 */

#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_string.h>

/* Convert the character to its upper case */
gn_int16_t gn_toupper(gn_int16_t ch)
{
	if((ch >= 'a') && (ch <= 'z'))
		return ch-32;

	return ch;
}

/* Convert the character to its lower case */
gn_int16_t gn_tolower(gn_int16_t ch)
{
	if((ch >= 'A') && (ch <= 'Z'))
		return ch+32;

	return ch;
}

/* Length of the string */
gn_size_t gn_strlen(gn_cstr_t string)
{
	gn_size_t len=0;
	while(string[len])
		len++;
	return len;
}

/* Convert the string to upper case */
gn_str_t gn_strupr(gn_str_t string)
{
	gn_str_t s;
	
	if(string) {
		for (s = string; *s; ++s)
			*s = (gn_char_t) gn_toupper(*s);
	}
	return string;
}


/* Convert the string to lower case */
gn_str_t gn_strlwr(gn_str_t string)
{
	gn_str_t s;
	
	if(string) {
		for (s = string; *s; ++s)
			*s = (gn_char_t) gn_tolower(*s);
	}
	return string;
}

/* Copy the string from source to destination */
gn_str_t gn_strcpy(gn_str_t to, gn_cstr_t from)
{
    gnmem_memmove(to, (gn_str_t) from, 1+gn_strlen(from));
    return to;
}

/* Copy first n characters from source to destination */
gn_str_t gn_strncpy(gn_str_t to, gn_cstr_t from,gn_size_t count)
{
	gn_size_t len_copy;

	if(count >= (gn_strlen(from)+1)) {
		len_copy = gn_strlen(from)+1;
	}
	else {
		len_copy = count;
	}

    gnmem_memmove(to, (gn_str_t) from, len_copy);
    return to;
}

/* Concatenate the string */
gn_str_t gn_strcat(gn_str_t to, gn_cstr_t from)
{
    gn_strcpy(to + gn_strlen(to), from);
    return to;
}

/* Initialize characters of a string to a given format */
gn_str_t gn_strset(gn_str_t string,gn_int16_t val)
{
	gnmem_memset(string,val,gn_strlen(string));
	return string;
}


/* Set first n characters of string to specified charactert */
gn_str_t gn_strnset(gn_str_t string,gn_int16_t val,gn_size_t count)
{
	gnmem_memset(string,val,(count < gn_strlen(string))?count:gn_strlen(string));
	return string;
}

/* Duplicate the string */
gn_str_t gn_strdupe(gn_cstr_t string)
{
	gn_str_t newstring;

	if (string)	{
		if (NULL != (newstring = gnmem_malloc(gn_strlen(string) + 1)))
			gn_strcpy(newstring, string);
		return newstring;
	}
	else  
		return NULL;
}


/* Finds the pattern in the string */
gn_str_t gn_strstr(gn_cstr_t string, gn_cstr_t pattern)
{
	gn_str_t pptr, sptr, start;
	gn_uint32_t  slen, plen;

    for (start = (gn_str_t)string,
		pptr  = (gn_str_t)pattern,
		slen  = gn_strlen(string),
		plen  = gn_strlen(pattern);
        /* while string length not shorter than pattern length */
		slen >= plen;
		start++, slen--) {
         
		/* find start of pattern in string */
        while (*start != *pattern) {
			start++;
			slen--;

           /* if pattern longer than string */

             if (slen < plen)
				 return(NULL);
		}

        sptr = start;
        pptr = (gn_str_t)pattern;

		while (*sptr == *pptr) {
			sptr++;
			pptr++;
			
			/* if end of pattern then pattern was found */
			
			if ('\0' == *pptr)
				return (start);
		}
	}
    return(NULL);
}


/* Finds the pattern in the string (case insensitive) */
gn_str_t gn_stristr(gn_cstr_t string, gn_cstr_t pattern)
{
	gn_str_t pptr, sptr, start;
	gn_uint32_t  slen, plen;

    for (start = (gn_str_t)string,
		pptr  = (gn_str_t)pattern,
		slen  = gn_strlen(string),
		plen  = gn_strlen(pattern);
        /* while string length not shorter than pattern length */
		slen >= plen;
		start++, slen--) {
         
		/* find start of pattern in string */
        while (gn_toupper(*start) != gn_toupper(*pattern)) {
			start++;
			slen--;

           /* if pattern longer than string */

             if (slen < plen)
				 return(NULL);
		}

        sptr = start;
        pptr = (gn_str_t)pattern;

		while (gn_toupper(*sptr) == gn_toupper(*pptr)) {
			sptr++;
			pptr++;
			
			/* if end of pattern then pattern was found */
			
			if ('\0' == *pptr)
				return (start);
		}
	}
    return(NULL);
}

/* Compare the strings */
gn_int16_t gn_strcmp(gn_cstr_t stringa, gn_cstr_t stringb)
{
	gn_int32_t i = 0;

	while (('\0' != stringa[i]) && ('\0' != stringb[i])) {
		if (stringa[i] < stringb[i])
			return -1;        /* Char from first string is first  */
		
		if (stringa[i] > stringb[i])
			return 1;         /* Char from second string is first */
		
		i++;                    /* Next char                        */
	}
	
	if (('\0' == stringa[i] ) && ('\0' != stringb[i]))
		return -1;              /* First string is shortest         */
	
	if (('\0' != stringa[i] ) && ('\0' == stringb[i]))
		return 1;               /* Second string is shortest        */
	
	return 0;                     /* They are identical               */
}

/* Compare the strings (case insensitive) */
gn_int16_t gn_stricmp(gn_cstr_t stringa, gn_cstr_t stringb)
{
	gn_int32_t i = 0;

	while (('\0' != stringa[i]) && ('\0' != stringb[i])) {
		if (gn_toupper(stringa[i]) < gn_toupper(stringb[i]))
			return -1;        /* Char from first string is first  */
		
		if (gn_toupper(stringa[i]) > gn_toupper(stringb[i]))
			return 1;         /* Char from second string is first */
		
		i++;                    /* Next char                        */
	}
	
	if (('\0' == stringa[i] ) && ('\0' != stringb[i]))
		return -1;              /* First string is shortest         */
	
	if (('\0' != stringa[i] ) && ('\0' == stringb[i]))
		return 1;               /* Second string is shortest        */
	
	return 0;                     /* They are identical               */
}



/* Compare the strings to specified size */
gn_int16_t gn_strncmp(gn_cstr_t stringa, gn_cstr_t stringb, gn_size_t size)
{
	gn_size_t i = 0;

	if(size == 0)
		return 0;

	while (('\0' != stringa[i]) && ('\0' != stringb[i])) {
		if (stringa[i] < stringb[i])
			return -1;        /* Char from first string is first  */
		
		if (stringa[i] > stringb[i])
			return 1;         /* Char from second string is first */

		if(i == size-1)
			return 0;
		
		i++;                    /* Next char                        */
	}
	
	if (('\0' == stringa[i] ) && ('\0' != stringb[i]))
		return -1;              /* First string is shortest         */
	
	if (('\0' != stringa[i] ) && ('\0' == stringb[i]))
		return 1;               /* Second string is shortest        */
	
	return 0;                     /* They are identical               */
}

/* Compare the strings to specified size (case insensitive)*/
gn_int16_t gn_strnicmp(gn_cstr_t stringa, gn_cstr_t stringb, gn_size_t size)
{
	gn_size_t i = 0;

	if(size == 0)
		return 0;

	while (('\0' != stringa[i]) && ('\0' != stringb[i])) {
		if (gn_toupper(stringa[i]) < gn_toupper(stringb[i]))
			return -1;        /* Char from first string is first  */
		
		if (gn_toupper(stringa[i]) > gn_toupper(stringb[i]))
			return 1;         /* Char from second string is first */

		if(i == size-1)
			return 0;
		
		i++;                    /* Next char                        */
	}
	
	if (('\0' == stringa[i] ) && ('\0' != stringb[i]))
		return -1;              /* First string is shortest         */
	
	if (('\0' != stringa[i] ) && ('\0' == stringb[i]))
		return 1;               /* Second string is shortest        */
	
	return 0;                     /* They are identical               */
}


/* Tokenise the string */
gn_str_t strtok(gn_str_t string,gn_cstr_t token)
{
	static gn_str_t ptr=NULL;
	gn_size_t i=0,j=0;
	gn_str_t temp=NULL;
	
	if(string != NULL)
		ptr = string;

	if(ptr == NULL)
		return NULL;

	while(ptr[i]) {
		for(j = 0; j < gn_strlen(token); j++) {
			if(ptr[i] == token[j]) {
				ptr[i] = '\0';
				ptr = &ptr[i+1];
				return ptr-i-1;
			}
		}
		i++;
	}

	temp = ptr;
	ptr = NULL;
	return temp;
}

/* Find first occurrence of specified character in string */
gn_str_t gn_strchr(gn_cstr_t string, gn_int16_t c )
{
	gn_size_t index=0;

	while(string[index]) {
		if(string[index] == c)
			return (gn_str_t) &string[index];
		index++;
	}

	return NULL;
}


/* Find last occurrence of specified character in string */
gn_str_t gn_strrchr(gn_cstr_t string, gn_int16_t c )
{
	gn_int32_t index=0;

	index = gn_strlen(string)-1;

	while(index >= 0) {
		if(string[index] == c)
			return (gn_str_t) &string[index];
		index--;
	}

	return NULL;
}
