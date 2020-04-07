/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_upd_constants.h - constants used in API for data updates.
 */

#ifndef	_GN_UPD_CONSTANTS_H_
#define _GN_UPD_CONSTANTS_H_


/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */

#define	FS_FNAME_MAX	15	/* maximum character count for a file name (not including terminating null */
#define	FS_PNAME_MAX	GN_MAX_PATH	/* maximum character count for a full directory and file path name,
							 * (not including terminating null */

/* update state identifier -- used in update status callback and error reporting */
#define UPDSTATE_Default			0
#define UPDSTATE_Update_Start		1
#define UPDSTATE_Download_Start		2
#define UPDSTATE_Downloading		3
#define UPDSTATE_Download_End		4
#define UPDSTATE_Install_Start		5
#define UPDSTATE_Installing			6
#define UPDSTATE_Install_End		7
#define UPDSTATE_Integrate_Start	8
#define UPDSTATE_Integrating		9
#define UPDSTATE_Integrate_End		10
#define UPDSTATE_Backup_Start		11
#define UPDSTATE_BackingUp			12
#define UPDSTATE_Backup_End			13
#define UPDSTATE_Update_End			14


/*
 * Typedefs
 */

typedef gn_error_t gn_upd_error_t;
typedef gn_uint16_t gn_upd_state;


#ifdef __cplusplus
}
#endif 


#endif /* _GN_UPD_CONSTANTS_H_ */


