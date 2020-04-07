/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_crypt.h - abstraction layer interface for initialization of
 * of device's cryptographic space.
 */

#ifndef	_GN_CRYPT_H_
#define _GN_CRYPT_H_


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

typedef gn_error_t gn_crypt_error_t;


/*
 * Prototypes
 */

/* initialize cryptographic space */
gn_crypt_error_t
gn_crypt_init(void* crypt_buffer, gn_size_t buffer_size);

void
gn_crypt_shutdown(void);

/*
 * Read the encryption key area from ROM.
 * The memory should be freed using gnmem_free after it has been passed
 * to gn_crypt_init().  This function must be reimplemented for each platform.
 */
gn_crypt_error_t
gn_crypt_read_key_area(void** crypt_buffer, gn_size_t* buffer_size);

/* 
 * Generate cryptographically random data. 
 * This function must be reimplemented for each platform,
 * since the source of and manner of access for truely random numbers
 * differs from device to device.
 */
gn_crypt_error_t
gn_crypt_generate_random(gn_uchar_t* buffer, gn_size_t buffer_size);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* _GN_CRYPT_H_ */

