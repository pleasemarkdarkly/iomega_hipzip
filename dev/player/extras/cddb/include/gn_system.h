/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_system.h -	Application interface for system initialization,
 *					shutdown, and integrity management.
 */


#ifndef _GN_SYSTEM_H_
#define _GN_SYSTEM_H_

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
 * Constants
 */

#define		gn_sys_error_t			gn_error_t


/*
 * Typedefs
 */

typedef gn_uint16_t gn_sys_pkg_error_t;


/*
 * Prototypes.
 */

/* Initialize eCDDB system. */
gn_sys_error_t
gnsys_initialize(void);

/* Shutdown eCDDB system. */
gn_sys_error_t
gnsys_shutdown(void);

/* Perform consistency check on eCDDB system. */
gn_sys_error_t
gnsys_self_check(void);

/* Recover eCDDB system after device failure. */
gn_sys_error_t
gnsys_recover(void);

/* Reset eCDDB system. */
gn_sys_error_t
gnsys_reset(void);

gn_sys_error_t
gnsys_reset_db(void);

gn_error_t
gnsys_revert_db(void);

#ifdef __cplusplus
}
#endif 

#endif /* #ifndef _GN_SYSTEM_H_ */
