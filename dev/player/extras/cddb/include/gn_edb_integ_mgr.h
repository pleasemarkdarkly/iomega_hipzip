/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_edb_integ_mgr.h - Integrity manager for the "Database Subsystem".
 */


#ifndef _GN_EDB_INTEG_MGR_H_
#define _GN_EDB_INTEG_MGR_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

/* File identifiers */
#define	EDBIMFID_Default	0
#define	EDBIMFID_Index		0x01
#define	EDBIMFID_Data		0x02
#define	EDBIMFID_Cache		0x04
#define	EDBIMFID_TypeMask	0x0F
#define	EDBIMFID_Live		0x10
#define	EDBIMFID_Backup		0x20
#define	EDBIMFID_Original	0x40
#define	EDBIMFID_LiveMask	0xF0
#define	EDBIMFID_Update		0x100
#define	EDBIMFID_Language	0x200
#define	EDBIMFID_WhatMask	0xFF00
#define	EDBIMFID_IdMask		0x7FFF0000


/* Basic combinations of file identifiers (for convenience) */
#define		EDBIMFID_LiveIndex			((gn_edb_im_fident_t)(EDBIMFID_Live|EDBIMFID_Index))
#define		EDBIMFID_LiveData			((gn_edb_im_fident_t)(EDBIMFID_Live|EDBIMFID_Data))
#define		EDBIMFID_LiveCache			((gn_edb_im_fident_t)(EDBIMFID_Live|EDBIMFID_Cache))
#define		EDBIMFID_LiveUpdateIndex	((gn_edb_im_fident_t)(EDBIMFID_Live|EDBIMFID_Update|EDBIMFID_Index))
#define		EDBIMFID_LiveUpdateData		((gn_edb_im_fident_t)(EDBIMFID_Live|EDBIMFID_Update|EDBIMFID_Data))
#define		EDBIMFID_BackupUpdateIndex	((gn_edb_im_fident_t)(EDBIMFID_Backup|EDBIMFID_Update|EDBIMFID_Index))
#define		EDBIMFID_BackupUpdateData	((gn_edb_im_fident_t)(EDBIMFID_Backup|EDBIMFID_Update|EDBIMFID_Data))
#define		EDBIMFID_BackupCache		((gn_edb_im_fident_t)(EDBIMFID_Backup|EDBIMFID_Cache))

#define		EDBIMFID_BackupIndex		((gn_edb_im_fident_t)(EDBIMFID_Backup|EDBIMFID_Index))
#define		EDBIMFID_BackupData			((gn_edb_im_fident_t)(EDBIMFID_Backup|EDBIMFID_Data))
#define		EDBIMFID_OriginalIndex		((gn_edb_im_fident_t)(EDBIMFID_Original|EDBIMFID_Index))


/* Options for operations performed by Integrity Manager subsystem */
#define	IMOPT_Default		0
#define	IMOPT_Lookup		0x001
#define	IMOPT_Update		0x002
#define	IMOPT_ActionMask	0x00F
#define	IMOPT_Embed_DB		0x010
#define	IMOPT_Cache_DB		0x020
#define	IMOPT_DBIdentMask	0x0F0
#define	IMOPT_NoRecovery	0x100
#define	IMOPT_OptionsMask	0xF00


/*
 * Structures and typedefs.
 */

typedef gn_uint32_t		gn_edb_im_fident_t;
typedef gn_uint32_t		gn_im_option_t;


/* integrity manager subsystem configuration structure */
typedef struct edb_im_config {
	gn_char_t	db_path[GN_MAX_PATH];
	gn_bool_t	set_db_path;
	gn_char_t	db_bkp_path[GN_MAX_PATH];
	gn_bool_t	set_db_bkp_path;
	gn_bool_t	no_update_backups;
	gn_bool_t	set_no_update_backups;
	gn_bool_t	no_cache_backups;
	gn_bool_t	set_no_cache_backups;
}
gn_edb_im_config_t;


/*
 * Prototypes.
 */

/* Initialize Integrity Manager. */
/* Call gntlu_initialize to initialize the integrity manager in lookup mode; */
/* or call gn_upd_update to initialize the integrity manager in update mode. */

/* Shutdown Integrity Manager. */
/* Call gntlu_shutdown to shut down the integrity manager in lookup mode; */
/* or, if in update mode, gn_upd_update will handle the shutdown. */

/* Set configuration information */
gn_error_t
gn_edb_im_configure(gn_edb_im_config_t* config);

/* File name retrieval functions */
/* Return pointer to common buffer with pathname of specified file */
/* NOTE: this buffer may be over-written by the next call to this function */
gn_cstr_t
gn_edb_im_get_file_path(gn_edb_im_fident_t identifier);

/* Return pointer to allocated buffer with pathname of specified file */
/* NOTE: this buffer must be freed by the caller of this function */
gn_cstr_t
gn_edb_im_copy_file_path(gn_edb_im_fident_t identifier);


/******************
 CRC-related Calls
 ******************/

gn_error_t
check_index_crc(gn_edb_im_fident_t identifier, gn_bool_t* crc_mismatch);

gn_error_t
crc32_file(gn_cstr_t filename, gn_uint32_t* new_crc, gn_uint32_t crc_offset);

gn_error_t
get_index_crc(gn_uint32_t* crc, gn_cstr_t override_filename);

gn_error_t
set_index_crc(gn_uint32_t new_crc, gn_cstr_t override_filename);

gn_error_t
get_index_crc_offset(gn_cstr_t override_filename, gn_uint32_t* crc_offset);

gn_error_t
reset_index_crc(gn_cstr_t override_filename);

gn_error_t
reset_file_crc(gn_cstr_t override_filename);

gn_error_t
check_file_crc(gn_cstr_t filename, gn_bool_t* crc_mismatch);


#ifdef __cplusplus
}
#endif 

#endif /* _GN_EDB_INTEG_MGR_H_ */
