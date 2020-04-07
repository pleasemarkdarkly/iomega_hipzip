/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * lookup.c:	Sample code demonstrating looking up disc information
 *				from a TOC from the local database.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include GN_STDLIB_H
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_lookup.h>
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
#include <extras/cddb/gn_cache.h>
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */
#include <extras/cddb/toc_util.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_memory.h>
#include "lookup.h"
#include "display.h"
#include "shell.h"
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_configmgr.h>
#include "branding.h"
#include <util/debug/debug.h>




/*
 * Variables
 */

gn_bool_t	g_validate_lookup = GN_FALSE;
char*		g_artiststr = GN_NULL;
char*		g_titlestr = GN_NULL;
char*		g_tocstr = GN_NULL;

int*			g_num_fuzzies = GN_NULL;
int*			g_num_nomatch = GN_NULL;
int*			g_num_mismatch = GN_NULL;

static gn_lookup_t	g_lookup_type = OFFLINE|ONLINE|CACHE|AUTOFUZZY|AUTOCACHE;


/* extern FILE*		g_misfile; */


#if defined(GN_NO_LOCAL)
static int do_toc_lookup_local(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	diag_printf("Local database lookups not supported.\n");
	return 0;
}
#else
/* Actually do the lookup of a TOC structure from the local database */
static int do_toc_lookup_local(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	gn_error_t			lookup_err;
	gn_xlt_error_t		xlate_err = XLTERR_NoError;
	gn_error_t			our_err = SUCCESS;
	gn_tlu_result_t		**results;
	gn_tlu_result_t		*selection = NULL;
	gn_tlu_match_t		match;
	gn_uint32_t			nresults = 0;
	gn_bool_t			did_init = GN_FALSE;
	void				*info = NULL;
	gn_size_t			info_size;
	gn_discinfo_t		*disc_info = NULL;
 	gn_lookup_t			type_lookup = AUTOFUZZY;
	gn_tlu_options_t	lu_options = TLOPT_Default;
  
    /* check input params */
	if (display_proc == NULL || toc == NULL) {
	    return -1;
    }

	/* get configuration information to determine lookup options */
	type_lookup = get_lookup_type();
	if ((type_lookup & AUTOFUZZY))
		lu_options |= TLOPT_AutoFuzzy;
	if ((type_lookup & EXACTONLY))
		lu_options |= TLOPT_ExactOnly;

	/* branding display on lookup */
	brand_display_on_lookup();

	/* perform the lookup */
	lookup_err = gntlu_lookup(toc, 0, lu_options, &match, &results, &nresults);

	if (lookup_err == TLERR_NotInited) {
		/* initialize lookup layer */
		lookup_err = gntlu_initialize();
		if (lookup_err != TLERR_NoError) {
			diag_printf("Error initializing (%X)\n", lookup_err);
			return lookup_err;
		}
		did_init = GN_TRUE;
	
		/* perform the lookup */
		lookup_err = gntlu_lookup(toc, 0, lu_options, &match, &results, &nresults);
	}

	/* if we find one or more exact or fuzzy matches */
	if (lookup_err == TLERR_NoError && nresults > 0) {
		/* default selection is the first result */
		selection = *results;

		if (g_num_fuzzies && match == TLMATCH_Fuzzy)
			(*g_num_fuzzies)++;

		/* for multiple matches, optionally let the user choose */
		if (nresults > 1) {

			our_err = resolve_local_fuzzy(results, nresults, &selection);

			if (our_err != SUCCESS) {

				/* release the search results */
				gntlu_release(results, nresults);

				/* call the display handler with NULL if the lookup was not successful */
   				display_proc(NULL);

				/* clean up */
				if (did_init)
					gntlu_shutdown();
				
				return our_err;
			}

		}

		if (selection != NULL) {
			/* retrieve the XML  data */
			lookup_err = gntlu_result_info_alloc(selection, &info, &info_size);
		}
		else {
			diag_printf("None selected.\n");
			if (g_num_nomatch)
				(*g_num_nomatch)++;
		}

		/* free the search results now that we have the metadata */
		gntlu_release(results, nresults);
	
		if (selection != NULL && lookup_err == TLERR_NoError) {
			/* diagnostic output */
			if (gDumpRaw == GN_TRUE) {
				diag_printf("Raw XML:\n");
				diag_printf("%s\n", info);
				diag_printf("End XML\n");
			}

		    /* convert XML data to disc info structure */
			xlate_err = gnxlt_convert_alloc(info, info_size, &disc_info);
		
			if (xlate_err == XLTERR_NoError) {
				/* call the display handler */
	   			our_err = display_proc(disc_info);

				/* and release these */
				gnxlt_release(disc_info);
				gnmem_free(disc_info);
			}
			else {
				diag_printf("Error translating data (%X)\n", xlate_err);
				our_err = xlate_err;
			}

			/* and release these */
			gnmem_free(info);
		}
		else
			our_err = lookup_err;
	}
	else {
		/* call the display handler with NULL if the lookup was not successful */
   		display_proc(NULL);
		our_err = lookup_err;
		if (g_num_nomatch)
			(*g_num_nomatch)++;
	}
	
	if (did_init == GN_TRUE) {
		/* clean up */
		gntlu_shutdown();
	}
	return our_err;
}
#endif /* #if defined(GN_NO_LOCAL) */


