/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_cache.h - Application and mid-level interface definition for
 *				the module supporting local caching of online and local
 *				lookup information.
 */


#ifndef _GN_CACHE_H_
#define _GN_CACHE_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_lookup.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

/* Options for operations performed by Local Data Cache subsystem */
#define CACHE_OPT_Default		0
#define CACHE_OPT_Lookup		0x1
#define CACHE_OPT_Update		0x2
#define CACHE_OPT_Recover		0x4
#define CACHE_OPT_Create		0x10
#define CACHE_OPT_AutoOpen		0x20
#define CACHE_OPT_AutoCreate	0x40

/* Values returned by callbacks */
#define CACHE_CB_None			0		/* keep on chugging */
#define CACHE_CB_Abort			-1		/* stop iterating through records */
#define CACHE_CB_Delete			1		/* delete current record */
#define CACHE_CB_DontFree		2		/* don't release memory for current record */

/* Types of data stored in the Local Data Cache */
#define CACHE_TYPE_TOC			1		/* TOC-only	*/
#define CACHE_TYPE_TOC_ID		2		/* TOC with remembered toc_id */
#define CACHE_TYPE_DATA			3		/* TOC with data from on-line lookup */


/*
 * Structures and typedefs.
 */

typedef gn_error_t		gn_cache_error_t;
typedef gn_uint32_t		gn_cache_option_t;
typedef gn_int16_t		gn_cache_action_t;
typedef gn_int16_t		gn_cache_rec_type_t;

/* Opaque type for accessing raw data records */
typedef void *gn_cache_data_t;


/*
 * Prototypes.
 */

/* Signature for "callback" function supplied when looking up records */
typedef gn_cache_action_t (*CACHECallBackFunc)(gn_cache_data_t cookie);


/* Set configuration information for Local Data Cache subsystem. */
gn_cache_error_t
gncache_configure(void *buffer, gn_size_t size);

/* Initialize Local Data Cache subsystem. */
gn_cache_error_t
gncache_initialize(gn_cache_option_t options);

/* Shutdown Local Data Cache subsystem. */
gn_cache_error_t
gncache_shutdown(void);

/* Delete Local Data Cache. */
gn_cache_error_t
gncache_delete(gn_bool_t backup);


/*
 * Lookup-related functions.
 */
/* Lookup (first) record matching passed toc. */
gn_cache_error_t
gncache_lookup_toc(gn_toc_hdr_t *toc, gn_cache_data_t *cookie);

/* Lookup records matching passed toc. */
gn_cache_error_t
gncache_lookup_tocs(gn_toc_hdr_t *toc, CACHECallBackFunc callback);

/* Lookup record with passed toc id. */
gn_cache_error_t
gncache_lookup_toc_id(gn_uint32_t toc_id, gn_cache_data_t *cookie);

/* Lookup records with passed toc id. */
gn_cache_error_t
gncache_lookup_toc_ids(gn_uint32_t *toc_id, CACHECallBackFunc callback);

/* Get type of data associated with passed cookie. */
gn_cache_error_t
gncache_data_type(gn_cache_data_t cookie, gn_cache_rec_type_t *type);

/* Get size of data associated with passed cookie. */
gn_cache_error_t
gncache_data_size(gn_cache_data_t cookie, gn_size_t *size);

/* Get TOC associated with passed cookie. */
gn_cache_error_t
gncache_data_toc(gn_cache_data_t cookie, gn_toc_hdr_t **toc);

/* Get toc_id associated with passed cookie. */
gn_cache_error_t
gncache_data_toc_id(gn_cache_data_t cookie, gn_uint32_t *toc_id);

/* Retrieve data associated with passed cookie. */
gn_cache_error_t
gncache_retrieve_data(gn_cache_data_t cookie, void *buffer, gn_size_t size);

/* Allocate buffer and retrieve data associated with passed cookie. */
gn_cache_error_t
gncache_retrieve_data_alloc(gn_cache_data_t cookie, void **buffer, gn_size_t *size);

/* Release cookie. */
gn_cache_error_t
gncache_release_data(gn_cache_data_t cookie);


/*
 * Update-related functions.
 */
/* Add TOC with data to cache */
gn_cache_error_t
gncache_add_toc_data(gn_toc_hdr_t *toc, gn_uint32_t toc_id, void *buffer, gn_size_t size);

/* Update TOC data in cache */
gn_cache_error_t
gncache_update_toc_data(gn_cache_data_t cookie, gn_uint32_t toc_id, void *buffer, gn_size_t size);

/* Add TOC with toc_id (may be zero) to cache */
gn_cache_error_t
gncache_add_toc_id(gn_toc_hdr_t *toc, gn_uint32_t toc_id);

/* Update toc_id for TOC in cache */
gn_cache_error_t
gncache_update_toc_id(gn_cache_data_t cookie, gn_uint32_t toc_id);

/* Delete TOC from cache */
gn_cache_error_t
gncache_delete_toc(gn_cache_data_t cookie, gn_toc_hdr_t *toc);


#ifdef __cplusplus
}
#endif 

#endif /* _GN_CACHE_H_ */
