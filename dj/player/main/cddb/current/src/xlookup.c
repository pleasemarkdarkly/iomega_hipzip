/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * xlookup.c:	Sample code demonstrating looking up disc information
 *				from a TOC from a variety of sources.
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
 * do_toc_lookup_all()
 *
 * Description:
 *	Look in variouse locations for information associated with this TOC.
 *	The possible locations are the local cache, the local database, and
 *	the online database.  If caching is enabled, the local cache is
 *	checked first.  Then the local database is checked.  Finally the
 *	lookup will proceed online and (depending on options) update the
 *	local cache.
 */

int
do_toc_lookup_all(gn_toc_info_t* toc, int location, display_discinfo_proc display_proc)
{
	gn_error_t			lookup_err;
	gn_xlt_error_t		xlate_err = XLTERR_NoError;
	gn_error_t			our_err = SUCCESS;
	gn_tlu_result_t		**results = NULL;
	gn_tlu_result_t		*selection = NULL;
	gn_tlu_match_t		match;
	gn_tlu_options_t	options = TLOPT_Default;
	gn_uint32_t			nresults = 0;
	void				*info = NULL;
	gn_size_t			info_size = 0;
	gn_discinfo_t		*disc_info = NULL;
	XMLTagRef			xml = 0;
	XMLTagRef			xml_choice = 0;
	XMLTagRef			xml_full = 0;
	XMLTagRef			discid_tag = 0;
	XMLTagRef			muid_tag = 0;
	XMLTagRef			mediaid_tag = 0;
	gn_uint32_t			toc_id = 0;
	gn_lookup_t			type_lookup = (ONLINE|OFFLINE|CACHE);
	gn_bool_t			cache_sup = GN_TRUE;
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	gn_cstr_t			muid_str;
	gn_toc_full_t		toc_full;
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	gn_cache_data_t		cookie = NULL;
	gn_cache_rec_type_t	type = 0;
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

	/* check input params */
	if (display_proc == NULL || toc == NULL || location == 0) {
		return -1;
	}

	/* get configuration information to determine how to proceed - defaults set above */
	gnconf_get(CACHE_SUPPORT, &cache_sup);
	type_lookup = get_lookup_type();

	/* if possible, first look in local cache */
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)

	if (cache_sup == GN_TRUE && (type_lookup & CACHE) && TRY_CACHE(location)) {
		diag_printf("Looking up TOC in local cache\n");

		/* convert toc_info into cache storage format */
		convert_toc_info_to_full(toc, &toc_full);

		/* branding display on lookup */
		brand_display_on_lookup();

		lookup_err = gncache_lookup_toc((gn_toc_hdr_t*)&toc_full, &cookie);
		if (lookup_err == SUCCESS && cookie != NULL) {
			/* see what kind of record we have here */
			our_err = gncache_data_type(cookie, &type);

			if (our_err == SUCCESS) {
				/* if this includes data, go ahead and retrieve */
				if (type == CACHE_TYPE_DATA)
					our_err = gncache_retrieve_data_alloc(cookie, &info, &info_size);
				else if (type == CACHE_TYPE_TOC_ID)
					our_err = gncache_data_toc_id(cookie, &toc_id);

				if (our_err != SUCCESS || (type != CACHE_TYPE_TOC && info == NULL && toc_id == 0)) {
					diag_printf("Unexpected result getting cookie info from cache (err=%X)\n", our_err);
					gncache_release_data(cookie);
					cookie = NULL;
				}
			}
			else {
				diag_printf("Unexpected result getting cookie type from cache (err=%X)\n", our_err);
				gncache_release_data(cookie);
				cookie = NULL;
			}
		}
		/* this error means the TOC wasn't in the cache */
		/* others mean we should abort */
		else if (
			(lookup_err != CACHE_ERR_NotFound)
			&&
			(lookup_err != CACHE_ERR_NoCache)
			) {
			diag_printf("Unexpected result looking up TOC from cache(err=%X)\n", lookup_err);
			return -1;
		}
		if (cookie == NULL)
			diag_printf("TOC not found in local cache\n");
	}

	/* we found something in the local cache - take a closer look */
	if (cookie != NULL) {
		diag_printf("Found record type %s in local cache.\n", type == CACHE_TYPE_TOC ? "TOC" : (type == CACHE_TYPE_TOC_ID ? "TOC_ID" : "TOC_DATA"));

#if	!defined(GN_NO_LOCAL)
		/* for this type, we can look in the local database for disc information */
		if (type == CACHE_TYPE_TOC_ID) {

			/* this mix of options doesn't make sense */
			if (!TRY_LOCAL(location) || !(type_lookup & OFFLINE)) {
				diag_printf("Inconsistent location/mode with TOC_ID cache type.  Aborting.\n");
				gncache_release_data(cookie);
				return -1;
			}

			/* branding display on lookup */
			brand_display_on_lookup();

			lookup_err = gntlu_lookup(toc, toc_id, 0, &match, &results, &nresults);

			/* we should find the ID */
			if (lookup_err == TLERR_NoError && nresults == 1) {
				/* retrieve the XML  data */
				lookup_err = gntlu_result_info_alloc(*results, &info, &info_size);

				/* free the search results now that we have the metadata */
				gntlu_release(results, nresults);
			}
			else {
				diag_printf("Error or unexpected results looking up toc_id (err=%X)\n", lookup_err);
				if (nresults) {
					gntlu_release(results, nresults);
					gncache_release_data(cookie);
					return -1;
				}
			}
		}
#endif /* #if	!defined(GN_NO_LOCAL) */


	}
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

	/* for other local cache hits or local cache misses, must look up by TOC */
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	if (cookie == NULL || type == CACHE_TYPE_TOC)
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */
	{

		/* set options from type_lookup */
		if (type_lookup & AUTOFUZZY)
			options |= TLOPT_AutoFuzzy;

#if	!defined(GN_NO_LOCAL)
		/* try local first - if allowed */
		if (TRY_LOCAL(location) && (type_lookup & OFFLINE)) {
			diag_printf("Looking up TOC in local database\n");

			/* branding display on lookup */
			brand_display_on_lookup();

			lookup_err = gntlu_lookup(toc, 0, options, &match, &results, &nresults);

			/* found at least one */
			if (lookup_err == SUCCESS && nresults > 0) {

				/* default to first match */
				selection = *results;

				/* (optionally) let user choose if more than one match */
				if (nresults > 1) {
					diag_printf("There are %d potential matches for this TOC.\n", nresults);
					our_err = resolve_local_fuzzy(results, nresults, &selection);
					match = TLMATCH_Fuzzy;
				}

				if (our_err == SUCCESS && selection != NULL) {
					/* get the toc_id & XML data */
					toc_id = gntlu_result_toc_id(selection);
					lookup_err = gntlu_result_info_alloc(selection, &info, &info_size);
				}
				else if (selection == NULL)
					diag_printf("No selection chosen from local list.\n");

				/* free the search results now that we have the metadata */
				gntlu_release(results, nresults);
			}
			else {
				diag_printf("TOC not found in local database (err=%X)\n", lookup_err);
			}
		}
#endif /* #if	defined(GN_NO_LOCAL) */

		/* we have not found a match - try online if available */
#if	!defined(GN_NO_ONLINE)

		if (info == NULL) {

			/* try online - if allowed */
			if (TRY_ONLINE(location) && (type_lookup & ONLINE)) {

				diag_printf("Looking up TOC in online database\n");

				/* branding display on lookup */
				brand_display_on_lookup();

				lookup_err = gntlu_lookup_online_ex(toc, options, 0, &xml, &match);

				if (lookup_err == SUCCESS) {

					if (match == TLMATCH_Fuzzy) {
						our_err = resolve_online_fuzzy(xml, &xml_choice);

						/* upon choice by user, get full disc info from online database */
						if (our_err == SUCCESS && xml_choice != NULL) {

							/* xml_choice should be a partial ALB tag which has a DISCID tag */
							/* the DISCID tag contains the MUID and MEDIAID we need to get the */
							/* full disc information for this TOC */
							if (strcmp(GetXMLTagName(xml_choice), GN_STR_ALBUM))
								our_err = kXMLInvalidParamError;

							if (our_err == SUCCESS) {
								discid_tag = GetXMLSubTagFromStr(xml_choice, GN_STR_DISCID);

								/* extract toc_id(MUID) and media_id(MEDIAID) from XML */
								if (discid_tag) {
									muid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MUID);
									mediaid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MEDIAID);
								}
								if (muid_tag == 0 || mediaid_tag == 0)
									our_err = kXMLInvalidParamError;
							}

							/* if still okay, get full disc information */
							if (our_err == SUCCESS) {

								/* branding display on lookup */
								brand_display_on_lookup();

								lookup_err = gntlu_lookup_id_online(GetXMLTagData(muid_tag), GetXMLTagData(mediaid_tag), &xml_full);
								if (lookup_err == SUCCESS && xml_full) {
									diag_printf("Successfully got full info online.\n");
								}
								our_err = lookup_err;
							}
						}
						else if (our_err == SUCCESS) {
							diag_printf("No selection chosen from online list.\n");
							DisposeXMLTag(xml);
							xml = NULL;
						}
					}
					else  if (match == TLMATCH_None) {
						diag_printf("TOC not found in online lookup (err=%X)\n", lookup_err);
					}

					/* if we did a fuzzy match, replace original XML */
					if (xml_full) {
						if (xml)
							DisposeXMLTag(xml);
						xml = xml_full;
						xml_full = NULL;
					}
				}
				else
					our_err = lookup_err;
			}
		}
