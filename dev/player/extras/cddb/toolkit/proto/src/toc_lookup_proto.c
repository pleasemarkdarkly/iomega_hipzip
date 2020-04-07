/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * toc_lookup_proto.c - protype implementation of the "TOC Lookup" module.
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDLIB_H
#include <extras/cddb/gn_memory.h>
#include "proto_lookup.h"
#include <extras/cddb/gn_lookup.h>
#include "proto_database.h"
#include "hid.h"

/*
 * Variables.
 */

/* global to hold results (only one supported) */
gn_tlu_result_t	OurResult;
gn_tlu_result_t *OurResults = &OurResult;

static gn_uint32_t	busy = 0;
static gn_bool_t	tlu_inited = GN_FALSE;


/* Initialize TOC lookup subsystem. */
gn_tlu_error_t
gntlu_initialize(void)
{
	gn_edb_error_t		dberror;

	if (tlu_inited != GN_FALSE)
		return TLERR_Busy;

	/* Initialize structure for holding lookup results */
	gnmem_memset(&OurResult, 0, sizeof(OurResult));

	dberror = gnedb_initialize(EDBOPT_Lookup);
	if (dberror != EDBERR_NoError)
	{
		return dberror;
	}

	tlu_inited = GN_TRUE;

	return TLERR_NoError;
}

/* Shutdown TOC lookup subsystem. */
gn_tlu_error_t
gntlu_shutdown(void)
{
	if (tlu_inited == GN_FALSE)
		return TLERR_NotInited;

	gnedb_shutdown();

	gntlu_release(&OurResults, 1);

	tlu_inited = GN_FALSE;

	return TLERR_NoError;
}


/* Perform consistency check on TOC lookup subsystem. */
gn_tlu_error_t
gntlu_self_check(void)
{
	/* not implemented in prototype */
	return TLERR_NoError;
}

/* Call back for lookup performed below */
gn_edb_action_t OurCallBackFunc(gn_toc_hdr_t* toc, gn_uint32_t toc_id, gnedb_data_t cookie)
{
	gn_uint32_t		n;

	/* only accept first result */
	if (OurResult.toc.num_offsets != 0 || OurResult.toc_id != 0)
		return EDBCB_Abort;

	/* copy toc & other info */
	OurResult.toc.num_offsets = toc->num_offsets;
	for (n = 0; n < toc->num_offsets; n++)
		OurResult.toc.offsets[n] = toc->offsets[n];

	OurResult.toc_id = toc_id;
	OurResult.reserved = cookie;

	/* keep this around for later */
	return EDBCB_DontFree;
}


/* Lookup the passed TOC (use toc_id if passed). */
gn_tlu_error_t
gntlu_lookup(gn_toc_info_t *toc, gn_uint32_t toc_id, gn_tlu_options_t options, gn_tlu_match_t *match, gn_tlu_result_t ***results, gn_uint32_t *numresults)
{
	gn_edb_error_t	dberror;
	hid_t			hid;

	/* simple reentry protection */
	if (busy != 0)
		return TLERR_Busy;

	busy = 1;
	*numresults = 0;
	*match = TLMATCH_None;
	gntlu_release(&OurResults, 1);

	/*
	 * We don't do exact lookups for CD singles. This is almost guaranteed
	 * to give incorrect results. Two or more tracks are always required
	 * to provide sufficient certainty of an exact match.
	 */
	if(toc->num_offsets == MINOFFSETS) {
		busy = 0;
		return TLERR_NoError;
	}

	/* Generate a hash ID from the offsets. */
	gen_hid(&toc->offsets[0], toc->num_offsets, &hid);

	/* Do a lookup in the database for this hash ID. */
	dberror = gnedb_lookup_mid(&hid, OurCallBackFunc);
	if (dberror != EDBERR_NoError) {
		busy = 0;
		return dberror;
	}

	/* not found in our data set */
	if (OurResult.toc_id == 0) {
		busy = 0;
		return TLERR_NoError;
	}

	/* since we're just handling a single match, go ahead and fetch the data */
	dberror = gnedb_retrieve_data(OurResult.reserved, &OurResult.info, &OurResult.info_size);
	if (dberror != EDBERR_NoError) {
		gntlu_release(&OurResults, 1);
		busy = 0;
		return dberror;
	}

	/* release this resource */
	gnedb_release_data(OurResult.reserved);
	OurResult.reserved = 0;

	/* return the data */
	*numresults = 1;
	*match = TLMATCH_Exact;
	*results = &OurResults;
	busy = 0;

	return TLERR_NoError;
}

/* Release results from previous lookup. */
gn_tlu_error_t
gntlu_release(gn_tlu_result_t **results, gn_uint32_t numresults)
{
	/* release the cookie */
	if (results == &OurResults) {
		if (OurResult.reserved != 0) {
			gnedb_release_data(OurResult.reserved);
		}
		if (OurResult.info != NULL) {
			gnmem_free(OurResult.info);
		}
		gnmem_memset(&OurResult, 0, sizeof(OurResult));
	}
	else {
		/* release all the results */
	}
	return TLERR_NoError;
}


/* Get result ranking (0 is best) */
gn_uint32_t
gntlu_result_rank(gn_tlu_result_t *result)
{
	return result->rank;
}


/* Get TOC for this result (local DB only) */
gn_toc_info_t*
gntlu_result_toc(gn_tlu_result_t *result)
{
	return &result->toc;
}


/* Get toc_id for this result (for fuzzy match selection) */
gn_uint32_t
gntlu_result_toc_id(gn_tlu_result_t *result)
{
	return result->toc_id;
}


/* Get size of XML data for this result */
gn_size_t
gntlu_result_info_size(gn_tlu_result_t *result)
{
	return result->info_size;
}


/* Get XML data for this result into passed buffer */
gn_tlu_error_t
gntlu_result_info(gn_tlu_result_t *result, void *info, gn_size_t size)
{
	if (size > result->info_size)
		size = result->info_size;
	gnmem_memcpy(info, result->info, size);
	return TLERR_NoError;
}


/* Get XML data for this result into allocated buffer - info freed using gnmem_free() */
gn_tlu_error_t
gntlu_result_info_alloc(gn_tlu_result_t *result, void **info, gn_size_t *size)
{
	*info = gnmem_malloc(result->info_size);
	if (*info == NULL)
		return TLERR_MemoryError;
	*size = result->info_size;
	gnmem_memcpy(*info, result->info, *size);
	return TLERR_NoError;
}


