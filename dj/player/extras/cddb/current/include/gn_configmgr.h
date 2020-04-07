/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_configmgr.h - Configuration Manager.
 */


#ifndef	_GN_CONFIGMGR_H_
#define _GN_CONFIGMGR_H_


/*
 * Dependencies.
 */

#include	<extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C"{
#endif 

/*
 * Constants
 */

#define URLSIZE			255
#define DESCSIZE		32

	/* key names */
#define IS_DEV_ONLINE			"IS_DEV_ONLINE"
#define LANGUAGE				"LANGUAGE"
#define CACHE_SUPPORT			"CACHE_SUPPORT"
#define DATA_FILE_PATH			"DATA_FILE_PATH"
#define BKUP_DATA_FILE_PATH		"BKUP_DATA_FILE_PATH"
#define ONLINE_LOOKUP_URL		"ONLINE_LOOKUP_URL"
#define ONLINE_UPDATE_URL		"ONLINE_UPDATE_URL"
#define DB_BUFFER				"DB_BUFFER"
#define XFER_BUFFER				"XFER_BUFFER"
#define ONLINE_PROXY_SERVER		"ONLINE_PROXY_SERVER"
#define ONLINE_PROXY_PORT		"ONLINE_PROXY_PORT"
#define ONLINE_LOOKUP_CHARSET	"ONLINE_LOOKUP_CHARSET"
#define ONLINE_LOOKUP_ENCODING	"ONLINE_LOOKUP_ENCODING"
#define REG_FILE_PATH			"REG_FILE_PATH"
#define INSTALL_PATH			"INSTALL_PATH"
#define FIMG_PATH				"FIMG_PATH"
#define FLASH_BASE				"FLASH_BASE"
#define FLASH_SIZE				"FLASH_SIZE"
#ifdef _BLD_TH_
#define NO_PAUSE_ON_UPDATES		"NO_PAUSE_ON_UPDATES"
#endif
#define CONNECT_TIMEOUT			"CONNECT_TIMEOUT"
#define TRANSFER_TIMEOUT		"TRANSFER_TIMEOUT"
#define NO_UPDATE_BACKUPS		"NO_UPDATE_BACKUPS"
#define NO_CACHE_BACKUPS		"NO_CACHE_BACKUPS"


#define STR_TRUE					"TRUE"
#define STR_FALSE					"FALSE"


/*
 * Typedefs
 */

typedef gn_str_t		gn_config_param_t;


/* retain the following three for backward compatibility */
typedef gn_bool_t		gn_cache_support_t;
#define CACHE_UNAVAILABLE			GN_FALSE
#define CACHE_AVAILABLE				GN_TRUE


/*
 * Prototypes.
 */

gn_error_t
gnconf_initialize(void);

void
gnconf_shutdown(void);

gn_error_t
gnconf_load_params(gn_cstr_t config_file);

void
gnconf_store_params(gn_cstr_t filename);

/* Get the parameter value*/
gn_error_t
gnconf_get(gn_config_param_t param, void *param_val);

/* Set the parameter value using final target data type */
gn_error_t 
gnconf_set(gn_config_param_t param, void *param_val);

gn_error_t
gnconf_get_str_element(gn_size_t index, gn_config_param_t* param, gn_str_t* param_val);

/* Set the parameter value using the string representation of the data */
gn_error_t
gnconf_set_str(gn_config_param_t param, gn_str_t param_val);


#ifdef __cplusplus
}
#endif 


#endif /* _GN_CONFIGMGR_H_ */
