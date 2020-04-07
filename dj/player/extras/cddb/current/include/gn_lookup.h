/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_lookup.h - Application interface definition for the "toc lookup" module.
 */


#ifndef _GN_LOOKUP_H_
#define _GN_LOOKUP_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_tocinfo.h>
#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_comm.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

/* Options for doing TOC lookups */
#define	TLOPT_Default		0
#define	TLOPT_AutoFuzzy		0x1			/* Pick best choice for fuzzy matches */
#define	TLOPT_ExactOnly		0x8			/* Don't perform fuzzy lookup if not exact */

/* Match types for TOC lookups */
#define	TLMATCH_None		0			/* No match */
#define	TLMATCH_Exact		1			/* Exact match(s) */
#define	TLMATCH_Fuzzy		2			/* One or more fuzzy matches */

/* Sources for successful TOC lookups */
#define	TLSRC_None			0			/* No match */
#define	TLSRC_LocalDB		1			/* Local embedded DB */
#define	TLSRC_Cache			2			/* Local cache of results */
#define	TLSRC_Online		3			/* On-line service */

/* opaque structure holding results from TOC or ID lookups */
typedef struct tlu_result gn_tlu_result_t;


/*
 * Typedefs
 */

typedef gn_error_t		gn_tlu_error_t;
typedef gn_uint32_t		gn_tlu_options_t;
typedef gn_uint32_t		gn_tlu_match_t;
typedef gn_int16_t		gn_tlu_source_t;


/*
 * Prototypes.
 */

/* Initialize TOC lookup subsystem. */
gn_tlu_error_t
gntlu_initialize(void);

/* Shutdown TOC lookup subsystem. */
gn_tlu_error_t
gntlu_shutdown(void);

/* Configure online lookup options */
gn_error_t
gntlu_configure_online_lookup(gn_cstr_t new_server_url, gn_cstr_t new_result_charset, gn_cstr_t new_result_encoding);

/* Set callback function for online lookups. */
gn_tlu_error_t
gntlu_set_lookup_cb(gncomm_callback_t callback, gn_uint32_t user_data);

/* Cancel active online lookup. */
gn_tlu_error_t
gntlu_cancel_lookup(void);

/* Look up a TOC online. If lu_options is TLOPT_AutoFuzzy, exactly zero or one   */
/* match will be returned, and match_type will be TLMATCH_None or TLMATCH_Exact, */
/* even if the actual lookup resulted in a fuzzy match. If lu_options is         */
/* something else, there is the possibility of multiple matches, and match_type. */
/* could also be TLMATCH_Fuzzy. If the lookup result is an exact match, xml will */
/* be an ALB element. If there is a multiple match, xml will be an ALBS element. */
/* If there is no match, xml will be null. xml should be freed by the caller     */
/* with DisposeXMLTag. The server will guarantee that the plain-text results     */
/* will be no more than max_results_size bytes long. This value should be about  */
/* 1/4 of the memory usage desired for the online lookup. If you pass zero for   */
/* max_results_size, the server will not limit the size of the result.           */
gn_error_t
gntlu_lookup_online_ex(gn_toc_info_t *toc, gn_tlu_options_t lu_options,
                       gn_size_t max_results_size, XMLTagRef *xml, gn_tlu_match_t* match_type);

/* Look up a TOC online. This function simply calls gntlu_lookup_online_ex with  */
/* lu_options of TLOPT_AutoFuzzy and a max_results_size of zero.                 */
gn_tlu_error_t
gntlu_lookup_online(gn_toc_info_t* toc, XMLTagRef* xml, gn_tlu_match_t* match_type);

/* Look up a disc using the toc_id/media_id online. These values are returned    */
/* from a TOC lookup with fuzzy results (MUID/MEDIAID tags of the DISCID subtag  */
/* of the ALB element).         */
gn_tlu_error_t
gntlu_lookup_id_online(gn_cstr_t toc_id, gn_cstr_t media_id, XMLTagRef* xml);


/* Lookup the passed TOC in the local database (use toc_id if passed). */
gn_tlu_error_t
gntlu_lookup(gn_toc_info_t *toc, gn_uint32_t toc_id, gn_tlu_options_t options, gn_tlu_match_t *match_type, gn_tlu_result_t ***results, gn_uint32_t *numresults);

/* Release results from previous lookup. */
gn_tlu_error_t
gntlu_release(gn_tlu_result_t **results, gn_uint32_t numresults);


/* Get result ranking (0 is best) */
gn_uint32_t
gntlu_result_rank(gn_tlu_result_t *result);

/* Get result source */
gn_tlu_source_t
gntlu_result_source(gn_tlu_result_t *result);

/* Get TOC for this result (local DB only) */
gn_toc_info_t*
gntlu_result_toc(gn_tlu_result_t *result);

/* Get toc_id for this result (for fuzzy match selection) */
gn_uint32_t
gntlu_result_toc_id(gn_tlu_result_t *result);

/* Get size of XML data for this result */
gn_size_t
gntlu_result_info_size(gn_tlu_result_t *result);

/* Get XML data for this result into passed buffer */
gn_tlu_error_t
gntlu_result_info(gn_tlu_result_t *result, void *info, gn_size_t size);

/* Get XML data for this result into allocated buffer - info freed using gnmem_free() */
gn_tlu_error_t
gntlu_result_info_alloc(gn_tlu_result_t *result, void **info, gn_size_t *size);


#ifdef __cplusplus
}
#endif 

#endif /* _GN_LOOKUP_H_ */
