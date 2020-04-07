/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * clookup.c:	Sample code demonstrating looking up disc information
 *				from a TOC and using the local cache.
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



#if defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE)
int
do_toc_lookup_cache(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	diag_printf("Cache lookups not supported.\n");
	return 0;
}
#else
/* Actually do the lookup of a TOC structure from the local cache */
int
do_toc_lookup_cache(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	gn_error_t				error;
	int						our_error = 0;
	gn_toc_full_t			toc_full;
	gn_uint32_t				toc_id;
	void*					info = NULL;
	gn_size_t				info_size = 0;
	gn_discinfo_t			*disc_info = NULL;
	gn_cache_data_t			cookie = NULL;
	gn_cache_rec_type_t		type;
    
    /* turn the supplied TOC into right format */
	convert_toc_info_to_full(toc, &toc_full);

	/* branding display on lookup */
	brand_display_on_lookup();

	error = gncache_lookup_toc((gn_toc_hdr_t*)&toc_full, &cookie);

	if (error == SUCCESS) {
		error = gncache_data_type(cookie, &type);

		if (error == SUCCESS) {
			if (type == CACHE_TYPE_TOC)
				diag_printf("Record found in cache, TOC-only\n");
			else if (type == CACHE_TYPE_TOC_ID) {
				gn_tlu_match_t		match = 0;
				gn_tlu_result_t		**results = NULL;
				gn_tlu_result_t		*selection = NULL;
				gn_uint32_t			nresults = 0;

				toc_id = 0;
				gncache_data_toc_id(cookie, &toc_id);
				diag_printf("Record found in cache, toc_id == %d\n");
#if !defined(GN_NO_LOCAL)
				if (gVerbose) {
					error = gntlu_lookup(toc, toc_id, TLOPT_Default, &match, &results, &nresults);
					if (error == TLERR_NoError && nresults == 1) {
						/* retrieve the XML  data */
						error = gntlu_result_info_alloc(*results, &info, &info_size);

						/* free the search results now that we have the metadata */
						gntlu_release(results, nresults);
					}
				}
#endif
			}
			else if (type == CACHE_TYPE_DATA)
				error = gncache_retrieve_data_alloc(cookie, &info, &info_size);
			else
				diag_printf("Record found in cache, unknown type: %d\n", type);

			if (error == SUCCESS && info != NULL) {
				/* diagnostic output */
				if (gDumpRaw == GN_TRUE) {
					diag_printf("Raw XML:\n");
					diag_printf("%s\n", info);
					diag_printf("End XML\n");
				}

				/* convert XML data to disc info structure */
				error = gnxlt_convert_alloc(info, info_size, &disc_info);
			
				if (error == XLTERR_NoError) {
					/* call the display handler */
	   				error = display_proc(disc_info);

					/* and release these */
					gnxlt_release(disc_info);
					gnmem_free(disc_info);
				}
				else {
					diag_printf("Error translating data (%d)\n", error);
				}

				/* and release these */
				gnmem_free(info);
			}
		}
		gncache_release_data(cookie);
	}
	else
	{
		display_proc(NULL);
		if (error == CACHE_ERR_NotFound)
			error = SUCCESS;
	}

	return error;
}
#endif /* #if defined(GN_NO_CACHE) */



#if (defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE)) || defined(GN_NO_ONLINE)
int
do_id_lookup_cache(gn_cstr_t toc_id, display_discinfo_proc display_proc)
{
	diag_printf("Cache lookups not supported.\n");
	return 0;
}
#else
/* Actually do the lookup of IDs from the local cache */
int
do_id_lookup_cache(gn_cstr_t toc_id, display_discinfo_proc display_proc)
{
	gn_error_t			error;
	gn_uint32_t			id_number;
	gn_cache_data_t		cookie = 0;
	void*				info = NULL;
	gn_size_t			info_size = 0;
	gn_discinfo_t		*disc_info = NULL;

	/* convert string to binary */
	id_number = atoi(toc_id);
	if (id_number == 0) {
		diag_printf("Invalid ID passed\n");
		return -1;
	}

	/* branding display on lookup */
	brand_display_on_lookup();

	error = gncache_lookup_toc_id(id_number, &cookie);

	if (error != SUCCESS)
		return (int) error;

	if (error == 0 && cookie != 0) {

	    /* retrieve XML data from cache */
		error = gncache_retrieve_data_alloc(cookie, &info, &info_size);
		
	    /* and convert to disc info structure */
		if (error == SUCCESS && info != NULL)
			error = gnxlt_convert_alloc(info, info_size, &disc_info);

		if (error == 0) {
			/* call the display handler */
   			display_proc(disc_info);

			/* and release the C structure */
			if (info != 0) {
				gnxlt_release(disc_info);
				gnmem_free((void*)disc_info);
				info = 0;
			}
		} else {
			diag_printf("Error getting/translating data (%d)\n", error);
		}

		/* and release the XML text */
		if (info)
			gnmem_free(info);
	}
	return error;
}

