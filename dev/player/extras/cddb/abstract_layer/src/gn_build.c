/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_build.c - APIs for build/vendor information.
 */


/*
 * Dependencies
 */

#include <extras/cddb/gn_build.h>
#include GN_STDIO_H


/*
 * Constants
 */

const gn_char_t	internal_vers_fmt[] =	"eCDDB Version  : %s";

const gn_char_t	external_vers_fmt[] =	"Software Vers. : %s\n"
										"Hardware Vers. : %s\n"
										"Build Date     : %s %s";

const gn_char_t	internal_ident_fmt[] =	"Vendor         : %s\n"
										"Product        : %s\n"
										"Licensee ID    : %s\n"
										"Client ID      : %s";

const gn_char_t	external_ident_fmt[] =	"Device ID      : %s";

const gn_char_t	functionality_fmt[] =	"Functionality  : %s";

#define	BUILD_SUMMARY_SIZE	512


/*
 * Variables
 */

static gn_char_t	build_summary[BUILD_SUMMARY_SIZE] = "";


/*
 * Implementations
 */

gn_str_t get_build_summary(void)
{
	gn_str_t	ecddb_vers_ptr = GN_NULL;
	gn_str_t	sw_ptr = GN_NULL;
	gn_str_t	hw_ptr = GN_NULL;
	gn_str_t	date_ptr = GN_NULL;
	gn_str_t	time_ptr = GN_NULL;
	gn_str_t	vend_ptr = GN_NULL;
	gn_str_t	prod_ptr = GN_NULL;
	gn_str_t	lic_ptr = GN_NULL;
	gn_str_t	client_ptr = GN_NULL;
	gn_str_t	dev_id_ptr = GN_NULL;

	gn_char_t	empty_str[] = "(null)";

	gn_char_t	fmt_buffer[BUILD_SUMMARY_SIZE] = "";

	ecddb_vers_ptr = (gn_str_t)gn_get_eCDDB_version();
	if (ecddb_vers_ptr == GN_NULL)
		ecddb_vers_ptr = empty_str;
	sw_ptr = (gn_str_t)gn_get_software_version();
	if (sw_ptr == GN_NULL)
		sw_ptr = empty_str;
	hw_ptr = (gn_str_t)gn_get_hardware_version();
	if (hw_ptr == GN_NULL)
		hw_ptr = empty_str;
	date_ptr = (gn_str_t)gn_get_build_date();
	if (date_ptr == GN_NULL)
		date_ptr = empty_str;
	time_ptr = (gn_str_t)gn_get_build_time();
	if (time_ptr == GN_NULL)
		time_ptr = empty_str;
	vend_ptr = (gn_str_t)gn_get_vendor();
	if (vend_ptr == GN_NULL)
		vend_ptr = empty_str;
	prod_ptr = (gn_str_t)gn_get_product();
	if (prod_ptr == GN_NULL)
		prod_ptr = empty_str;
	lic_ptr = (gn_str_t)gn_get_licensee_id();
	if (lic_ptr == GN_NULL)
		lic_ptr = empty_str;
	client_ptr = (gn_str_t)gn_get_client_id();
	if (client_ptr == GN_NULL)
		client_ptr = empty_str;
	dev_id_ptr = (gn_str_t)gn_get_device_id();
	if (dev_id_ptr == GN_NULL)
		dev_id_ptr = empty_str;

	sprintf(fmt_buffer,
		"%s\n%s\n\n%s\n%s\n\n%s",
		internal_vers_fmt,
		external_vers_fmt,
		internal_ident_fmt,
		external_ident_fmt,
		functionality_fmt
		);

	sprintf(build_summary,fmt_buffer,
		ecddb_vers_ptr,
		sw_ptr,sw_ptr,date_ptr,time_ptr,
		vend_ptr,prod_ptr,lic_ptr,client_ptr,
		dev_id_ptr,
#ifdef _BLD_PROTO_
		"\n\tPrototype"
#endif
#ifdef _BLD_TH_
		"\n\tTest"
#endif
#ifdef GN_CACHE_DATA
		"\n\tAlways Cache Data"
#endif
#ifdef GN_DCDDB
		"\n\tData Disk CDDB"
#endif
#ifdef GN_FLASH_PAGE_IO
		"\n\tFlash Memory Requires Paged I/O"
#endif
#ifdef GN_MANUAL_DATA_PACK
		"\n\tUnique Word Alignments"
#endif
#ifdef GN_MEM_CACHE
		"\n\tMemory-Based Cache"
#endif
#ifndef GN_NO_CACHE
		"\n\tLocal Cache"
#endif
#ifndef GN_NO_CACHE_BACKUPS
		"\n\tCache File Backups"
#endif
#ifndef GN_NO_FILE_ATTR
		"\n\tFile System Modes Recognized"
#endif
#ifndef GN_NO_LOCAL
		"\n\tLocal Database"
#endif
#ifndef GN_NO_LOGGING
		"\n\tLogging"
#endif
#ifndef GN_NO_ONLINE
		"\n\tOnline Functionality"
#endif
#ifndef GN_NO_ONLINE_UPDATES
		"\n\tOnline Updates"
#endif
#ifndef GN_NO_PCUPDATES
		"\n\tPC Connectivity/Updates"
#endif
#ifdef GN_NO_PROTO_COMPRESSION
		"\n\tUpstream Communications Not Compressed"
#endif
#ifndef GN_NO_TOC_READING
		"\n\tTOC Lookups on CD"
#endif
#ifndef GN_NO_UPDATE_BACKUPS
		"\n\tUpdate DB Backups"
#endif
#ifndef GN_NO_UPDATES
		"\n\tLocal Updates"
#endif
#ifdef GN_READONLY
		"\n\tRead-Only File System"
#endif
		""
		);

	return build_summary;
}


gn_cstr_t
gn_get_build_date(void)
{
	return __DATE__;
}


gn_cstr_t
gn_get_build_time(void)
{
	return __TIME__;
}