#if defined(GN_NO_LOCAL)
do_id_lookup_local(gn_cstr_t toc_id, gn_cstr_t media_id, display_discinfo_proc display_proc)
{
	diag_printf("Local database lookups not supported.\n");
	return 0;
}
#else
/* Actually do the lookup of an ID from the local database */
static int
do_id_lookup_local(gn_cstr_t toc_id, gn_cstr_t media_id, display_discinfo_proc display_proc)
{
	gn_error_t			lookup_err;
	gn_xlt_error_t		xlate_err = XLTERR_NoError;
	gn_error_t			our_err = SUCCESS;
	gn_tlu_result_t		**results;
	gn_tlu_result_t		*selection = NULL;
	gn_tlu_match_t		match;
	gn_uint32_t			nresults = 0;
	gn_uint32_t			id_number;
	gn_bool_t			did_init = GN_FALSE;
	void				*info = NULL;
	gn_size_t			info_size;
	gn_discinfo_t		*disc_info = NULL;
   
    /* check input params */
	if (display_proc == NULL || toc_id == NULL) {
	    return -1;
    }

	/* convert string to binary */
	id_number = atoi(toc_id);
	if (id_number == 0) {
		diag_printf("Invalid ID passed\n");
		return -1;
	}

	/* branding display on lookup */
	brand_display_on_lookup();

	/* perform the lookup */
	lookup_err = gntlu_lookup(NULL, id_number, TLOPT_Default, &match, &results, &nresults);

	if (lookup_err == TLERR_NotInited) {
		/* initialize lookup layer */
		lookup_err = gntlu_initialize();
		if (lookup_err != TLERR_NoError) {
			diag_printf("Error initializing (%X)\n", lookup_err);
			return lookup_err;
		}
		did_init = GN_TRUE;
	
		/* perform the lookup */
		lookup_err = gntlu_lookup(NULL, id_number, TLOPT_Default, &match, &results, &nresults);
	}

	/* if we find the ID */
	if (lookup_err == TLERR_NoError && nresults == 1) {
		/* use the first result selection */
		selection = *results;

		/* retrieve the XML  data */
		lookup_err = gntlu_result_info_alloc(selection, &info, &info_size);

		/* free the search results now that we have the metadata */
		gntlu_release(results, nresults);
	
		if (lookup_err  == TLERR_NoError) {

		    /* convert XML data to disc info structure */
			xlate_err = gnxlt_convert_alloc(info, info_size, &disc_info);
		
			if (xlate_err == XLTERR_NoError) {
				/* call the display handler */
	   			our_err = display_proc(disc_info);

				/* and release these */
				gnxlt_release(disc_info);
				gnmem_free(disc_info);
			}
			else {
				diag_printf("Error translating data (%d)\n", xlate_err);
				our_err = xlate_err;
			}

			/* and release these */
			gnmem_free(info);
		}
		our_err = lookup_err;
	}
	else {
		/* call the display handler with NULL if the lookup was not successful */
   		display_proc(NULL);
		our_err = lookup_err;
	}
	
	if (did_init == GN_TRUE) {
		/* clean up */
		gntlu_shutdown();
	}
	return our_err;
}
#endif /* #if defined(GN_NO_LOCAL) */


/*
 * Functions for looking up disc information from TOCs or IDs.
 */

