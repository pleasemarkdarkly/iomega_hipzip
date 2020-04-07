/*
 * Copyright (c) 2002 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * logging.c - Functionality to display logging status from the shell.
 */


/*
 * Dependencies
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_log.h>
#include <extras/cddb/gn_errors.h>
#include GN_STRING_H
#include "logging.h"
#include <util/debug/debug.h>

#if !defined(GN_NO_LOGGING)

/*
 * Variables
 */

/* This table is used for displaying, enabling, and disabling
 * If more packages are added, add them to this table.
 */
static const gn_pkg_map_t pkg_map[] = {
	{ "Generic",		GNPKG_Generic },
	{ "Abstraction",	GNPKG_Abstraction },
	{ "DBEngine",		GNPKG_DBEngine },
	{ "XML",		GNPKG_XML },
	{ "Translator",		GNPKG_Translator },
	{ "EmbeddedDB",		GNPKG_EmbeddedDB },
	{ "TOCLookup",		GNPKG_TOCLookup },
	{ "Crypto",		GNPKG_Crypto },
	{ "Communications",	GNPKG_Communications },
	{ "OnlineProtocol",	GNPKG_OnlineProtocol },
	{ "DynBuf",		GNPKG_DynBuf },
	{ "Updater",		GNPKG_Updater },
	{ "Utils",		GNPKG_Utils },
	{ "System",		GNPKG_System },
	{ "EmbeddedDBIM",	GNPKG_EmbeddedDBIM },
	{ "LocalDataCache",	GNPKG_LocalDataCache },
	{ "MemoryMgr",		GNPKG_MemoryMgr },
	{ "FileSystem",		GNPKG_FileSystem },
	{ "ALL",		GNLOG_PKG_ALL},
	{ NULL,			0}
};

/* If more mask bits are added, add them to this table
 */
static const gn_mask_map_t mask_map[] = {
	{ "GNLOG_FORCE",	GNLOG_FORCE},
	{ "GNLOG_VERBOSE",	GNLOG_VERBOSE},
	{ "GNLOG_NO_TIME_STAMP",	GNLOG_NO_TIME_STAMP},
	{ "GNLOG_IDS_ALL",	GNLOG_IDS_ALL},
	{ "GNLOG_ERROR",	GNLOG_ERROR},
	{ "GNLOG_FTRACE",	GNLOG_FTRACE},
	{ "GNLOG_CTRACE",	GNLOG_CTRACE},
	{ "GNLOG_CTRACE2",	GNLOG_CTRACE2},
	{ "GNLOG_DTRACE",	GNLOG_DTRACE},
	{ "GNLOG_DTRACE2",	GNLOG_DTRACE2},
	{ "GNLOG_DTRACE3",	GNLOG_DTRACE3},
	{ "GNLOG_DTRACE4",	GNLOG_DTRACE4},
	{ NULL,			0}
};


/*
 * Implementations
 */

/* print a 32 bit number in binary */
static void print_bin32(gn_uint32_t val)
{
	int	i;

	for(i = 0; i < 32; i++)
	{
		if(val & 0x80000000)
		{
			diag_printf("1");
		}
		else
		{
			diag_printf("0");
		}
		val <<= 1;
	}
}

static gn_bool_t print_pkg_status(gn_cstr_t pkg_str, gn_uint16_t pkg_num)
{
	gn_bool_t	retval;
	gn_uint32_t	mask;

	retval = gnlog_get_pkg_mask(pkg_num, &mask);
	if(retval == GN_TRUE)
	{
		diag_printf("%-20s  ", pkg_str);
#if 0
		print_bin32(mask);
#else
		diag_printf("0x%.8x", mask);
#endif
	}

	return retval;
}

static int find_pkg_index(gn_cstr_t str)
{
	int	i;

	for(i = 0; pkg_map[i].str != NULL; i++)
	{
		if(stricmp(pkg_map[i].str, str) == 0)
		{
			return i;
		}
	}

	/* not found */
	return -1;
}

void do_display_logging(int argc, char* argv[])
{
	int		i;

	diag_printf("Logging Mask Key:\n");
	for(i = 0; mask_map[i].str != NULL; i++)
	{
		diag_printf("%-20s   0x%.8x", mask_map[i].str, mask_map[i].mask);
		if(i % 2)
		{
			diag_printf("\n");
		}
		else
		{
			diag_printf("        ");
		}
	}
	diag_printf("\n\n");

	diag_printf("Current Package Logging Status:\n");
	diag_printf("Package               Mask Bits         Package                Mask Bits\n");
	diag_printf("--------------------------------        --------------------------------\n");

	for(i = 0; pkg_map[i].str != NULL; i++)
	{
		print_pkg_status(pkg_map[i].str, pkg_map[i].pkg);
		if(i % 2)
		{
			diag_printf("\n");
		}
		else
		{
			diag_printf("        ");
		}
	}
	diag_printf("\n");
}

void do_enable_logging(int argc, char *argv[])
{
	int		i;
	int		k;
	int		map_index;
	gn_uint32_t	mask = 0;

	if(argc < 2)
	{
		diag_printf("Invalid Number Of Arguements\n");
		return;
	}

	map_index = find_pkg_index(argv[0]);
	if(map_index == -1)
	{
		diag_printf("Package \"%s\" not found\n", argv[0]);
		return;
	}

	for(k = 1; k < argc; k++)
	{
		for(i = 0; mask_map[i].str != NULL; i++)
		{
			if(stricmp(argv[k], mask_map[i].str) == 0)
			{
				mask |= mask_map[i].mask;
			}
		}
	}

	diag_printf("enable mask: ");
	print_bin32(mask);
	diag_printf("\n\n");

	gnlog_enable(pkg_map[map_index].pkg, mask);
}
     
void do_disable_logging(int argc, char *argv[])
{
	int		i;
	int		k;
	int		map_index;
	gn_uint32_t	mask = 0;

	if(argc < 2)
	{
		diag_printf("Invalid Number Of Arguements\n");
		return;
	}

	map_index = find_pkg_index(argv[0]);
	if(map_index == -1)
	{
		diag_printf("Package \"%s\" not found\n", argv[0]);
		return;
	}

	for(k = 1; k < argc; k++)
	{
		for(i = 0; mask_map[i].str != NULL; i++)
		{
			if(strcmp(argv[k], mask_map[i].str) == 0)
			{
				mask |= mask_map[i].mask;
			}
		}
	}

	diag_printf("disable mask: ");
	print_bin32(mask);
	diag_printf("\n\n");

	gnlog_disable(pkg_map[map_index].pkg, mask);
}

#endif  /* GN_NO_LOGGING */
