/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_cachex.h - Application and mid-level interface definition for
 *				 the module supporting local caching of external data.
 *
 * External Data Caches use the same database engine as the eCDDB/dCDDB
 * lookup layer to store arbitrary data.  There is very little overhead
 * in terms of code and file space required to store and access the data.
 * External Data Caches support three keys, two of which are of type
 * gn_uint32_t.  They need not be unique, but if applications assign
 * duplicate keys they must use the appropriate APIs to manage situations
 * where there are multiple cache entries with the same key.  The third key
 * is an arbitrary amount of binary data, which could be a character string
 * or some other value.  The data itself is not interpreted by the cache
 * support code, but the cache layer does support storing a comment with
 * each data record and a revision number.
 *
 */


#ifndef _GN_CACHEX_H_
#define _GN_CACHEX_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_cache.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

/* See also gn_cache.h */

#define CACHEX_DB_Format		1


/*
 * Structures and typedefs.
 */

/* Opaque type for accessing data records */
typedef void* gn_cachex_data_t;

/* Discriminator for managing multiple caches w/ different data */
typedef	gn_cstr_t	gn_cachex_id_t;

/* Handle for accessing cache functionality */
typedef void* gn_cachex_handle_t;

/* The two "id" keys for each record */
typedef gn_uint32_t	gn_cachex_id_t;


/*
 * Prototypes.
 */

/* Signature for "callback" function supplied when looking up records */
typedef gn_cache_action_t (*CACHEXCallBackFunc)(gn_cachex_data_t cookie);


/* Initialize Local External Data Cache. */
gn_error_t
gncache_initialize(gn_cachex_id_t identifier, gn_cache_option_t options, gn_cachex_handle_t* handle);

/* Shutdown Local External Data Cache. */
gn_error_t
gncache_shutdown(gn_cachex_handle_t handle);

/* Delete Local External Data Cache. */
gn_error_t
gncache_delete(gn_cachex_id_t identifier);

/* ??? open & close ??? */

/*
 * Lookup-related functions.
 */
/* Lookup (first) record matching passed Gracenote Id. */
gn_error_t
gncachex_lookup_gnid(gn_cachex_handle_t handle, gn_cachex_id_t gnid, gn_cachex_data_t *cookie);

/* Lookup records matching passed Gracenote Id (or all if gnid == NULL). */
gn_error_t
gncachex_lookup_gnids(gn_cachex_handle_t handle, gn_cachex_id_t *gnid, CACHEXCallBackFunc callback);

/* Lookup (first) record matching passed external id. */
gn_error_t
gncachex_lookup_xid(gn_cachex_handle_t handle, gn_cachex_id_t xid, gn_cachex_data_t *cookie);

/* Lookup records matching passed external id (or all if xid == NULL). */
gn_error_t
gncachex_lookup_xids(gn_cachex_handle_t handle, gn_cachex_id_t *xid, CACHEXCallBackFunc callback);

/* Lookup (first) record matching passed opaque key. */
gn_error_t
gncachex_lookup_key(gn_cachex_handle_t handle, void* key, gn_size_t key_size, gn_cachex_data_t *cookie);

/* Lookup records matching passed opaque key (or all if key == NULL). */
gn_error_t
gncachex_lookup_keys(gn_cachex_handle_t handle, void* key, gn_size_t key_size, CACHEXCallBackFunc callback);

/* Get revision of data associated with passed cookie. */
gn_error_t
gncachex_revision(gn_cachex_data_t cookie, gn_cache_rec_type_t *type);

/* Get size of data associated with passed cookie. */
gn_error_t
gncachex_data_size(gn_cachex_data_t cookie, gn_size_t *size);

/* Get data associated with passed cookie. */
gn_error_t
gncachex_data(gn_cachex_data_t cookie, void *data, gn_size_t size);

/* Allocate space and get data associated with passed cookie. */
gn_error_t
gncachex_data_alloc(gn_cachex_data_t cookie, void **data, gn_size_t *size);

/* Get comment associated with passed cookie. */
gn_error_t
gncachex_comment_size(gn_cachex_data_t cookie, gn_size_t *size);

/* Get comment associated with passed cookie. */
gn_error_t
gncachex_comment(gn_cachex_data_t cookie, void *comment, gn_size_t size);

/* Get comment associated with passed cookie. */
gn_error_t
gncachex_comment_alloc(gn_cachex_data_t cookie, void **comment, gn_size_t *size);

/* Get gn_id associated with passed cookie. */
gn_error_t
gncachex_data_gn_id(gn_cachex_data_t cookie, gn_cachex_id_t *gn_id);

/* Release cookie. */
gn_error_t
gncachex_release_data(gn_cachex_data_t cookie);


/*
 * Add/Update/Delete functions.
 */
/* Add entry (no data) to cache */
gn_error_t
gncachex_add_entry(gn_cachex_handle_t handle, gn_cachex_id_t gn_id, gn_cachex_id_t xid, void *key, gn_size_t size);

/* Add entry with data to cache */
gn_error_t
gncachex_add_entry_data(gn_cachex_handle_t handle, gn_cachex_id_t gn_id, gn_cachex_id_t xid, void *key, gn_size_t size, gn_uint32_t revision, void *comment, gn_size_t comment_size, void *data, gn_size_t data_size);

/* Update data in cache */
gn_error_t
gncachex_update_entry(gn_cachex_data_t cookie, gn_uint32_t revision, void *comment, gn_size_t comment_size, void *data, gn_size_t data_size);

/* Delete entry from cache (cookie not released). */
gn_error_t
gncachex_delete(gn_cachex_handle_t handle, gn_cachex_data_t cookie, gn_cachex_id_t gn_id, gn_cachex_id_t xid, void *key, gn_size_t size);


#ifdef __cplusplus
}
#endif 

#endif /* _GN_CACHEX
_H_ */