/* Lookup TOC based upon passed string */
int
lookup_toc_info(const char* tocstr, int location)
{
	gn_uint32_t			offsets[MAXOFFSETS];
	gn_toc_info_t		toc;
	gn_char_t			err_buff[BUFSIZ];
	int					status;
    
    /* turn the supplied ASCII string into a TOC structure */
 	toc.offsets = offsets;
    status = convert_toc_string_to_array(tocstr, &toc, err_buff, sizeof(err_buff));
    if (status != RT_OK) {
        /* bad input, most likely */
        return status;
    }

	g_validate_lookup = GN_FALSE;
	g_artiststr = GN_NULL;
	g_titlestr = GN_NULL;
	g_tocstr = (char*)tocstr;

    /* Now that we have a legal TOC structure, do the lookup process. */
#ifndef GN_NO_ONLINE
	if (IS_ONLINE(location))
	    status = do_toc_lookup_online(&toc, display_discinfo);
	else
#endif
		if (IS_LOCAL(location))
		status = do_toc_lookup_local(&toc, display_discinfo);
	else if (IS_CACHE(location)) {
		status = do_toc_lookup_cache(&toc, display_discinfo);
	}
 	else {
		status = do_toc_lookup_all(&toc, location, display_discinfo);
	}
   
    /* anything else to do? */
    return status;
}

/* Lookup TOC and validate against expected title and artist */
int
lookup_toc_info_and_validate(const char* tocstr, int location, const char* artiststr, const char* titlestr, int* num_fuzzies, int* num_nomatch, int* num_mismatch)
{
	gn_uint32_t			offsets[MAXOFFSETS];
	gn_toc_info_t		toc;
	gn_char_t			err_buff[BUFSIZ];
	int					status;
    
    /* turn the supplied ASCII string into a TOC structure */
 	toc.offsets = offsets;
    status = convert_toc_string_to_array(tocstr, &toc, err_buff, sizeof(err_buff));
    if (status != RT_OK) {
        /* bad input, most likely */
        return status;
    }

	g_validate_lookup = GN_TRUE;
	g_artiststr = (char*)artiststr;
	g_titlestr = (char*)titlestr;
	g_tocstr = (char*)tocstr;
	g_num_fuzzies = num_fuzzies;
	g_num_nomatch = num_nomatch;
	g_num_mismatch = num_mismatch;

    /* Now that we have a legal TOC structure, do the lookup process. */
#ifndef GN_NO_ONLINE
	if (IS_ONLINE(location))
	    status = do_toc_lookup_online(&toc, display_validation_discinfo);
	else
#endif
		if (IS_LOCAL(location))
		status = do_toc_lookup_local(&toc, display_validation_discinfo);
	else if (IS_CACHE(location))
		status = do_toc_lookup_cache(&toc, display_validation_discinfo);
	else {
		status = do_toc_lookup_all(&toc, location, display_validation_discinfo);
	}

	g_num_fuzzies = GN_NULL;
	g_num_nomatch = GN_NULL;
	g_num_mismatch = GN_NULL;

    /* anything else to do? */
    return status;
}


/* Lookup IDs based upon passed string */
int
lookup_id_info(const char* idstr, int location)
{
	gn_char_t			strbuff[TOCBUFSIZE];
	gn_str_t			toc_id;
	gn_str_t			media_id;

	if (IS_ONLINE(location)) {
#if defined (GN_NO_ONLINE)
		return 1;
#else
		/* parse the supplied ASCII string into toc_id/media_id */
 		strncpy(strbuff, idstr, sizeof(strbuff) - 1);
		strbuff[sizeof(strbuff) - 1] = 0;
		toc_id = strbuff;
		media_id = strchr(strbuff, ' ');
		if (media_id == NULL)
			return 1;	/* bad format */

		*media_id++ = 0;

		g_validate_lookup = GN_FALSE;
		g_artiststr = GN_NULL;
		g_titlestr = GN_NULL;

		/* Now that we have parameters, do the lookup process. */
		return do_id_lookup_online(toc_id, media_id, display_discinfo);
#endif
	}
#if !defined (GN_NO_CACHE) || defined(GN_MEM_CACHE)
	else if (IS_CACHE(location)) {
		return do_id_lookup_cache(idstr, display_discinfo);
	}
#endif /* #if !defined (GN_NO_CACHE) || defined(GN_MEM_CACHE) */
	else {
		/* parse the supplied ASCII string into toc_id/media_id */
 		strncpy(strbuff, idstr, sizeof(strbuff) - 1);
		strbuff[sizeof(strbuff) - 1] = 0;
		toc_id = strbuff;
		media_id = strchr(strbuff, ' ');

		if (media_id)
			*media_id++ = 0;

		g_validate_lookup = GN_FALSE;
		g_artiststr = GN_NULL;
		g_titlestr = GN_NULL;

		/* Now that we have parameters, do the lookup process. */
		return do_id_lookup_local(toc_id, media_id, display_discinfo);
	}
}