#endif /* #if defined(GN_NO_CACHE) */



/*
 * Functions for updating the local data cache.
 */

#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)

/* helper for displaying record types */
static char*	entry_type(gn_cache_rec_type_t type)
{
	if (type == CACHE_TYPE_TOC_ID) {
		return "CACHE_TYPE_TOC_ID";
	}
	else if (type == CACHE_TYPE_DATA) {
		return "CACHE_TYPE_DATA";
	}
	else if (type == CACHE_TYPE_TOC) {
		return "CACHE_TYPE_TOC";
	}
	return "UNKNOWN";
}


/* Add 'missed' TOC to cache */
int add_toc_2_cache(const char* tocstr)
{
	gn_uint32_t				offsets[MAXOFFSETS];
	gn_toc_info_t			toc;
	gn_toc_full_t			toc_full;
	gn_char_t				err_buff[BUFSIZ];
	gn_cache_data_t			cookie = NULL;
	gn_cache_rec_type_t		type;
	gn_cache_error_t		error = SUCCESS;
	int						status;
    
    /* turn the supplied ASCII string into a TOC structure */
 	toc.offsets = offsets;
    status = convert_toc_string_to_array(tocstr, &toc, err_buff, sizeof(err_buff));
    if (status != RT_OK) {
        /* bad input, most likely */
        return status;
    }

	/* convert to proper format for cache lookups/inserts */
	convert_toc_info_to_full(&toc, &toc_full);

	/* branding display on lookup */
	brand_display_on_lookup();

	/* first check to see if already in cache */
	error = gncache_lookup_toc((gn_toc_hdr_t*)&toc_full, &cookie);

	/* if we found a match, look closer */
	if (error == SUCCESS) {

		/* get the type for this record */
		error = gncache_data_type(cookie, &type);

		if (error == SUCCESS)
			diag_printf("Record of type '%s' already in cache for TOC: %s\n", entry_type(type), tocstr);
		else
			diag_printf("Error (%X) getting record type\n", error);

		/* always release this */
		gncache_release_data(cookie);
		return error;
	}

	/* TOC not there, so go ahead and add */
	error = gncache_add_toc_id((gn_toc_hdr_t*)&toc_full, 0);
   
    return error;
}

/* Delete TOC from cache */
int
delete_toc_from_cache(const char* tocstr)
{
	gn_uint32_t			offsets[MAXOFFSETS];
	gn_toc_info_t		toc;
	gn_toc_full_t		toc_full;
	gn_char_t			err_buff[BUFSIZ];
	gn_cache_error_t	error = SUCCESS;
	int					status;
    
    /* turn the supplied ASCII string into a TOC structure */
 	toc.offsets = offsets;
    status = convert_toc_string_to_array(tocstr, &toc, err_buff, sizeof(err_buff));
    if (status != RT_OK) {
        /* bad input, most likely */
        return status;
    }
	convert_toc_info_to_full(&toc, &toc_full);

	error = gncache_delete_toc(0, (gn_toc_hdr_t*)&toc_full);
   
    return error;
}

