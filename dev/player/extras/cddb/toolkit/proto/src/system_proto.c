/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * system_proto.c - protype implementation of the system manager module.
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STDLIB_H
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_system.h>
#include <extras/cddb/gn_configmgr.h>
#if !defined(GN_NO_ONLINE)
#include <extras/cddb/toolkit/proto/online/proto_online.h>
#endif
#include <extras/cddb/gn_lookup.h>
#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
#include <extras/cddb/gn_cache.h>
#endif 
#include <extras/cddb/gn_errors.h>

//#include "gn_proto_id.h"


/*
 * Variables.
 */

static gn_bool_t	sys_inited = GN_FALSE;


/* Initialize TOC lookup subsystem. */
gn_sys_error_t
gnsys_initialize(void)
{
	gn_error_t		error = SYSERR_NoError;

#if !defined(GN_NO_ONLINE)
	gn_bool_t			is_dev_online = GN_TRUE;
	gn_char_t			online_proxy_server[URLSIZE] = "";
	gn_uint16_t			online_proxy_port = 0;
#endif

	/* Are we initialized? */
	if (sys_inited != GN_FALSE)
		return SYSERR_Busy;

	/* Perform any file system initialization */
	error = gnfs_initialize();
	if(error != SUCCESS) {
		return error;
	}

	/* Read configuration information and initialize lower layers - as needed */

#if !defined(GN_NO_ONLINE)
	/* for "on-line" devices, try to initialize COMM layer */
	gnconf_get(IS_DEV_ONLINE, &is_dev_online);
	if (is_dev_online == GN_TRUE) {
		error = gncomm_initialize(GNCOMM_DefaultConnections);
		if ((error != SUCCESS) && (error != COMMERR_AlreadyInitialized)) {
			return error;
		}

		error = SUCCESS;

		/* HTTP proxy info */
		if (error == SUCCESS) {
			error = gnconf_get(ONLINE_PROXY_SERVER, online_proxy_server);
			if (error == ABSTERR_NotFound) {
				error = SUCCESS;	/* it's OK if this setting is not in the config file */
			}
		}
		if (error == SUCCESS) {
			error = gnconf_get(ONLINE_PROXY_PORT, &online_proxy_port);
			if (error == ABSTERR_NotFound) {
				error = SUCCESS;	/* it's OK if this setting is not in the config file */
			}
		}
		if (error == SUCCESS) {
			error = online_configure_proxy_info(online_proxy_server, online_proxy_port);
		}
	}
#endif /* #if !defined(GN_NO_ONLINE) */

	if (error == SYSERR_NoError)
		error = gntlu_initialize();

#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	if (error == SYSERR_NoError)
		error = gncache_initialize(CACHE_OPT_Default);
#endif /* #if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

	/* If no errors, good to go... */
	if (error == SYSERR_NoError)
		sys_inited = GN_TRUE;

	return error;
}

/* Shutdown TOC lookup subsystem. */
gn_sys_error_t
gnsys_shutdown(void)
{
	gn_error_t		error = SYSERR_NoError;

	if (sys_inited != GN_TRUE)
		return SYSERR_NotInited;

#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	gncache_shutdown();
#endif /* #if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

#if !defined(GN_NO_LOCAL)
	gntlu_shutdown();
#endif /* #if !defined(GN_NO_LOCAL) */

#if !defined(GN_NO_ONLINE)
	gncomm_shutdown();
#endif /* #if !defined(GN_NO_ONLINE) */

	gnconf_shutdown();

	gnfs_shutdown();
	sys_inited = GN_FALSE;
	return SYSERR_NoError;
}

/* Get vendor/build strings */
gn_cstr_t
gn_get_vendor(void)
{
	return "eCDDB Development";
}

gn_cstr_t
gn_get_product(void)
{
#define	DEVICE_CLASS	"Linux Prototype Kit"
	return DEVICE_CLASS;
#undef DEVICE_CLASS
}

gn_cstr_t
gn_get_licensee_id(void)
{
	return "unknown";
}

gn_cstr_t
gn_get_client_id(void)
{
	return "unknown";
}

gn_cstr_t
gn_get_eCDDB_version(void)
{
#define	ECDDB_VERSION	"3.0.1.3"
	return ECDDB_VERSION;
#undef ECDDB_VERSION
}