#endif /* #if	!defined(GN_NO_ONLINE) */
	}


	/* we've looked everywhere we can - if there are results, display them */
	if (info) {
		/* convert raw XML data to disc info structure */
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
	}
	else if (xml) {
		/* convert structured XML data to disc info structure */
		xlate_err = gnxlt_convert_xml_alloc(xml, &disc_info);
	
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
	}
	else {
		diag_printf("Couldn't locate information for this TOC.\n");
	}

#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)

	/* update cache, if so configured and we didn't get data from cache */
	if (cache_sup == GN_TRUE && (type_lookup & AUTOCACHE) && (cookie == 0 || type == CACHE_TYPE_TOC)) {

		/* if we got this locally, data will be in "info" */
		if (info) {
#if defined(GN_CACHE_DATA)
			void*		rawbuff2 = NULL;
			gn_size_t	rawsize2 = 0;

			/* reduce the amount of data being stored in the cache */
			our_err = gnxlt_reduce_raw(info, info_size, &rawbuff2, &rawsize2, 0);
			our_err = gncache_add_toc_data((gn_toc_hdr_t*)&toc_full, 2, rawbuff2, rawsize2);
			gnmem_free(rawbuff2);
#else
			/* we would only benefit from caching the results if it's a fuzzy match */
			/* or there were multiple exact matches */
			if (match == TLMATCH_Fuzzy || nresults > 1) {
				if (cookie)
					our_err = gncache_update_toc_id(cookie, toc_id);
				else
					our_err = gncache_add_toc_id((gn_toc_hdr_t*)&toc_full, toc_id);
			}
#endif
		}
		else if (xml) {
			/* for online lookup, data will be in structured XML */
			/* we must render it to a buffer before saving */
			info = RenderXMLTagToStrEx(xml, GN_FALSE, GN_FALSE, GN_FALSE);

			/* we also need the toc_id to keep this TOC from being updated */
			discid_tag = GetXMLSubTagFromStr(xml, GN_STR_DISCID);

			/* extract toc_id(MUID) from XML */
			if (discid_tag) {
				muid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MUID);
				if (muid_tag) {
					muid_str = GetXMLTagData(muid_tag);
					if (muid_str)
						toc_id = parse_buf_to_int32(muid_str, strlen(muid_str));
				}
			}
			/* just in case we couldn't get it, set to non-zero */
			if (toc_id == 0)
				toc_id = 2;

			/* now update, or add to cache */
			if (cookie)
				our_err = gncache_update_toc_data(cookie, toc_id, info, strlen(info) + 1);
			else
				our_err = gncache_add_toc_data((gn_toc_hdr_t*)&toc_full, toc_id, info, strlen(info) + 1);
		}
	}
	/* release any data from our work */
	if (cookie)
		gncache_release_data(cookie);
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

	if (xml)
		DisposeXMLTag(xml);
	if (info)
		gnmem_free(info);

	return our_err;
}


