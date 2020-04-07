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

#include	<extras/cddb/common/gn_platform.h>
#include    <extras/cddb/common/gn_defines.h>
#include    <extras/cddb/abstract_layer/gn_errors.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants
 */

#define		gn_sys_error_t			gn_error_t

#define		SYS_NUM_PKG_ERRS		1	/* number of enums in sys_pkg_error */

/* Package specific errors returned from System Manager */
#define _SYSERR_InitError		1

/* Errors returned from TOC lookup subsystem */
#define		SYSERR_NoError			SUCCESS
#define		SYSERR_NotInited		GNERR_PKG_GEN_ERR(GNPKG_System, GNERR_NotInited)
#define		SYSERR_Busy				GNERR_PKG_GEN_ERR(GNPKG_System, GNERR_Busy)
#define		SYSERR_MemoryError		GNERR_PKG_GEN_ERR(GNPKG_System, GNERR_NoMemory)
#define		SYSERR_InitError		GNERR_PKG_ERR(GNPKG_System, _SYSERR_InitError)


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

/* Check subsystems for initialization problems. */
gn_sys_error_t
gnsys_init_check(gnerr_pkg_id_t *ids, gn_uint32_t *num_ids);

/* Recover eCDDB system after device failure. */
gn_sys_error_t
gnsys_recover(void);

/* Reset eCDDB system. */
gn_sys_error_t
gnsys_reset(void);

gn_sys_error_t
gnsys_reset_db(void);

#ifdef __cplusplus
}
#endif 

#endif /* #ifndef _GN_SYSTEM_H_ */
