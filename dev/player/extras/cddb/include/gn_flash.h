/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_flash.h - abstraction layer interface for accessing flash RAM
 *				for the non-volatile memory cache.
 */

#ifndef	_GN_FLASH_H_
#define _GN_FLASH_H_


/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */


/*
 * Constants
 */

typedef	void*	gn_flash_handle_t;


/*
 * Prototypes
 */

/* initialize access to the flash RAM area for the memory cache */
gn_error_t
gnflash_initialize(void* buffer, gn_size_t size, gn_flash_handle_t* handle);

void
gnflash_shutdown(gn_flash_handle_t handle);


#if defined(GN_FLASH_PAGE_IO)

/*
 * Read data from the specified offset in our area of flash RAM to a memory buffer.
 */
gn_error_t
gnflash_read(gn_flash_handle_t handle, gn_uint32_t offset, void* dest, gn_size_t size, gn_size_t* nread);

/*
 * Write data from a memory buffer to the specified offset in our area of flash RAM.
 */
gn_error_t
gnflash_write(gn_flash_handle_t handle, gn_uint32_t offset, void* src, gn_size_t size, gn_size_t* nwritten);

/*
 * Move data within our area of flash RAM.
 */
gn_error_t
gnflash_move(gn_flash_handle_t handle, gn_uint32_t dest, gn_uint32_t src, gn_size_t size);

/*
 * Commit any buffered data into flash RAM.
 */
gn_error_t
gnflash_commit(gn_flash_handle_t handle);

#endif /* #if defined(GN_FLASH_PAGE_IO) */

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _GN_FLASH_H_ */