/* Update TOCs in cache */
int
update_cache(int location)
{
	gn_cache_error_t		error = SUCCESS;
	int						our_error = 0;
	unsigned int			n;
	gn_tlu_result_t			**results = NULL;
	gn_uint32_t				nresults = 0;
	XMLTagRef				xml = 0;
	XMLTagRef				disc_id_tag = 0;
	XMLTagRef				toc_id_tag = 0;
	gn_str_t				buffer = NULL;
	gn_cstr_t				toc_id_str = NULL;
	gn_toc_hdr_t*			toc = NULL;
	gn_toc_info_t			toc_info;
	gn_cache_data_t			cookie;
	gn_cache_rec_type_t		type;
	gn_uint32_t				toc_id;
	gn_tlu_match_t			match;
	gn_bool_t				not_found = GN_FALSE;


	/* branding display on update */
	brand_display_on_update();

	/* loop through all records with toc_id == 0 */
	while (error == SUCCESS) {
		cookie = NULL;
		toc = NULL;
		toc_id = 0;
		xml = NULL;
		match = TLMATCH_None;
		nresults = 0;

		error = gncache_lookup_toc_id(0, &cookie);

		if (error != SUCCESS) {
			/* this error condition indicates we've exhausted the records */
			if (error == CACHE_ERR_NotFound)
				error = SUCCESS;
			break;
		}

		/* make sure that we have consistent data with the record */
		error = gncache_data_type(cookie, &type);
		if (error != SUCCESS) {
			diag_printf("Unexpected error getting record type\n");
			break;
		}

		error = gncache_data_toc_id(cookie, &toc_id);
		if (error != SUCCESS) {
			diag_printf("Unexpected error getting toc_id\n");
			break;
		}

		if (type != CACHE_TYPE_TOC || toc_id != 0) {
			diag_printf("Record had unexpected type(%d) or toc_id(%d)\n", type, toc_id);
			break;
		}

		/* we need the TOC to perform the lookup for the disc data */
		error = gncache_data_toc(cookie, &toc);
		if (error != SUCCESS || toc == NULL) {
			diag_printf("Unexpected error getting toc\n");
			break;
		}

		/* put TOC into proper format for online/local lookups */
		toc_info.num_offsets = toc->num_offsets;
		toc_info.offsets = &toc->offsets[0];

#if !defined(GN_NO_LOCAL)
		/* first try local db */
		if (TRY_LOCAL(location)) {
			diag_printf("Looking up TOC in local database.\n");

			/* perform the lookup */
			error = gntlu_lookup(&toc_info, 0, TLOPT_AutoFuzzy, &match, &results, &nresults);

			/* if we got a match, update cache or delete TOC */
			if (error == SUCCESS && nresults == 1) {
#if defined(GN_CACHE_DATA) || defined(GN_DCDDB)
				void*		rawbuff = NULL;
				gn_size_t	rawsize = 0;
				void*		rawbuff2 = NULL;
				gn_size_t	rawsize2 = 0;

				/* reduce the amount of data stored in the cache */
				error = gntlu_result_info_alloc(*results, &rawbuff, &rawsize);
				error = gnxlt_reduce_raw(rawbuff, rawsize, &rawbuff2, &rawsize2, 0);
				gnmem_free(rawbuff);
				error = gncache_update_toc_data(cookie, 2, rawbuff2, rawsize2);
				gnmem_free(rawbuff2);

				if (error == SUCCESS) {
					diag_printf("Adding album data to cache for TOC:\n");
					for (n = 0; n < toc_info.num_offsets; n++)
						diag_printf(" %d", toc_info.offsets[n]);
					diag_printf("\n");
				}

				/* TEMP: set this to non-zero to avoid re-querying this TOC */
				toc_id = 1;
#else
				/* only update cache if it was a fuzzy match */
				if (match == TLMATCH_Fuzzy) {
					diag_printf("Updating cache with local toc id.\n");
					toc_id = gntlu_result_toc_id(*results);
					error = gncache_update_toc_id(cookie, toc_id);
				}
				else {
					diag_printf("Deleting TOC from cache, now in local database.\n");
					error = gncache_delete_toc(cookie, NULL);

					if (error == SUCCESS) {
						diag_printf("TOC deleted:");
						for (n = 0; n < toc_info.num_offsets; n++)
							diag_printf(" %d", toc_info.offsets[n]);
						diag_printf("\n");
					}

					toc_id = 0xffffffff;	/* set to prevent online lookup */
				}
#endif

				gntlu_release(results, nresults);
			}
		}
#endif /* #if !defined(GN_NO_LOCAL) */

#if !defined(GN_NO_ONLINE)
		/* if not found, try online */
		if (toc_id == 0 && TRY_ONLINE(location)) { 
			diag_printf("Looking up TOC in online database.\n");

			/* NOTE: we could allow Fuzzy choices here, but not for now */
			error = gntlu_lookup_online(&toc_info, &xml, &match);
			if (error != SUCCESS) {
				diag_printf("Error (%X) looking up toc online, TOC:", error);
				for (n = 0; n < toc_info.num_offsets; n++)
					diag_printf(" %d", toc_info.offsets[n]);
				diag_printf("\n");
				break;
			}

			if (xml != NULL) {
				/* We need to convert the XML into a buffer for storage */
				buffer = RenderXMLTagToStrEx(xml, GN_FALSE, GN_FALSE, GN_FALSE);
				if (buffer == NULL) {
					diag_printf("Problem converting XML to buffer\n");
					break;
				}

				/* Extract the MUID (toc_id) from the XML data.  We want to update this */
				/* in the cache to avoid finding entries which have updated data in this while loop */
				disc_id_tag = GetXMLSubTagFromStr(xml, GN_STR_DISCID);
				if (disc_id_tag) {
					toc_id_tag = GetXMLSubTagFromStr(disc_id_tag, GN_STR_MUID);
					if (toc_id_tag) {
						toc_id_str = GetXMLTagData(toc_id_tag);
						if (toc_id_str)
							toc_id = parse_buf_to_int32(toc_id_str, strlen(toc_id_str));
					}
				}

				/* always set this to something non-zero so we don't update this TOC again */
				if (toc_id == 0)
					toc_id = 1;

				/* We're finished with the XML representation of the data */
				DisposeXMLTag(xml);
				xml = NULL;

				/* Finally add the data to the cache entry */
				error = gncache_update_toc_data(cookie, toc_id, buffer, strlen(buffer)+1);
				gnmem_free(buffer);
				buffer = NULL;

				if (error == SUCCESS) {
					diag_printf("Adding album data to cache for TOC:\n");
					for (n = 0; n < toc_info.num_offsets; n++)
						diag_printf(" %d", toc_info.offsets[n]);
					diag_printf("\n");
				}
			}
		}
#endif /* #if !defined(GN_NO_ONLINE) */

		if (toc_id == 0) {
			diag_printf("This TOC not found: \n");
			for (n = 0; n < toc_info.num_offsets; n++)
				diag_printf(" %d", toc_info.offsets[n]);
			diag_printf("\n");

			not_found = GN_TRUE;
			/* reset the toc_id for this TOC so we won't loop */
			toc_id = 0xffffffff;
			error = gncache_update_toc_id(cookie, toc_id);
		}

		/* we're done with this record */
		gncache_release_data(cookie);
		cookie = NULL;
	}

	/* clean up on unexpected error */
	if (cookie != NULL) {
		gncache_release_data(cookie);
		cookie = NULL;
	}
	if (xml != NULL) {
		DisposeXMLTag(xml);
	}

	/* if there were some TOCs that weren't found, reset their toc_ids to zero */
	if (not_found == GN_TRUE && error == SUCCESS) {
		while (error == SUCCESS) {
			cookie = NULL;
			toc_id = 0;

			error = gncache_lookup_toc_id(0xffffffff, &cookie);

			if (error != SUCCESS) {
				/* this error condition indicates we've exhausted the records */
				if (error == CACHE_ERR_NotFound)
					error = SUCCESS;
				break;
			}

			/* make sure that we have consistent data with the record */
			error = gncache_data_type(cookie, &type);
			if (error != SUCCESS) {
				diag_printf("Unexpected error getting record type\n");
				break;
			}

			error = gncache_data_toc_id(cookie, &toc_id);
			if (error != SUCCESS) {
				diag_printf("Unexpected error getting toc_id\n");
				break;
			}

			if (type != CACHE_TYPE_TOC_ID || toc_id != 0xffffffff) {
				diag_printf("Record had unexpected type(%d) or toc_id(%d)\n", type, toc_id);
				break;
			}
			error = gncache_update_toc_id(cookie, 0);
			gncache_release_data(cookie);
			cookie = NULL;
		}
	}

	/* clean up on unexpected error */
	if (cookie != NULL) {
		gncache_release_data(cookie);
		cookie = NULL;
	}

	return error;
}

