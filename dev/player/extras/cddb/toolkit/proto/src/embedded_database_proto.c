/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * embedded_database_proto.c - protype implementation of the "Database Subsystem" module.
 */

#if	!defined(MIDSZ)
#define MIDSZ		16	/* Size of a media ID; same as MD5 digest. */
#define MIDASCSZ	32	/* Size of a media ID in ascii hex form. */
#endif

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDLIB_H

#include "proto_database.h"

#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_memory.h>
#include "hid.h"


static gn_bool_t	edb_inited = GN_FALSE;


/* Initialize Embedded Database subsystem. */
gn_edb_error_t
gnedb_initialize(gn_edb_option_t options)
{
	if (edb_inited != GN_FALSE)
		return EDBERR_Busy;

	edb_inited = GN_TRUE;

	return EDBERR_NoError;
}

/* Shutdown Embedded Database subsystem. */
gn_edb_error_t
gnedb_shutdown(void)
{
	if (edb_inited == GN_FALSE)
		return EDBERR_NotInited;

	edb_inited = GN_FALSE;

	return EDBERR_NoError;
}



/*
 * Lookup-related functions.
 */

/* Lookup records matching passed media id. */
gn_edb_error_t
gnedb_lookup_mid(gn_mid_t *mid, EDBMidCallBackFunc callback)
{
	gn_handle_t			handle;
	gn_char_t			filename[HIDASCSZ*4+1];
	gn_edb_action_t		action;
	gn_toc_hdr_t		tochdr;

	if (edb_inited == GN_FALSE)
		return EDBERR_NotInited;

	/* For now, the data is stored in a file named with the media id */
	hid2asc((hid_t*)mid, filename);
	filename[HIDASCSZ] = 0;	/* null terminate */

	/* Read the TOC from the passed file. */
	handle = gnfs_open(filename, FSMODE_ReadOnly);
	if (handle == -1)
		return EDBERR_NotFound;

	/* call back with single result */
	tochdr.num_offsets = 0;
	action = (*callback)(&tochdr, *((gn_uint32_t*)mid), (gnedb_data_t)handle);
	if (action != EDBCB_DontFree) {
		gnfs_close(handle);
	}

	return EDBERR_NoError;
}



/* Lookup record with passed toc id. */
gn_edb_error_t
gnedb_lookup_toc_id(gn_uint32_t toc_id, void **data, gn_size_t *size)
{
	return EDBERR_NoError;
}


/* Retrieve data associated with passed cookie. */
gn_edb_error_t
gnedb_retrieve_data(gnedb_data_t cookie, void **data, gn_size_t *size)
{
	gn_size_t		data_size, data_read;
	gn_foffset_t	file_size;
	gn_handle_t		handle = (gn_handle_t)cookie;
	gn_handle_t		handle2 = -1;
	gn_char_t		file_name[16];
	gn_str_t		tmp;

	file_size = gnfs_get_eof(handle);
	if (file_size == FS_INVALID_OFFSET)
		return EDBERR_DataError;

	/* if file_size is 9, this file contains name of file with data */
	if (file_size == 9) {
		data_read = gnfs_read(handle, file_name, 9);
		if (data_read != 9)
			return EDBERR_DataError;

		handle2 = gnfs_open(file_name, FSMODE_ReadOnly);
		if (handle2 == -1)
			return EDBERR_DataError;

		handle = handle2;

		file_size = gnfs_get_eof(handle);
		if (file_size == FS_INVALID_OFFSET)
			return EDBERR_DataError;
	}

	/* arbitrary limit for prototype */
	if (file_size > 32768)
		data_size = 32768;
	else
		data_size = file_size + 1;
	*data = gnmem_malloc(data_size);
	if (*data == NULL) {
		if (handle2 != -1)
			gnfs_close(handle2);
		return EDBERR_MemoryError;
	}

	data_read = gnfs_read_at(handle, (gn_foffset_t)0, *data, data_size);
	if (handle2 != -1)
		gnfs_close(handle2);
	if (data_read < data_size - 1) {
		gnmem_free(*data);
		*data = NULL;
		return EDBERR_DataError;
	}

	/* null terminate for convenience */
	tmp = (gn_str_t)*data;
	*(tmp + data_size - 1) = 0;

	*size = data_size;
	return EDBERR_NoError;
}


/* Release cookie. */
gn_edb_error_t
gnedb_release_data(gnedb_data_t cookie)
{
	gnfs_close((gn_handle_t)cookie);
	return EDBERR_NoError;
}



