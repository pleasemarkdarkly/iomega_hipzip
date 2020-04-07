/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_build.h - APIs for build/vendor information.
 */


#ifndef	_GN_BUILD_H_
#define _GN_BUILD_H_


/*
 * Dependencies.
 */

#include	<extras/cddb/gn_platform.h>
#include	<extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Prototypes.
 */

/* Get build string */
gn_str_t get_build_summary(void);

/* Get vendor/build strings */

gn_cstr_t
gn_get_build_date(void);

gn_cstr_t
gn_get_build_time(void);

gn_cstr_t
gn_get_vendor(void);

gn_cstr_t
gn_get_product(void);

gn_cstr_t
gn_get_licensee_id(void);

gn_cstr_t
gn_get_client_id(void);

gn_cstr_t
gn_get_eCDDB_version(void);

/* These APIs must be provided by device vendor */
gn_cstr_t
gn_get_device_id(void);

gn_cstr_t
gn_get_software_version(void);

gn_cstr_t
gn_get_hardware_version(void);

#ifdef __cplusplus
}
#endif 


#endif /* _BUILD_H_ */
