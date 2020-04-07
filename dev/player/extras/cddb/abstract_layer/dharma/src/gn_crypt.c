/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_crypt.c - Implementation for public interface for encryption / decryption.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_crypt.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_fs.h>
#include GN_TIME_H
#include GN_STDLIB_H

#include <util/debug/debug.h>

// The cddb key is stored on flash, so ask the flash manager for it
#include <fs/flash/flashmanager.h>

// Length of the CDDB key
#define CDDB_KEY_LENGTH     256

/*
 * Variables.
 */

/* TEMP: remove when ported to device and reading ROM	*/
//static gn_str_t keyfilename = "ecddb.ka";

/*
 * Implementations.
 */

/*
 * Read the encryption key area from ROM.
 * The memory should be freed using gnmem_free after it has been passed
 * to gn_crypt_init().  This function must be reimplemented for each platform.
 */
gn_crypt_error_t
gn_crypt_read_key_area(void** crypt_buffer, gn_size_t* buffer_size)
{
#if 1
//    int i;
    struct fis_image_desc* desc = flash_fs_find_image("key");

    if( desc == NULL )
    {
        // lie about the issue
        return GNERR_ERR_CODE(CRYPTERR_MemoryError);
    }

	*crypt_buffer = gnmem_malloc(CDDB_KEY_LENGTH);
	if (*crypt_buffer == NULL)
    {
		return GNERR_ERR_CODE(CRYPTERR_MemoryError);
	}

    memcpy(*crypt_buffer, desc->flash_base, CDDB_KEY_LENGTH);
    *buffer_size = CDDB_KEY_LENGTH;

/*
    diag_printf("**** CDDB KEY: ****\n");
    for (i = 0; i < CDDB_KEY_LENGTH; ++i)
    {
        diag_printf("%02x ", ((char*)*crypt_buffer)[i]);
        if ((i + 1) % 16 == 0)
            diag_printf("\n");
        else if ((i + 1) % 8 == 0)
            diag_printf(" ");
    }
    diag_printf("*******************\n");
*/

	return CRYPTERR_NoError;

#else

	/* THE FOLLOWING CODE IS FOR ILLUSTRATION PURPOSES ONLY */
	/* THE DEVELOPER MUST IMPLEMENT THE REAL CODE WHICH READS
		THE KEY AREA FROM ROM */

	gn_handle_t		key_file = FS_INVALID_HANDLE;
	gn_size_t		size = 0;
	gn_foffset_t	eof = FS_INVALID_OFFSET;

	/* look in the current directory */
	key_file = gnfs_open(keyfilename, FSMODE_ReadOnly);
	if (key_file == FS_INVALID_HANDLE)
		return GNERR_ERR_CODE(gnfs_get_error());

	eof = gnfs_get_eof(key_file);
	size = eof;

	*crypt_buffer = gnmem_malloc(size);
	if (*crypt_buffer == NULL) {
		gnfs_close(key_file);
		return GNERR_ERR_CODE(CRYPTERR_MemoryError);
	}

	*buffer_size = gnfs_read(key_file, *crypt_buffer, size);
	gnfs_close(key_file);
	if (*buffer_size != size) {
		gnmem_free(*crypt_buffer);
		*crypt_buffer = NULL;
		return GNERR_ERR_CODE(CRYPTERR_IOError);
	}
	return CRYPTERR_NoError;

#endif
}


/* 
 * Generate cryptographically random data. 
 * This function must be reimplemented for each platform,
 * since the source of and manner of access for truely random numbers
 * differs from device to device.
 */
gn_crypt_error_t
gn_crypt_generate_random(gn_uchar_t* buffer, gn_size_t buffer_size)
{
	/*
		A RATHER BOGUS PLACE-HOLDER (DON'T USE THE CLOCK FOR SEEDS)
		PLEASE, PLEASE, PLEASE PUT IN SOME REAL RANDOM NUMBER GENERATION HERE
	*/
	gn_uint16_t	i = 0;

/*	srand((unsigned int)time(NULL)); */
	for (i = 0; i < buffer_size; i++)
		*buffer++ = (gn_uchar_t)rand();

	return CRYPTERR_NoError;
}

