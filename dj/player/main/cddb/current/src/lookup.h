/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * lookup.h - Wrappers for TOC lookup functionality used by shell app.
 */


#ifndef _LOOKUP_H_
#define _LOOKUP_H_

/*
 * Dependencies
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_tocinfo.h>
#include "display.h"


/*
 * Constants
 */

#define		LOC_DB		0x1
#define		LOC_ONLINE	0x2
#define		LOC_CACHE	0x4
#define		LOC_ALL		(LOC_DB|LOC_ONLINE|LOC_CACHE)

#ifdef _DEBUG
#define			FUZZY_CHOICES		50
#else
#define			FUZZY_CHOICES		20
#endif

#define NO_LOOKUP_TYPES				0
#define OFFLINE						1		/* Lookup toc from local database */
#define ONLINE						2		/* Lookup toc from online database */
#define CACHE						4		/* Lookup toc from local cache */
#define AUTOFUZZY					0x10	/* Perform Auto fuzzy lookup if there is no perfect match */ 
#define AUTOCACHE					0x20	/* Automatically cache information looked up */ 
#define EXACTONLY					0x40	/* Fail lookup if there is not an exact match */
#define DEFAULT_LOOKUP_TYPES		0xFFFFFFFF

#define STR_OFFLINE_LOOKUP			"OFFLINE"
#define STR_ONLINE_LOOKUP			"ONLINE"
#define STR_CACHE_LOOKUP			"CACHE"
#define STR_AUTOFUZZY_LOOKUP		"AUTOFUZZY"
#define STR_EXACTONLY_LOOKUP		"EXACTONLY"
#define STR_AUTOCACHE_LOOKUP		"AUTOCACHE"
#define STR_DEFAULT_LOOKUP			"DEFAULT_LOOKUP"


/*
 * Typedefs
 */

typedef gn_uint32_t		gn_lookup_t;


/*
 * Macros
 */

#define		IS_LOCAL(x)		(x == LOC_DB)
#define		IS_ONLINE(x)	(x == LOC_ONLINE)
#define		IS_CACHE(x)		(x == LOC_CACHE)
#define		IS_ALL(x)		(x == LOC_ALL)
#define		TRY_LOCAL(x)	(x & LOC_DB)
#define		TRY_ONLINE(x)	(x & LOC_ONLINE)
#define		TRY_CACHE(x)	(x & LOC_CACHE)


/*
 * Prototypes.
 */

/* Lookup TOC based upon passed string */
int lookup_toc_info(const char* tocstr, int location);

/* Lookup TOC and validate against expected title and artist */
int lookup_toc_info_and_validate(const char* tocstr, int location, const char* artiststr, const char* titlestr, int* num_fuzzies, int* num_nomatch, int* num_mismatch);

/* Lookup IDs based upon passed string */
int lookup_id_info(const char* idstr, int location);

/* Lookup IDs and validate against expected title and artist */
int lookup_id_info_and_validate(const char* idstr, int location, const char* artiststr, const char* titlestr);

/* Actually do the lookup of IDs from the online database */
int do_id_lookup_online(gn_cstr_t toc_id, gn_cstr_t media_id, display_discinfo_proc display_proc);

/* Actually do the lookup of IDs from the cache */
int do_id_lookup_cache(gn_cstr_t toc_id, display_discinfo_proc display_proc);

/* Actually do the lookup of a TOC structure from the online database */
int do_toc_lookup_online(gn_toc_info_t* toc, display_discinfo_proc display_proc);

/* Actually do the lookup of a TOC structure from the local cache */
int do_toc_lookup_cache(gn_toc_info_t* toc, display_discinfo_proc display_proc);

/* Actually do the lookup of a TOC structure from the local cache, the local database, and online */
int do_toc_lookup_all(gn_toc_info_t* toc, int location, display_discinfo_proc display_proc);

#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)

/* Add 'missed' TOC to cache */
int add_toc_2_cache(const char* tocstr);

/* Delete TOC from cache */
int delete_toc_from_cache(const char* tocstr);

/* Update TOCs in cache */
int update_cache(int location);

/* Display entry(ies) with matching TOC ID */
int dump_cache_entry(int toc_id, int verbose);

/* Dump all entries in the cache */
int dump_cache(int verbose);

/* convert gn_toc_info_t structure into gn_toc_full_t for cache lookups */
int convert_toc_info_to_full(const gn_toc_info_t* toc, gn_toc_full_t* toc_full);


#endif  /* #if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

void set_lookup_type(gn_lookup_t type);

gn_lookup_t get_lookup_type(void);

void display_lookup_type(void);

void set_lookup_type_str(gn_str_t type_str);


#endif  /* #ifndef _LOOKUP_H_ */
