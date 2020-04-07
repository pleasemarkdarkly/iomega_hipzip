/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * proto_database.h - Interface definition for the Prototype "Database Subsystem".
 */


#ifndef _PROTO_DATABASE_H_
#define _PROTO_DATABASE_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_tocinfo.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

#define	EDB_NUM_PKG_ERRS	1	/* number of enums in edb_pkg_error */

/* Options for operations performed by Embedded Database subsystem */
#define	EDBOPT_Default		0,
#define	EDBOPT_Lookup		0x1
#define	EDBOPT_Update		0x2
#define	EDBOPT_Recover		0x4
#define	EDBOPT_GetData		0x10

/* values returned by callbacks */
#define	EDBCB_None			0		/* keep on chugging */
#define	EDBCB_Abort			-1		/* stop iterating through records */
#define	EDBCB_Delete		1		/* delete current record */
#define	EDBCB_DontFree		2		/* don't release memory for current record */


/*
 * Structures and typedefs.
 */

typedef gn_uint32_t	gn_edb_error_t;
typedef gn_uint32_t	gn_edb_option_t;
typedef gn_int16_t	gn_edb_action_t;

/* Opaque type for accessing raw data records */
typedef void *gnedb_data_t;


/* TODO: define these types in appropriate header */
#if !defined(gn_mid_t)
#define	gn_mid_t	void
#endif
#if !defined(gn_fval_t)
#define	gn_fval_t	void
#endif


/*
 * Prototypes.
 */

/* Signature for "callback" function supplied when looking up records */
typedef gn_edb_action_t (*EDBMidCallBackFunc)(gn_toc_hdr_t* toc, gn_uint32_t toc_id, gnedb_data_t cookie);


/* Initialize Embedded Database subsystem. */
gn_edb_error_t
gnedb_initialize(gn_edb_option_t options);

/* Shutdown Embedded Database subsystem. */
gn_edb_error_t
gnedb_shutdown(void);


/*
 * Lookup-related functions.
 */

/* Lookup records matching passed media id. */
gn_edb_error_t
gnedb_lookup_mid(gn_mid_t *mid, EDBMidCallBackFunc callback);

/* Lookup record with passed toc id. */
gn_edb_error_t
gnedb_lookup_toc_id(gn_uint32_t toc_id, void **data, gn_size_t *size);

/* Retrieve data associated with passed cookie. */
gn_edb_error_t
gnedb_retrieve_data(gnedb_data_t cookie, void **data, gn_size_t *size);

/* Release cookie. */
gn_edb_error_t
gnedb_release_data(gnedb_data_t cookie);


/*
 * Update-related functions.
 */

/* Add new disc data and index records to local database */
gn_edb_error_t
gnedb_add_records(gn_uint32_t count, gn_uint32_t *toc_ids, gn_mid_t *mids, gn_fval_t *fvals, gn_toc_hdr_t **tocs, void *data, gn_size_t data_size);

/* Add updated disc data and index records to local database */
gn_edb_error_t
gnedb_update_records(gn_uint32_t count, gn_uint32_t *toc_ids, gn_mid_t *mids, gn_fval_t *fvals, gn_toc_hdr_t **tocs, void *data, gn_size_t data_size);

/* Mark disc records as bad */
gn_edb_error_t
gnedb_invalidate_records(gn_uint32_t count, gn_uint32_t *toc_ids);

/* Finish update - close files, recalculate CRC on index */
gn_edb_error_t
gn_update_complete(void);

#ifdef __cplusplus
}
#endif 

#endif /* _PROTO_DATABASE_H_ */
