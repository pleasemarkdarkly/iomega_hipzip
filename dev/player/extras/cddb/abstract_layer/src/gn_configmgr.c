/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_configmgr.c - Configuration Manager implementation for abstraction layer 
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_configmgr.h>
#include	GN_STRING_H
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_utils.h>
#include	GN_CTYPE_H


/*
 * Constants
 */

#define		CFG_KEY_LEN			64

#define		CFG_TYPE_INVAL		0
#define		CFG_TYPE_STR		1
#define		CFG_TYPE_BOOL		2
#define		CFG_TYPE_SHORT		3
#define		CFG_TYPE_LONG		4
#define		CFG_TYPE_HEX		5

#if GN_MAX_PATH > URL_SIZE
#define		CFG_MAX_BUFF_SIZE	GN_MAX_PATH
#else
#define		CFG_MAX_BUFF_SIZE	URL_SIZE
#endif

#define		LNSZ				256


/*
 * Typedefs
 */

typedef struct config_record_t
{
	gn_char_t	key[CFG_KEY_LEN];
	gn_uint16_t	data_type;
	gn_bool_t	value_set_flag;
	gn_uint32_t	ivalue;
	gn_char_t	svalue[CFG_MAX_BUFF_SIZE];
}
gn_config_record_t;


/*
 * Variables
 */

static gn_bool_t		gnconf_is_initialized = GN_FALSE;
static gn_bool_t		gnconf_loaded = GN_FALSE;
static gn_bool_t		gnconf_needs_storing = GN_FALSE;
static gn_bool_t		gnconf_honor_presets = GN_FALSE;