/* Lookup IDs and validate against expected title and artist */
int
lookup_id_info_and_validate(const char* idstr, int location, const char* artiststr, const char* titlestr)
{
	diag_printf("Not implemented\n");
	return 0;
}



/*
 *	Conversion helper functions.
 */


/* convert gn_toc_info_t structure into gn_toc_full_t for cache lookups */
int
convert_toc_info_to_full(const gn_toc_info_t* toc, gn_toc_full_t* toc_full)
{
	toc_full->num_offsets = toc->num_offsets;
	gnmem_memcpy(&toc_full->offsets[0], toc->offsets, toc->num_offsets * sizeof(toc_full->offsets[0]));
	return 0;
}


void set_lookup_type(gn_lookup_t type)
{
	g_lookup_type = type;
}


gn_lookup_t get_lookup_type(void)
{
	return g_lookup_type;
}


void display_lookup_type(void)
{
	gn_lookup_t	type_lookup = get_lookup_type();
	gn_bool_t	predecessor = GN_FALSE;

	diag_printf("Lookup type : ");

	if (type_lookup & OFFLINE)
	{
		diag_printf("%s", STR_OFFLINE_LOOKUP);
		predecessor = GN_TRUE;
	}

	if (type_lookup & ONLINE)
	{
		if (predecessor == GN_TRUE)
			diag_printf(",");
		diag_printf("%s", STR_ONLINE_LOOKUP);
		predecessor = GN_TRUE;
	}
	
	if (type_lookup & CACHE)
	{
		if (predecessor == GN_TRUE)
			diag_printf(",");
		diag_printf("%s", STR_CACHE_LOOKUP);
		predecessor = GN_TRUE;
	}
	
	if (type_lookup & AUTOFUZZY)
	{
		if (predecessor == GN_TRUE)
			diag_printf(",");
		diag_printf("%s", STR_AUTOFUZZY_LOOKUP);
		predecessor = GN_TRUE;
	}
	
	if (type_lookup & AUTOCACHE)
	{
		if (predecessor == GN_TRUE)
			diag_printf(",");
		diag_printf("%s", STR_AUTOCACHE_LOOKUP);
		predecessor = GN_TRUE;
	}
	
	if (type_lookup & EXACTONLY)
	{
		if (predecessor == GN_TRUE)
			diag_printf(",");
		diag_printf("%s", STR_EXACTONLY_LOOKUP);
		predecessor = GN_TRUE;
	}

	diag_printf("\n");
}


void set_lookup_type_str(gn_str_t type_str)
{
	gn_uchar_t	buffer[256] = "";
	gn_lookup_t	type_lookup = 0;
	gn_uchar_t*	token = NULL;

	if ((type_str == NULL) || (*type_str == 0))
	{
		set_lookup_type(0);
		return;
	}

	strcpy(buffer,type_str);
	token = strtok(buffer, ", ");

	if (token)
	{
		do
		{
			if (strcmp(token, STR_OFFLINE_LOOKUP) == 0)
				type_lookup |= OFFLINE;
			else if (strcmp(token, STR_ONLINE_LOOKUP) == 0)
				type_lookup |= ONLINE;
			else if (strcmp(token, STR_CACHE_LOOKUP) == 0)
				type_lookup |= CACHE;
			else if (strcmp(token, STR_AUTOFUZZY_LOOKUP) == 0)
				type_lookup |= AUTOFUZZY;
			else if (strcmp(token, STR_AUTOCACHE_LOOKUP) == 0)
				type_lookup |= AUTOCACHE;
			else if (strcmp(token, STR_EXACTONLY_LOOKUP) == 0)
				type_lookup |= EXACTONLY;

		}
		while ((token = strtok( NULL, ", ")) != NULL);
	}

	set_lookup_type(type_lookup);
}