static int	entry_count;
static int	full_info;

/* Callback function for displaying contents of the local cache */
gn_cache_action_t dump_callback(gn_cache_data_t cookie)
{
	gn_cache_error_t		error = SUCCESS;
	gn_toc_hdr_t*			toc = NULL;
	gn_cache_rec_type_t		type = 0;
	gn_uint32_t				toc_id = 0;
	gn_size_t				size = 0;
	gn_uint32_t				n;
	void*					info = NULL;

	diag_printf("Cache entry #%d:\n", ++entry_count);
	error = gncache_data_type(cookie, &type);
	if (error == SUCCESS) {
		error = gncache_data_toc(cookie, &toc);
		if (error == SUCCESS) {
			error = gncache_data_toc_id(cookie, &toc_id);
			if (error == SUCCESS) {
				if (type == CACHE_TYPE_TOC_ID) {
				}
				else if (type == CACHE_TYPE_DATA) {
					error = gncache_data_size(cookie, &size);
				}
				else if (type != CACHE_TYPE_TOC) {
					diag_printf("Unexpected entry type: %d\n");
				}
			}
		}
		if (error == SUCCESS) {
			diag_printf("Type: %s, toc_id = %d, size = %d\n", entry_type(type), toc_id, size);
			if (full_info) {
				diag_printf("TOC:");
				for (n = 0; n < toc->num_offsets; n++)
					diag_printf(" %d", toc->offsets[n]);
				diag_printf("\n");
				if (type == CACHE_TYPE_TOC_ID) {
					diag_printf("TOC_ID: %d", toc_id);
#if !defined(GN_NO_LOCAL)
					if (full_info) {
						gn_tlu_match_t		match = 0;
						gn_tlu_result_t		**results = NULL;
						gn_tlu_result_t		*selection = NULL;
						gn_uint32_t			nresults = 0;

						error = gntlu_lookup(NULL, toc_id, TLOPT_Default, &match, &results, &nresults);
						if (error == TLERR_NoError && nresults == 1) {
							/* retrieve the XML  data */
							error = gntlu_result_info_alloc(*results, &info, &size);

							/* free the search results now that we have the metadata */
							gntlu_release(results, nresults);
							if (error == SUCCESS) {
								display_artist_title(info, size, NULL);
								gnmem_free(info);
							}
						}
					}
#endif /* #if !defined(GN_NO_LOCAL) */
				}
				else if (type == CACHE_TYPE_DATA) {
					error = gncache_retrieve_data_alloc(cookie, &info, &size);
					if (error == SUCCESS) {
						gn_error_t			xlate_err = XLTERR_NoError;
						gn_discinfo_t		*disc_info = NULL;
						gn_error_t			our_err = SUCCESS;

						xlate_err = gnxlt_convert_alloc(info, size, &disc_info);
		
						if (xlate_err == XLTERR_NoError) {
							/* call the display handler */
		   					our_err = display_discinfo(disc_info);

							/* and release these */
							gnxlt_release(disc_info);
							gnmem_free(disc_info);
							gnmem_free(info);
						}
					}
				}
			}
			else
				diag_printf("TOC: %d offsets, start: %d, end: %d\n", toc->num_offsets, toc->offsets[0], toc->offsets[toc->num_offsets-1]);
		}
	}
	if (error != SUCCESS)
		diag_printf("Error (%X) retrieving entry information.\n", error);

	return CACHE_CB_None;
}

/* Display entry(ies) with matching TOC ID */
int dump_cache_entry(int toc_id, int verbose)
{
	entry_count = 0;
	full_info = verbose;

	/* branding display on lookup */
	brand_display_on_lookup();

	return gncache_lookup_toc_ids(&toc_id, dump_callback);
}

/* Dump all entries in the cache */
int dump_cache(int verbose)
{
	entry_count = 0;
	full_info = verbose;

	/* branding display on lookup */
	brand_display_on_lookup();

	return gncache_lookup_tocs(NULL, dump_callback);
}

#endif	/* #if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)*/