static gn_config_record_t	cfg_internal_config[] = {

	/* key,					data type,			value set,	ivalue,	svalue */

#if !defined(GN_NO_UPDATE_BACKUPS) || !defined (GN_NO_CACHE_BACKUPS)
	{BKUP_DATA_FILE_PATH,	CFG_TYPE_STR,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_CACHE
	{CACHE_SUPPORT,			CFG_TYPE_BOOL,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_ONLINE
	{CONNECT_TIMEOUT,		CFG_TYPE_LONG,		GN_FALSE,	0L,		""},
#endif
#if !defined(_BLD_PROTO_) && !defined(GN_DCDDB)
	{DATA_FILE_PATH,		CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{DB_BUFFER,				CFG_TYPE_LONG,		GN_FALSE,	0L,		""},
#endif
#ifdef GN_DCDDB
	{FIMG_PATH,				CFG_TYPE_STR,		GN_FALSE,	0L,		""},
#endif
	{FLASH_BASE,			CFG_TYPE_HEX,		GN_FALSE,	0L,		""},
	{FLASH_SIZE,			CFG_TYPE_LONG,		GN_FALSE,	0L,		""},
#ifndef GN_NO_UPDATES
	{INSTALL_PATH,			CFG_TYPE_STR,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_ONLINE
	{IS_DEV_ONLINE,			CFG_TYPE_BOOL,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_CACHE_BACKUPS
	{NO_CACHE_BACKUPS,		CFG_TYPE_BOOL,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_UPDATE_BACKUPS
	{NO_UPDATE_BACKUPS,		CFG_TYPE_BOOL,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_ONLINE
	{ONLINE_LOOKUP_CHARSET,	CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{ONLINE_LOOKUP_ENCODING,CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{ONLINE_LOOKUP_URL,		CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{ONLINE_UPDATE_URL,		CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{ONLINE_PROXY_PORT,		CFG_TYPE_SHORT,		GN_FALSE,	0L,		""},
	{ONLINE_PROXY_SERVER,	CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{REG_FILE_PATH,			CFG_TYPE_STR,		GN_FALSE,	0L,		""},
	{TRANSFER_TIMEOUT,		CFG_TYPE_LONG,		GN_FALSE,	0L,		""},
#endif
#ifndef GN_NO_UPDATES
	{XFER_BUFFER,			CFG_TYPE_LONG,		GN_FALSE,	0L,		""},
#endif

	/* terminating element */
	{"",					CFG_TYPE_INVAL,		GN_FALSE,	0L,		""}

};


/*
 * Implementations
 */

gn_error_t
gnconf_initialize(void)
{
	gn_size_t	i = 0;

	if (gnconf_is_initialized == GN_TRUE)
		return ABSTERR_Busy;

	for(i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
		cfg_internal_config[i].value_set_flag = GN_FALSE;

	gnconf_loaded = GN_FALSE;
	gnconf_needs_storing = GN_FALSE;
	gnconf_honor_presets = GN_FALSE;

	gnconf_is_initialized = GN_TRUE;

	return ABSTERR_NoError;
}


void
gnconf_shutdown(void)
{
	gn_size_t	i = 0;

	if (gnconf_is_initialized == GN_FALSE)
		return;

	for(i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
		cfg_internal_config[i].value_set_flag = GN_FALSE;

	gnconf_loaded = GN_FALSE;
	gnconf_needs_storing = GN_FALSE;
	gnconf_honor_presets = GN_FALSE;

	gnconf_is_initialized = GN_FALSE;
}


/* get the parameter */
gn_error_t
gnconf_get(gn_config_param_t param, void *param_val)
{
	gn_size_t	i = 0;

	if ((param == GN_NULL) || (*param == 0))
		return ABSTERR_InvalidArg;

	for (i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
	{
		if (gn_stricmp(param,cfg_internal_config[i].key))
			continue;

		if (cfg_internal_config[i].value_set_flag == GN_FALSE)
			return ABSTERR_NotFound;

		switch (cfg_internal_config[i].data_type)
		{
		case CFG_TYPE_STR:
			strcpy((gn_str_t)param_val, cfg_internal_config[i].svalue);
			return SUCCESS;

		case CFG_TYPE_SHORT:
			sscanf(cfg_internal_config[i].svalue, "%d", (gn_uint16_t*)&(cfg_internal_config[i].ivalue));
			*(gn_uint16_t *)param_val = (gn_uint16_t)(cfg_internal_config[i].ivalue);
			return SUCCESS;

		case CFG_TYPE_LONG:
		case CFG_TYPE_HEX:
			*(gn_uint32_t *)param_val = cfg_internal_config[i].ivalue;
			return SUCCESS;

		case CFG_TYPE_BOOL:
			*(gn_bool_t *)param_val = (gn_bool_t)(cfg_internal_config[i].ivalue);
			return SUCCESS;

		default:
			return ABSTERR_InvalidArg;
		}
	}

	return ABSTERR_InvalidArg;
}


/* Set the parameter */
gn_error_t 
gnconf_set(gn_config_param_t param, void *param_val)
{
	gn_size_t	i = 0;

	if ((param == GN_NULL) || (*param == 0))
		return ABSTERR_InvalidArg;
	if (param_val == GN_NULL)
		return ABSTERR_InvalidArg;

	if (gnconf_is_initialized == GN_FALSE)
		/* that's right, we can just go ahead and assign settings automatically */
		gnconf_initialize();

	for (i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
	{
		if (gn_stricmp(param,cfg_internal_config[i].key))
			continue;

		switch (cfg_internal_config[i].data_type)
		{
		case CFG_TYPE_STR:
			strcpy(cfg_internal_config[i].svalue,(gn_str_t)param_val);
			cfg_internal_config[i].value_set_flag = GN_TRUE;
			break;

		case CFG_TYPE_SHORT:
			cfg_internal_config[i].ivalue = *(gn_uint16_t *)param_val;
			cfg_internal_config[i].value_set_flag = GN_TRUE;

			sprintf(cfg_internal_config[i].svalue, "%d",(gn_uint16_t)(cfg_internal_config[i].ivalue));
			break;

		case CFG_TYPE_LONG:
			cfg_internal_config[i].ivalue = *(gn_uint32_t *)param_val;
			cfg_internal_config[i].value_set_flag = GN_TRUE;

			sprintf(cfg_internal_config[i].svalue, "%ld",(gn_uint32_t)(cfg_internal_config[i].ivalue));
			break;

		case CFG_TYPE_HEX:
			cfg_internal_config[i].ivalue = *(gn_uint32_t *)param_val;
			cfg_internal_config[i].value_set_flag = GN_TRUE;

			sprintf(cfg_internal_config[i].svalue, "%X",(gn_uint32_t)(cfg_internal_config[i].ivalue));
			break;

		case CFG_TYPE_BOOL:
			(gn_bool_t)(cfg_internal_config[i].ivalue) = *(gn_bool_t *)param_val;
			cfg_internal_config[i].value_set_flag = GN_TRUE;

			if ((gn_bool_t)(cfg_internal_config[i].ivalue) == GN_TRUE)
				strcpy(cfg_internal_config[i].svalue,STR_TRUE);
			else
				strcpy(cfg_internal_config[i].svalue,STR_FALSE);
			break;
			break;

		default:
			return ABSTERR_InvalidArg;
		}

		break;
	}

	if (cfg_internal_config[i].data_type == CFG_TYPE_INVAL)
		return ABSTERR_InvalidArg;

	gnconf_needs_storing = GN_TRUE;

	return SUCCESS;
}


gn_error_t
gnconf_set_str(gn_config_param_t param, gn_str_t param_val)
{
	gn_size_t	i = 0;
	gn_error_t	error = GNERR_NoError;
	void*		transform_param = GN_NULL;
	gn_bool_t	bool_param = GN_FALSE;
	gn_uint16_t	short_param = 0;
	gn_uint32_t	long_param = 0L;

	if ((param == GN_NULL) || (*param == 0))
		return ABSTERR_InvalidArg;
	if (param_val == GN_NULL)
		return ABSTERR_CONF_Wrongdata;

	if (gnconf_is_initialized == GN_FALSE)
		/* that's right, we can just go ahead and assign settings automatically */
		gnconf_initialize();

	for (i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
	{
		if (gn_stricmp(param,cfg_internal_config[i].key))
			continue;

		if ((gnconf_honor_presets == GN_TRUE) && (cfg_internal_config[i].value_set_flag == GN_TRUE))
			return SUCCESS;

		strcpy(cfg_internal_config[i].svalue,param_val);
		switch (cfg_internal_config[i].data_type)
		{
		case CFG_TYPE_STR:
			cfg_internal_config[i].value_set_flag = GN_TRUE;
			break;

		case CFG_TYPE_SHORT:
			sscanf(param_val, "%d", (gn_uint16_t*)&(cfg_internal_config[i].ivalue));
			cfg_internal_config[i].value_set_flag = GN_TRUE;
			break;

		case CFG_TYPE_LONG:
			sscanf(param_val, "%ld",&(cfg_internal_config[i].ivalue));
			cfg_internal_config[i].value_set_flag = GN_TRUE;
			break;

		case CFG_TYPE_HEX:
			sscanf(param_val, "%X",&(cfg_internal_config[i].ivalue));
			cfg_internal_config[i].value_set_flag = GN_TRUE;
			break;

		case CFG_TYPE_BOOL:
			if (
				(!gn_stricmp(param_val,STR_TRUE))
				||
				(!gn_stricmp(param_val,"1"))
				)
			{
				cfg_internal_config[i].ivalue = 1;
				cfg_internal_config[i].value_set_flag = GN_TRUE;
			}
			else if (
				(!gn_stricmp(param_val,STR_FALSE))
				||
				(!gn_stricmp(param_val,"0"))
				)
			{
				cfg_internal_config[i].ivalue = 0;
				cfg_internal_config[i].value_set_flag = GN_TRUE;
			}
			else
			{
				cfg_internal_config[i].value_set_flag = GN_FALSE;
			}
			break;

		default:
			break;
		}

		break;
	}

	if (cfg_internal_config[i].data_type == CFG_TYPE_INVAL)
		return ABSTERR_InvalidArg;

	gnconf_needs_storing = GN_TRUE;

	return SUCCESS;
}


gn_error_t
gnconf_load_params(gn_cstr_t filename)
{
	gn_uchar_t	ln[LNSZ] = "";
	gn_uchar_t	*key = GN_NULL;
	gn_uchar_t	*key_tail = GN_NULL;
	gn_uchar_t	*value = GN_NULL;
	gn_str_t	internal_filename = (gn_str_t)filename;
	gn_handle_t	filehandle = FS_INVALID_HANDLE;
	gn_bool_t	local_needs_storing_value = gnconf_needs_storing;

	if (internal_filename == GN_NULL)
		internal_filename = "config.gn";

	if (gnfs_exists(internal_filename) == GN_FALSE)
		return gnfs_get_error();

	filehandle =  gnfs_open(internal_filename, FSMODE_ReadOnly);
	if (filehandle == FS_INVALID_HANDLE)
		return gnfs_get_error();

	while (1)
	{
		key = gnfs_readln(filehandle,ln,sizeof(ln));
		if (key == GN_NULL)	/* no more data in this file */
			break;

		while (isspace(key[0]))			/* drop leading whitespace */
			key++;
		if (key[0] == 0)					/* skip if blank line */
			continue;

		value = strchr(key, '=');					/* find equal sign */
		if (value == NULL)						/* error if none */
			continue;

		key_tail = value;
		while ((key_tail > key) && (isspace(key_tail[-1])))	/* remove whitespace */
			key_tail--;

		*key_tail = 0;								/* plug EOS over = sign */
		value++;	/* step over the '=' character */
		while (isspace(value[0]))			/* remove leading space on */
			value++;

		gnconf_honor_presets = GN_TRUE;
		gnconf_set_str(key,value);
		gnconf_honor_presets = GN_FALSE;
	}

	gnfs_close(filehandle);

	gnconf_loaded = GN_TRUE;
	gnconf_needs_storing = local_needs_storing_value;

	return SUCCESS;
}


gn_error_t
gnconf_get_str_element(gn_size_t index, gn_config_param_t* param, gn_str_t* param_val)
{
	if (param == NULL)
		return ABSTERR_InvalidArg;
	if (param_val == NULL)
		return ABSTERR_InvalidArg;

	if (index >= (sizeof(cfg_internal_config)/sizeof(gn_config_record_t)))
		return ABSTERR_InvalidArg;

	if (cfg_internal_config[index].data_type == CFG_TYPE_INVAL)
		return ABSTERR_NotFound;

	*param = cfg_internal_config[index].key;
	*param_val = cfg_internal_config[index].svalue;

	return SUCCESS;
}


void
gnconf_store_params(gn_cstr_t filename)
{
#if !defined(GN_READONLY)
	gn_size_t	i = 0;
	gn_error_t	error = SUCCESS;
	gn_str_t	internal_filename = (gn_str_t)filename;
	gn_handle_t	filehandle = FS_INVALID_HANDLE;
	gn_size_t	write_size = 0;

	if (gnconf_loaded == GN_FALSE)
		/* this is not a satisfactory solution */
		return;

	if (gnconf_needs_storing == GN_FALSE)
		/* nothing got changed */
		return;

	for (i = 0; cfg_internal_config[i].data_type != CFG_TYPE_INVAL; i++)
	{
		if (cfg_internal_config[i].value_set_flag == GN_FALSE)
			continue;

		switch (cfg_internal_config[i].data_type)
		{
		case CFG_TYPE_STR:
			break;

		case CFG_TYPE_BOOL:
			if ((gn_bool_t)(cfg_internal_config[i].ivalue) == GN_TRUE)
				strcpy(cfg_internal_config[i].svalue,STR_TRUE);
			else
				strcpy(cfg_internal_config[i].svalue,STR_FALSE);
			break;

		case CFG_TYPE_SHORT:
			sprintf(cfg_internal_config[i].svalue, "%d",(gn_uint16_t)(cfg_internal_config[i].ivalue));
			break;

		case CFG_TYPE_LONG:
			sprintf(cfg_internal_config[i].svalue, "%ld",(gn_uint32_t)(cfg_internal_config[i].ivalue));
			break;

		case CFG_TYPE_HEX:
			sprintf(cfg_internal_config[i].svalue, "%X",(gn_uint32_t)(cfg_internal_config[i].ivalue));
			break;

		default:
			continue;
		}

		if (filehandle == FS_INVALID_HANDLE)
		{
			if (internal_filename == GN_NULL)
				internal_filename = "config.gn";
			filehandle = gnfs_create(internal_filename, FSMODE_ReadWrite, FSATTR_ReadWrite);
			if (filehandle == FS_INVALID_HANDLE)
			{
				error = gnfs_get_error();
				goto cleanup;
			}
		}

		write_size = gnfs_write(filehandle,cfg_internal_config[i].key,strlen(cfg_internal_config[i].key));
		if (write_size != strlen(cfg_internal_config[i].key))
		{
			error = gnfs_get_error();
			goto cleanup;
		}

		write_size = gnfs_write(filehandle,"=",1);
		if (write_size != 1)
		{
			error = gnfs_get_error();
			goto cleanup;
		}

		write_size = gnfs_write(filehandle,cfg_internal_config[i].svalue,strlen(cfg_internal_config[i].svalue));
		if (write_size != strlen(cfg_internal_config[i].svalue))
		{
			error = gnfs_get_error();
			goto cleanup;
		}

		write_size = gnfs_write(filehandle,"\n",1);
		if (write_size != 1)
		{
			error = gnfs_get_error();
			goto cleanup;
		}
	}

	gnconf_needs_storing = GN_FALSE;

cleanup:
	if (filehandle != FS_INVALID_HANDLE)
		gnfs_close(filehandle);
#endif
}
