/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * command_table.c - Implements the command functions and table for the shell app.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/crossplatform.h>

#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H
#ifndef _BLD_TH_
#include GN_STRING_H
#endif
#include GN_CTYPE_H

#include "lookup.h"
#include "command_table.h"
#include "cmdhandlers.h"
#include "shell.h"
#include <extras/cddb/read_cd_drive.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_lookup.h>
#include <extras/cddb/gn_system.h>
#include <extras/cddb/gn_configmgr.h>
#if !defined(GN_NO_UPDATES)
#include <extras/cddb/gn_upd.h>
#include "updates.h"
#endif
#if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
#include <extras/cddb/gn_cache.h>
#endif /* #if	!defined(GN_NO_CACHE) || defined(GN_MEM_CACHE) */

#include "configuration.h"
#include "branding.h"
#if !defined(GN_NO_LOGGING)
#include "logging.h"
#endif  /* GN_NO_LOGGING */
#ifdef PLATFORM_WINDOWS
	#include <conio.h>
	#include <windows.h>
#endif

#ifdef PLATFORM_UNIX
	#include <unistd.h>
#endif

#ifdef _BLD_TH_
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include <extras/cddb/gn_log.h>
#include <util/debug/debug.h>


/*
 * Variables
 */

#ifdef _BLD_TH_
const gn_char_t	eatdisk_filename[] = "eatdisk";
#endif

static	gn_uchar_t*	heap = NULL;
static	gn_bool_t	gEcho_Reactivate = GN_FALSE;
static	gn_size_t	heap_size = 0;


/*
 * Local functions.
 */

/* command handlers */
static void 	lookup_disc_from_cmdline(int argc, char *argv[]);
static void 	lookup_disc_from_cmdline_online(int argc, char *argv[]);
static void 	lookup_disc_in_drive(int argc, char* argv[]);
static void 	lookup_disc_from_file(int argc, char* argv[]);
static void 	lookup_disc_in_drive_online(int argc, char* argv[]);
static void 	lookup_disc_from_file_online(int argc, char* argv[]);
static void 	lookup_disc_in_drive_cache(int argc, char* argv[]);
static void 	lookup_disc_from_file_cache(int argc, char* argv[]);
static void 	lookup_disc_in_drive_all(int argc, char* argv[]);
static void 	lookup_disc_from_file_all(int argc, char* argv[]);
static void 	batch_tocs_in_cache(int argc, char* argv[]);
static void 	add_toc_from_file_to_cache(int argc, char* argv[]);
static void 	delete_toc_from_file_from_cache(int argc, char* argv[]);
static void 	lookup_tocs_in_cache(int argc, char* argv[]);
static void 	list_tocs_in_cache(int argc, char* argv[]);
static void 	delete_cache(int argc, char* argv[]);
static void 	update_database_from_file(int argc, char* argv[]);
static void 	online_update(int argc, char* argv[]);
static void 	pc_update(int argc, char* argv[]);
static void 	init_ecddb(int argc, char* argv[]);
static void 	kill_ecddb(int argc, char* argv[]);
static void		disp_conf(int argc, char* argv[]);
static void		set_conf(int argc, char* argv[]);
static void		save_conf(int argc, char* argv[]);
static void		set_comm_callback(int argc, char* argv[]);
static void		set_upd_callback(int argc, char* argv[]);
static void		set_opt(int argc, char* argv[]);
static void		disp_opt(int argc, char* argv[]);
static void		recover(int argc, char* argv[]);
static void		show_profile(int argc, char* argv[]);
void wait(int argc, char* argv[]);
void print(int argc, char* argv[]);
#if !defined(GN_NO_UPDATES)
static void		get_free_disc_space(int argc, char* argv[]);
#ifdef _BLD_TH_
static void		set_free_disc_space(int argc, char* argv[]);
static void		reset_free_disc_space(int argc, char* argv[]);
#endif
static void		reset_db(int argc, char* argv[]);
static void		revert_db(int argc, char* argv[]);
#endif
static void		set_brand_duration(int argc, char* argv[]);
static void		set_command_repeat(int argc, char* argv[]);
static void		redirect_input(int argc, char* argv[]);
static void		print_version(int argc, char* argv[]);
static void		set_lkup_type(int argc, char* argv[]);
static void		disp_lkup_type(int argc, char* argv[]);
#if !defined(GN_NO_LOGGING)
static void		display_logging(int argc, char* argv[]);
static void		enable_logging(int argc, char *argv[]);
static void		disable_logging(int argc, char *argv[]);
#endif  /* GN_NO_LOGGING */


/* prompt user for filename */
static int	ask_user_for_file(char* filename_buffer, size_t filename_buffer_size);

/*
 * Variables.
 */

/* command table: holds all the commands with information about their usage */
command_record_t table_main_menu[] = {
	{ quit, 						"quit", 	"quit",					"Quit", NULL },
	{ show_commands,				"help",		"help [command]",		"Help on commands", NULL },
	{ print_version,				"version",		"version",		"Displays version and build information", NULL },
	{ init_ecddb,					"initialize",	"initialize",	"Initialize eCDDB for use.", NULL },
	{ kill_ecddb,					"shutdown",	"shutdown",	"Shutdown eCDDB sub-system.", NULL },
#ifndef GN_PC_UPDATE
#if !defined(GN_NO_LOCAL)
#if !defined(_BLD_PROTO_)
	{ lookup_disc_in_drive, 		"lookup",	"lookup [[-n NNN]|[drivespec]]",	"Lookup disc in the CD-ROM drive from local db", NULL },
#else
	{ lookup_disc_in_drive, 		"lookup",	"lookup [drivespec]",	"Lookup disc in the CD-ROM drive from local db", NULL },
#endif
	{ lookup_disc_from_file, 		"flookup",	"flookup [-i][filespec]",	"Lookup disc from TOC(s) in file from local db", NULL },
#if !defined(_BLD_PROTO_)
	{ lookup_disc_from_cmdline, 		"plookup",	"plookup [toc]",	"Lookup disc from TOC typed in from local db", NULL },
#endif
#endif
#if !defined(GN_NO_ONLINE)
	{ lookup_disc_in_drive_online, 	"olookup",	"olookup [drivespec]",	"Lookup disc in CD-ROM drive from online db", NULL },
	{ lookup_disc_from_file_online, 	"oflookup",	"oflookup [-i][filespec]",	"Lookup disc from TOC(s) in file from online db", NULL },
#if !defined(_BLD_PROTO_)
	{ lookup_disc_from_cmdline_online, 	"oplookup",	"oplookup [toc]",	"Lookup disc from TOC typed in from online db", NULL },
#endif
#endif
#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	{ lookup_disc_in_drive_cache, 	"clookup",	"clookup [[-n NNN]|[drivespec]]",	"Lookup disc in CD-ROM drive from cache", NULL },
	{ lookup_disc_from_file_cache, 	"cflookup",	"cflookup [-i][filespec]",	"Lookup disc from TOC(s) in file from cache", NULL },
#endif
#if !defined(_BLD_PROTO_)
#if ((!defined(GN_NO_ONLINE) || !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)) && (!defined(GN_NO_LOCAL) || !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)))
	{ lookup_disc_in_drive_all, 	"xlookup",	"xlookup [drivespec]",	"Lookup disc in CD-ROM drive from everywhere", NULL },
	{ lookup_disc_from_file_all, 	"xflookup",	"xflookup [filespec]",	"Lookup disc from TOC(s) in file from everywhere", NULL },
#endif
#endif
#if !defined(GN_NO_CACHE) || defined(GN_MEM_CACHE)
	{ batch_tocs_in_cache,			"batch",	"batch [drivespec]",	"Add TOC(s) to cache and look them up", NULL },
	{ add_toc_from_file_to_cache,   "fcache",	"fcache [filespec]",	"Add TOC(s) in file to cache", NULL },
	{ delete_toc_from_file_from_cache,   "dcache",	"dcache [filespec]",	"Delete TOC(s) in file from cache", NULL },
#if defined(GN_DCDDB)
	{ lookup_tocs_in_cache,         "ucache",	"ucache",	"Lookup TOCs in cache and add data", NULL },
#else
	{ lookup_tocs_in_cache,         "ucache",	"ucache [online|local|both]",	"Update/lookup/remove TOCs in cache", NULL },
#endif /* ifndef GN_PC_UPDATE */
	{ list_tocs_in_cache,			"listcache",	"listcache [-v][toc_id]",	"List TOCs in cache or those matching 'toc_id'", NULL },
	{ delete_cache,					"delcache",	"delcache",	"Delete cache (both live and backup files)", NULL },
#endif
#endif /* ifndef GN_PC_UPDATE */
#if !defined(GN_NO_UPDATES)
	{ show_profile,					"profile",	"profile",				"Display revision/update profile information", NULL },
	{ update_database_from_file,	"update",	"update [filespec]",	"Update local db from a file...", NULL },
	{ recover,						"recover",	"recover [filespec]",	"Recover from major error (e.g., device failure)", NULL },
#ifndef GN_PC_UPDATE
	{ online_update,				"oupdate",	"oupdate",	"Update local db from an online file", NULL },
	{ pc_update,				"pcupdate",	"pcupdate",	"Update local db from a PC Connectivity file", NULL },
#endif /* ifndef GN_PC_UPDATE */
#endif /* GN_NO_UPDATES */
	{ 
		set_lkup_type,		
		"set_lkup_type",	 
		"set_lkup_type [value]",
		"Set the lookup type value.",
		"set_lkup_type [value]\n"
		"The value can be any combination of the following, separated by ','\n"
		"  OFFLINE\n"
#ifndef GN_NO_ONLINE
		"  ONLINE\n"
#endif
		"  CACHE\n"
		"  AUTOFUZZY\n"
		"  AUTOCACHE\n"
		"  EXACTONLY\n"
	},
	{ disp_lkup_type,	"disp_lkup_type", "disp_lkup_type",			"Display the lookup type value."},
	{
		set_opt,
		"set_opt",
		"set_opt <option> [ON | OFF]",
		"Enable/Disable shell option setting",
		"Enable/Disable options:\n"
		" timing:     display timing information for each command\n"
		" encoding:   base64 decode lookup results\n"
		" echo:       echo commands for scripting or redirection\n"
#ifndef GN_NO_ONLINE
		" callback:   install/remove callback for online lookups\n"
#endif
		" update_callback: install/remove update callback\n"
		" strict:     perform strict validation comparison\n"
		" autoselect: select first choice from fuzzy list after display\n"
		" verbose:    print lots of stuff\n"
		" paging:     wait for keyboard input between TOC/ID lookups\n"
		" rawXML:     dump raw XML data from lookups\n"
		" rawUTF8:    display unconverted UTF8 data\n"
		" swap:       swap disc for batch lookups\n"
		" repeat      repeat the next command \'n\' times (include \'n\' after \'on\')\n"
		" init_ecddb  initialize ecddb upon startup\n"
	},
	{ disp_opt, 					"disp_opt", 	"disp_opt [option]",		"Display shell option setting", NULL },
	{ 
		set_conf,		
		"set_conf",	 
		"set_conf <option> [value]",
		"Set configuration value for specified option.",
		"set_conf <key> [value]\n"
		"  Key                        Values\n"
		"  ----------------------------------------------------------\n"
#if !defined(GN_NO_UPDATE_BACKUPS) || !defined (GN_NO_CACHE_BACKUPS)
		"  BKUP_DATA_FILE_PATH        (full or partial directory path)\n"
#endif
#ifndef GN_NO_CACHE
		"  CACHE_SUPPORT              TRUE\\FALSE\n"
#endif
#ifndef GN_NO_ONLINE
		"  CONNECT_TIMEOUT            (timeout in seconds before online connects fail)\n"
#endif
#if !defined(_BLD_PROTO_) && !defined(GN_DCDDB)
		"  DATA_FILE_PATH             (full or partial directory path)\n"
		"  DB_BUFFER                  (size in bytes)\n"
#endif
#ifdef GN_DCDDB
		"  FIMG_PATH                  (full or partial directory path and file name)\n"
		"  FLASH_BASE                 (base address in hex of FlashRAM buffer)\n"
		"  FLASH_SIZE                 (size in bytes of FlashRAM buffer)\n"
#endif
#ifndef GN_NO_UPDATES
		"  INSTALL_PATH               (full or parital directory path)\n"
#endif
#ifndef GN_NO_ONLINE
		"  IS_DEV_ONLINE              TRUE\\FALSE\n"
#endif
#ifndef GN_NO_CACHE_BACKUPS
		"  NO_CACHE_BACKUPS           TRUE\\FALSE\n"
#endif
#ifndef GN_NO_UPDATE_BACKUPS
		"  NO_UPDATE_BACKUPS          TRUE\\FALSE\n"
#endif
#ifndef GN_NO_ONLINE
		"  ONLINE_LOOKUP_CHARSET      UTF-8\\shift-jis\n"
		"  ONLINE_LOOKUP_ENCODING     (no value)\\base64\n"
		"  ONLINE_LOOKUP_URL          (valid server_name[:port]/path)\n"
		"  ONLINE_UPDATE_URL          (valid server_name[:port]/path)\n"
		"  ONLINE_PROXY_PORT          (valid port number)\n"
		"  ONLINE_PROXY_SERVER        (valid server_name)\n"
		"  REG_FILE_PATH              (file name plus full or partial directory path)\n"
		"  TRANSFER_TIMEOUT           (timeout in seconds before online transfers fail)\n"
#endif
#ifndef GN_NO_UPDATES
		"  XFER_BUFFER                (size in bytes)\n"
#endif
	},
	{ disp_conf,	"disp_conf", "disp_conf",			"Display the configuration information"},
	{ save_conf,	"save_conf", "save_conf",			"Save the current configuration to file"},
#if !defined(_BLD_PROTO_)
	{ redirect_input, "script",  "script filename",		"Execute the script of commands", NULL},
#endif /* #if !defined(_BLD_PROTO_) */
	{ redirect_input, "redir_input", "redir_input filename", "Redirect source of command input.", "Input returned to stdin after commands in file are exhausted."},
#if !defined(GN_NO_UPDATES)
	{ get_free_disc_space,	"get_fds.updates",	"get_fds", "Get the current free disc space.", NULL},
#ifdef _BLD_TH_
	{ set_free_disc_space,	"set_fds",	"set_fds <free space> [ KB | MB | GB]", "Set maximum free disc space.", "By default, free space is in bytes.\nUse a KB or MB or GB modifier to represent other units.\nRemember to put a space between the number and the modifier."},
	{ reset_free_disc_space,	"reset_fds",	"reset_fds", "Returns free disc space to initial conditions.", NULL},
#endif /* #ifdef _BLD_TH_ */
	{ reset_db,	"reset_db",	"reset_db", "Resets database to original factory conditions.", NULL},
	{ revert_db,	"revert_db",	"revert_db", "Reverts update database to backed-up version.", NULL},
#endif /* #if !defined(GN_NO_UPDATES) */
#if !defined(_BLD_PROTO_)
	{ print, "print", "print [free-form text]", "Print the free-form text to output.", NULL},
	{ wait, "wait", "wait [seconds | \"keystroke\"]", "Delay for specified number of seconds, or for user input.", NULL},
#endif
#if !defined(GN_NO_LOGGING)
	{ display_logging,	"disp_logging",	"disp_logging", "Display the current logging settings", NULL},
	{
		enable_logging,
		"enable_logging",
		"enable_logging package mask",
		"Enable logging on a specific package",
		"enable_logging key value [values]\n"
		"  Where key is one of the packages listed with disp_logging and\n"
		"  value is a space separated list of logging masks that are\n"
		"  OR'd together. The string \"all\" can be used for key to set\n"
		"  the value for all packages\n"
		"  Example: enable_logging communications GNLOG_ERROR GNLOG_VERBOSE\n"
	},
	{
		disable_logging,
		"disable_logging",
		"disable_logging package mask",
		"Disable logging on a specific package",
		"disable_logging key value [values]\n"
		"  Where key is one of the packages listed with disp_logging and\n"
		"  value is a space separated list of logging masks that are\n"
		"  OR'd together. The string \"all\" can be used for key to set\n"
		"  the value for all packages\n"
		"  Example: disable_logging communications GNLOG_ERROR GNLOG_VERBOSE\n"
		},
#endif  /* GN_NO_LOGGING */
	{ 0,						"",			"",						"", NULL }
};

/* command table: holds all the commands with information about their usage */
command_record_t*		command_table = table_main_menu;

/* command table: holds all the commands with information about their usage */
option_record_t table_options[] = {
	{ NULL, "timing", &gTiming },
	{ NULL, "encoding", &gEncoding },
	{ NULL, "echo", &gEcho },
	{ set_comm_callback, "comm_callback", &gCommCallback },
	{ set_upd_callback, "update_callback", &gUpdateCallback },
	{ NULL, "strict", &gStrict },
	{ NULL, "autoselect", &gAutoSelect },
	{ NULL, "verbose", &gVerbose },
	{ NULL, "rawXML", &gDumpRaw },
	{ NULL, "rawUTF8", &gRawUTF8 },
	{ NULL, "paging", &gPaging },
	{ NULL, "swap", &gSwapDisc },
	{ set_brand_duration, "branding", &gBranding },
	{ set_command_repeat, "repeat", &gRepeat },
	{ NULL, "init_ecddb", &gInitECDDB },
	{ NULL, NULL, NULL }
};


command_handler_t find_command(const char* str)
{
	/* Try to find a command that matches this name */
	int		i;
	
	if (!str || !*str)
		return NULL;
	
	for (i = 0; command_table[i].handler != 0; i++)
	{
		/* 
		 * Look for as short a match as we can find.  Ignore for now the 
		 * problem of ambiguous input.
		 */
		if (strnicmp(str, command_table[i].command, strlen(str)) == 0)
		{
			if (gEcho_Reactivate == GN_TRUE)
			{
				gEcho = GN_TRUE;
				gEcho_Reactivate = GN_FALSE;
			}

			if ((gEcho == GN_TRUE) && (!stricmp(command_table[i].command,"print")))
			{
				gEcho = GN_FALSE;
				gEcho_Reactivate = GN_TRUE;
			}

			return (command_table[i].handler);
		}
	}
	
	/* no such luck */
	return NULL;
}


void show_commands(int argc, char * argv[])
{
	int			i;
	gn_char_t	cmp_buffer[64] = "";
	gn_char_t*	tmp_ptr = GN_NULL;


	if (argc != 0)
	{
		for (i = 0; command_table[i].handler != 0; i++)
		{
			strcpy(cmp_buffer,command_table[i].command);
			tmp_ptr = strstr(cmp_buffer,".");
			if (tmp_ptr)
				*tmp_ptr = 0;
			if (strcmp(cmp_buffer,argv[0]) == 0)
			{
				diag_printf("Syntax:%-24s\nDesc: %s\n", command_table[i].syntax, command_table[i].desc);
				if (command_table[i].fulldesc != NULL)
					diag_printf("%s\n", command_table[i].fulldesc);
				break;
			}
		}
		if (command_table[i].handler == 0)
		{
			diag_printf("Command not found\n");
		}
	}
	else
	{
		diag_printf("Available commands (commands may be abbreviated):\n\n");
		diag_printf("%-24s\t%s\n", "Command Name/Syntax", "Description");
		diag_printf("%-24s\t%s\n", "-------------------", "-----------");
		for (i = 0; command_table[i].handler != 0; i++)
		{
			diag_printf("%-24s\t%s\n", command_table[i].syntax, command_table[i].desc);
		}
		diag_printf("\n");
	}
}


static void		print_version(int argc, char* argv[])
{
	print_shell_version();
}


int		prompt_user_proceed(const char* str)
{
	char	input_buff[256];

	diag_printf("%s", str);
	input_buff[0] = 0;
	fgets(input_buff, sizeof(input_buff), stdin);
	if (input_buff[0] == 'q' || input_buff[0] == 'Q')
		return QUIT;
	return PROCEED;
}

/*
 * The commands themselves
 */

void quit(int argc, char * argv[])
{
	gAllDone = 1;
}

void init_ecddb(int argc, char* argv[])
{
#ifdef _BLD_TH_
	if (argc)
	{
		heap_size = atol(argv[0]);
	}
	if (heap_size == 0)
	{
		heap_size = GN_HEAP_SIZE_DEFAULT;
	}
#endif

	initialize_ecddb(heap_size);
}


gn_error_t initialize_ecddb(gn_size_t heap_size)
{
	gn_error_t	error = GNERR_NoError;

#ifndef GN_NO_LOGGING
	error = gnlog_init(NULL);
#endif

#ifdef _BLD_TH_
	if (heap != NULL)
	{
		error = GNERR_Busy;
		goto cleanup;
	}

	if (heap_size != 0)
	{
		heap = (gn_uchar_t*)calloc(heap_size,sizeof(gn_uchar_t));
		if (heap == NULL)
		{
			error = GNERR_NoMemory;
			goto cleanup;
		}
	}
#endif

	error = gnmem_initialize(heap,heap_size);
	if ((error != SUCCESS) && (error != MEMERR_Busy))
		goto cleanup;

	error = gnsys_initialize();

#if !defined(GN_NO_ONLINE)
	/* if ONLINE_LOOKUP_ENCODING is set, set gEncoding for convenience */
	if (error == SUCCESS)
	{
		gn_char_t		online_lookup_encoding[DESCSIZE];

		error = gnconf_get(ONLINE_LOOKUP_ENCODING, online_lookup_encoding);
		if (error == SUCCESS && !strcmp(online_lookup_encoding, "base64"))
			gEncoding = 1;
		else
			gEncoding = 0;
		error = SUCCESS;	/* it's OK if this setting is not in the config file */
	}
#endif /* #if !defined(GN_NO_ONLINE) */

cleanup:
	if (error != SUCCESS)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("\nError %0#10X occurred while initializing eCDDB.\n", error);
		error_message = gnerr_get_error_message(error);
		if (error_message != GN_NULL)
			diag_printf("%s\n\n",error_message);

		if (GNERR_PKG_ID(error) == GNPKG_EmbeddedDBIM)
			diag_printf("Run the \'recover\' command to correct the problem.\n\n");
	}
	else
		diag_printf("eCDDB is initialized\n\n");

	return error;
}


void kill_ecddb(int argc, char* argv[])
{
	shutdown_ecddb();
}


gn_error_t shutdown_ecddb(void)
{
	gn_error_t	error = GNERR_NoError;
	gn_str_t	error_message = GN_NULL;

#ifdef _BLD_TH_
	if (heap == NULL)
	{
		return GNERR_NotInited;
	}
#endif

	error = gnsys_shutdown();
	if (error != SUCCESS)
	{
		diag_printf("\nError %0#10X occurred while shutting down eCDDB.\n", error);
		error_message = gnerr_get_error_message(error);
		if (error_message != GN_NULL)
			diag_printf("%s\n\n",error_message);
	}
	error = gnmem_shutdown();
	if (error != SUCCESS)
	{
		diag_printf("\nError %0#10X occurred while shutting down memory.\n", error);
		error_message = gnerr_get_error_message(error);
		if (error_message != GN_NULL)
			diag_printf("%s\n\n",error_message);
	}
	else
		diag_printf("eCDDB is not initialized\n\n");

#ifdef _BLD_TH_
	free((void*)heap);
	heap = NULL;
#endif

#ifndef GN_NO_LOGGING
	gnlog_shutdown();
#endif

	return error;
}


void recover(int argc, char* argv[])
{
	gn_error_t		error = GNERR_NoError;

	error = gnsys_shutdown();
	if (error == SYSERR_NotInited)
		error = SUCCESS;
#if !defined(_BLD_PROTO_)
	if (error == SUCCESS)
	{
#ifndef GN_NO_LOGGING
		error = gnlog_init(NULL);
#endif

#ifdef _BLD_TH_
		if ((heap == NULL) && (heap_size != 0))
		{
			heap = (gn_uchar_t*)calloc(heap_size,sizeof(gn_uchar_t));
			if (heap == NULL)
			{
				error = GNERR_NoMemory;
				goto cleanup;
			}
		}
#endif

	error = gnmem_initialize(heap,heap_size);
	if ((error != SUCCESS) && (error != MEMERR_Busy))
		goto cleanup;

		error = gnsys_recover();
	}
#endif /* #if defined(_BLD_PROTO_) */
	if (error == SUCCESS)
		error = gnsys_initialize();

#if !defined(GN_NO_UPDATES)
	if (argc)
	{
		gn_upd_init();
		gn_upd_cleanup(argv[0]);
		gn_upd_shutdown();
	}
#endif

cleanup:
	diag_printf("Recover return %X\n", error);
}

void show_profile(int argc, char* argv[])
{
#if !defined(GN_NO_UPDATES)
	int							n;
	gn_uint16_t*				p;
	gn_error_t					error;
	gn_bool_t					did_init = GN_FALSE;
	gn_upd_install_profile_t	profile;

	error = gn_upd_init();
	if (error == SUCCESS)
		did_init = GN_TRUE;
	else
	{
		diag_printf("Error %x initializing\n", error);
		return;
	}

	memset(&profile, 0, sizeof(profile));

	error = gn_upd_load_install_profile(&profile);
	if (error != SUCCESS)
	{
		diag_printf("Error %x retrieving install profile\n", error);
	}
	else
	{
		diag_printf("Update Profile:\nRevision: %d\n", profile.data_revision);
		if (profile.update_level_element_count)
		{
			diag_printf("Installed updates:");
			for (n = 0, p = profile.update_level_list; n < profile.update_level_element_count; n++, p++)
				diag_printf(" %d", *p);
			diag_printf("\n");
		}
		diag_printf("\n");
		gn_upd_cleanup_profile(&profile);
	}

	if (did_init == GN_TRUE)
		gn_upd_shutdown();

#endif /* !defined(GN_NO_UPDATES) */
}

#if defined(GN_NO_ONLINE)
void set_comm_callback(int argc, char* argv[])
{
	diag_printf("Communications callbacks not supported in this configuration.\n");
}
#else
/* this function is in lookup.c */
extern gncomm_cbret_t sampleCallback(gn_connection_t handle, int notification, gn_uint32_t p1, gn_uint32_t p2, gn_uint32_t user_data);

void set_comm_callback(int argc, char* argv[])
{
	if (gCommCallback)
		gntlu_set_lookup_cb(sampleCallback, 0x1234);
	else
		gntlu_set_lookup_cb(NULL, 0);
}
#endif /* #if defined(GN_NO_ONLINE) */

#if defined(GN_NO_UPDATES)
void set_upd_callback(int argc, char* argv[])
{
	diag_printf("Update callbacks not supported in this configuration.\n");
}
#else
/* this function is in cmdhandlers.c */
extern gn_upd_error_t updateCallback(gn_upd_state state, gn_uint32_t count1, gn_uint32_t total1, gn_uint32_t count2, gn_uint32_t total2);

void set_upd_callback(int argc, char* argv[])
{
	if (gUpdateCallback)
		gn_upd_set_callback(updateCallback);
	else
		gn_upd_set_callback(NULL);
}
#endif /* #if defined(GN_NO_UPDATES) */


static void set_brand_duration(int argc, char* argv[])
{
	if (gBranding)
	{
		int	delay = 0;

		if (argc > 2)
			delay = atoi(argv[2]);

		brand_set_duration_on_powerup(delay);
		brand_set_duration_on_lookup(delay);
		brand_set_duration_on_update(delay);
	}
	else
	{
		brand_set_duration_on_powerup(0);
		brand_set_duration_on_lookup(0);
		brand_set_duration_on_update(0);
	}
}


static void set_command_repeat(int argc, char* argv[])
{
	if (gRepeat)
	{
		if (argc > 2)
			gRepeatCount = atoi(argv[2]);
	}
	else
	{
		gRepeatCount = 0;
	}
}


option_record_t* get_option_info(char* opt)
{
	option_record_t*	opt_tab = &table_options[0];

	while (opt_tab->option)
	{
		if (!strncmp(opt_tab->option, opt, strlen(opt)))
			return opt_tab;
		opt_tab++;
	}

	return NULL;
}

void set_opt(int argc, char* argv[])
{
	option_record_t*	opt_tab;
	int					newval = -1;

	if (argc == 0)
	{
		diag_printf("No options specified\n");
		return;
	}

	opt_tab = get_option_info(argv[0]);
	if (opt_tab == NULL)
	{
		diag_printf("Unknown option specified: %s\n", argv[0]);
		return;
	}

	if (argc == 1)
	{
		diag_printf("Current setting for '%s' is: %s\n\n", opt_tab->option, *opt_tab->value ? "ON" : "OFF");
		return;
	}

	if (!strcmp(argv[1], "on") || !strcmp(argv[1], "ON"))
		newval = 1;
	else if (!strcmp(argv[1], "off") || !strcmp(argv[1], "OFF"))
		newval = 0;

	if (newval != -1)
	{
		*opt_tab->value = newval;
		if (opt_tab->handler)
			(*opt_tab->handler)(argc, argv);
	}
}

void disp_opt(int argc, char* argv[])
{
	option_record_t*	opt_tab;
	int					newval = -1;

	if (argc == 0)
	{
		opt_tab = &table_options[0];

		while (opt_tab->option)
		{
			diag_printf("Current setting for '%s' is: %s\n", opt_tab->option, *opt_tab->value ? "ON" : "OFF");
			opt_tab++;
		}
		diag_printf("\n");
		return;
	}

	opt_tab = get_option_info(argv[0]);
	if (opt_tab == NULL)
	{
		diag_printf("Unknown option specified: %s\n", argv[0]);
		return;
	}

	diag_printf("Current setting for '%s' is: %s\n\n", opt_tab->option, *opt_tab->value ? "ON" : "OFF");
	return;
}

void lookup_disc_in_drive_ex(int argc, char* argv[], int location)
{
#if defined (GN_NO_TOC_READING)
	diag_printf("TOC reading from drive not supported.\n");
#else	
	char 	toc[1024];
	int		error;
	/* was a drive letter specified? */
	if (argc > 0 && (!strcmp(*argv, "-n") ||!strcmp(*argv, "-N")))
	{
		if (argc < 2)
		{
			diag_printf("No Id specified\n");
			return;
		}
		argc--;
		argv++;
		while (argc--)
		{
			lookup_id(*argv, location);
		}
		return;
	}
	else if (argc > 0)
		error = read_toc_from_drive(argv[0], toc, sizeof(toc));
	else	
		error = read_toc_from_default_drive(toc, sizeof(toc));

	if (error == 0)
		lookup_toc(toc, location);
	else
		diag_printf("Error %d reading the TOC\n", error);
#endif
}

void lookup_disc_from_cmdline_ex(int argc, char* argv[], int location)
{
	char	toc[1024] = "";
	int		i;
	char*	p;

	if (argc == 0)
	{
		diag_printf("Enter TOC: ");
		fgets(toc, sizeof(toc), stdin);
		if (toc[0] == 0)
		{
			diag_printf("Invalid TOC\n");
			return;
		}
		p = strchr(toc, 0xA);
		if (p)
			*p = 0;
	}
	else
	{
		for(i = 0; i < argc; i++)
		{
			strcat(toc, argv[i]);
			strcat(toc, " ");
		}
	}

	lookup_toc(toc, location);
}

void lookup_disc_from_file_ex(int argc, char* argv[], int location)
{
	char 		toc_file[512];
	int 		result = 0;
	gn_bool_t	is_id = GN_FALSE;

	if (argc == 0)
	{
		/* get filename if not specified */
		result = ask_user_for_file(toc_file, sizeof(toc_file));
		if (result != 0)
		{
			return;
		}
	}
	else
	{
		if (*argv[0] == '-' && *(argv[0] + 1) == 'i')
		{
			is_id = GN_TRUE;
			argc--;

			if (argc == 0) 
			{
				result = ask_user_for_file(toc_file, sizeof(toc_file));
				if (result != 0)
					return;
			}
			else
			{
				/* filename is argv[0] */
				strncpy(toc_file, argv[1], sizeof(toc_file) - 1);
			}
		}
		else
		{
			/* filename is argv[0] */
			strncpy(toc_file, argv[0], sizeof(toc_file) - 1);
		}
	}
	if (is_id == GN_TRUE)
		result = lookup_id_from_file(toc_file, location);
	else
		result = lookup_toc_from_file(toc_file, location);
}

void lookup_disc_from_cmdline(int argc, char *argv[])
{
	lookup_disc_from_cmdline_ex(argc, argv, LOC_DB);
}

void lookup_disc_from_cmdline_online(int argc, char *argv[])
{
#if defined (GN_NO_ONLINE)
	diag_printf("Online Lookup not supported.\n");
#else	
	lookup_disc_from_cmdline_ex(argc, argv, LOC_ONLINE);
#endif
}

void lookup_disc_in_drive(int argc, char* argv[])
{
	lookup_disc_in_drive_ex(argc, argv, LOC_DB);
}

void lookup_disc_from_file(int argc, char* argv[])
{
	lookup_disc_from_file_ex(argc, argv, LOC_DB);
}

void lookup_disc_in_drive_online(int argc, char* argv[])
{
#if defined (GN_NO_ONLINE)
	diag_printf("Online Lookup not supported.\n");
#else	
	lookup_disc_in_drive_ex(argc, argv, LOC_ONLINE);
#endif
}

void lookup_disc_from_file_online(int argc, char* argv[])
{
#if defined (GN_NO_ONLINE)
	diag_printf("Online Lookup not supported.\n");
#else	
	lookup_disc_from_file_ex(argc, argv, LOC_ONLINE);
#endif
}

void lookup_disc_in_drive_cache(int argc, char* argv[])
{
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	diag_printf("Cache Lookup not supported.\n");
#else	
	lookup_disc_in_drive_ex(argc, argv, LOC_CACHE);
#endif
}

void lookup_disc_from_file_cache(int argc, char* argv[])
{
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	diag_printf("Online Lookup not supported.\n");
#else	
	lookup_disc_from_file_ex(argc, argv, LOC_CACHE);
#endif
}

void lookup_disc_in_drive_all(int argc, char* argv[])
{
	int		location = LOC_ALL;
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	location &= ~LOC_CACHE;
#endif
#if defined (GN_NO_ONLINE)
	location &= ~LOC_ONLINE;
#endif
	lookup_disc_in_drive_ex(argc, argv, location);
}

void lookup_disc_from_file_all(int argc, char* argv[])
{
	int		location = LOC_ALL;
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	location &= ~LOC_CACHE;
#endif
#if defined (GN_NO_ONLINE)
	location &= ~LOC_ONLINE;
#endif
	lookup_disc_from_file_ex(argc, argv, location);
}

void batch_tocs_in_cache(int argc, char* argv[])
{
#if (defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE))
	diag_printf("Cache not supported.\n");
#else	
	int 		error = 0;
	int			response = PROCEED;
	char		tocbuff[TOCBUFSIZE];

	/* shutdown lookup layer if swapping */
	if (gSwapDisc == GN_TRUE)
	{
		error = gntlu_shutdown();
		error = gnfs_shutdown();

		/* instruct user to eject data disc */
		response = prompt_user_proceed("Remove dCDDB data disc, enter <RETURN> to continue");
	}

	while (error == SUCCESS && response == PROCEED)
	{
		/* prompt to insert first music CD */
		response = prompt_user_proceed("Insert Audio CD, enter <RETURN> to continue");

		/* read TOC */
		if (argc > 0)
			error = read_toc_from_drive(argv[0], tocbuff, sizeof(tocbuff));
		else
			error = read_toc_from_default_drive(tocbuff, sizeof(tocbuff));

		if (error == SUCCESS)
			error = add_toc_2_cache(tocbuff);

		if (error == SUCCESS)
			diag_printf("Adding TOC to cache:\n%s\n",tocbuff);

		/* prompt to stop reading TOCs or continue */
		response = prompt_user_proceed("Press <RETURN> to continue with another CD, 'q' to start looking up discs: ");
	}

	/* reinitialize lookup layer if swapping */
	if (gSwapDisc == GN_TRUE)
	{
		/* instruct user to reinsert data disc */
		response = prompt_user_proceed("Insert dCDDB data disc, enter <RETURN> to continue");
		shell_sleep(2);
		while (
			((error = gnfs_initialize()) != SUCCESS)
			&&
			(prompt_user_proceed("Cannot read the data. Press <RETURN> to retry, 'q' to quit: ") == PROCEED)
			);
		if (error == SUCCESS)
			error = gntlu_initialize();
	}

	/* lookup the TOCs which were placed into the cache */
	if (error == SUCCESS)
		error = update_cache(LOC_DB);

	if (error != SUCCESS)
	{
		diag_printf("Error %X doing batch operation\n", error);	/* TODO: better error handling */
	}
	else
	{
		dump_cache(1);
	}
#endif /* #if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE) */
}

void add_toc_from_file_to_cache(int argc, char* argv[])
{
#if (defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE))
	diag_printf("Cache not supported.\n");
#else	
	char 		toc_file[512];
	int 		result = 0;

	if (argc == 0)
	{
		/* get filename if not specified */
		result = ask_user_for_file(toc_file, sizeof(toc_file));
		if (result != 0)
		{
			return;
		}
	}
	else
	{
		/* filename is argv[0] */
		strncpy(toc_file, argv[0], sizeof(toc_file) - 1);
	}
	result = add_toc_from_file(toc_file);
#endif /* #if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE) */
}

void delete_toc_from_file_from_cache(int argc, char* argv[])
{
#if (defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE))
	diag_printf("Cache not supported.\n");
#else	
	char 		toc_file[512];
	int 		result = 0;

	if (argc == 0)
	{
		/* get filename if not specified */
		result = ask_user_for_file(toc_file, sizeof(toc_file));
		if (result != 0)
		{
			return;
		}
	}
	else
	{
		/* filename is argv[0] */
		strncpy(toc_file, argv[0], sizeof(toc_file) - 1);
	}
	result = delete_toc_from_file(toc_file);
#endif /* #if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE) */
}

void  lookup_tocs_in_cache(int argc, char* argv[])
{
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	diag_printf("Cache not supported.\n");
#else
#if defined (GN_NO_ONLINE)
	int		location = LOC_DB; /* default */
#else
	int		location = LOC_DB|LOC_ONLINE; /* default */
#endif /* #if defined (GN_NO_ONLINE) */

	if (argc > 0)
	{
		if (!strcmp(argv[0], "online"))
			location = LOC_ONLINE;
		else if (!strcmp(argv[0], "local"))
			location = LOC_DB;
		else if (!strcmp(argv[0], "both"))
			location = (LOC_DB|LOC_ONLINE);
		else
		{
			diag_printf("Unrecognized option to 'ucache': %s\n", argv[0]);
			return;
		}

	}
	update_cache(location);
#endif
}

void  list_tocs_in_cache(int argc, char* argv[])
{
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	diag_printf("Cache not supported.\n");
#else
	int			toc_id = 0;
	int			verbose = 0;
	char*		arg;

	if (argc > 0)
	{
		arg = argv[0];
		if (*arg == '/' || *arg == '-')
		{
			if (*(arg+1) == 'v' || *(arg+1) == 'V')
				verbose = 1;
		}
		else
		{
			toc_id = atoi(argv[0]);
			dump_cache_entry(toc_id, verbose);
			return;
		}
		if (argc > 1)
		{
			toc_id = atoi(argv[1]);
			dump_cache_entry(toc_id, verbose);
			return;
		}
	}
	dump_cache(verbose);
#endif
}

void  delete_cache(int argc, char* argv[])
{
#if defined (GN_NO_CACHE) && !defined(GN_MEM_CACHE)
	diag_printf("Cache not supported.\n");
#else
	gn_error_t	error = SUCCESS;

	error = gncache_delete(GN_TRUE);
	if ((error != SUCCESS) && (error != FSERR_NotFound))
	{
		diag_printf("Error (%X) deleting cache.\n", error);
		GNERR_LOG_ERR(error);
	}
#endif
}

void update_database_from_file(int argc, char* argv[])
{
#if defined(GN_NO_UPDATES)
	diag_printf("Updates not supported in this build\n");
#else
	char 	update_file[512];
	int		result = 0;

	if (argc == 0)
	{
		/* get filename if not specified */
		result = ask_user_for_file(update_file, sizeof(update_file));
		if (result != 0)
		{
			return;
		}
	}
	else
	{
		/* filename is argv[0] */
		strncpy(update_file, argv[0], sizeof(update_file) - 1);
	}

	do_local_update(update_file);

#endif
}

void online_update(int argc, char* argv[])
{
#if defined(GN_NO_UPDATES)
	diag_printf("Updates not supported in this build\n");
#elif defined(GN_NO_ONLINE) || defined(GN_NO_ONLINE_UPDATES)
	diag_printf("Online Update not supported.\n");
#else
	int result = do_online_update();

	if (result != 0)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("Error %#X performing online update\n", result);

		error_message = gnerr_get_error_message(result);
		if(error_message != GN_NULL)
		{
			diag_printf("%s\n\n", error_message);
		}
	}
#endif
}


void pc_update(int argc, char* argv[])
{
#if defined(GN_NO_UPDATES)
	diag_printf("Updates not supported in this build\n");
#elif defined(GN_NO_PC_UPDATES)
	diag_printf("PC Update not supported.\n");
#else
	int result = do_pc_update();

	if (result != 0)
	{
		diag_printf("Error %d (%#X) performing PC update.\n", result, result);

		if (result >= 10000 && result < 11000)
		{
			diag_printf("\nThis is likely a Winsock error resulting from a problem in the connection\n"
			       "between the sample PC connectivity code in this shell application and the\n"
				   "PC Connectivity Application. Consult Winsock.h for a description of the\n"
				   "Winsock errors, and also make sure that the PC Update Application is running\n"
				   "before executing this command.\n\n");
		}
		else
		{
			gn_str_t error_message = gnerr_get_error_message(result);

			if (error_message != GN_NULL)
			{
				diag_printf("%s\n\nSince the example protocol used for communicating between this\n"
				       "shell and the PC Connectivity Application has no provision for sending\n"
					   "error information, it is possible that this error is a by-product of an\n"
					   "error condition detected by the PC Connectivity Application. It is up\n"
					   "to the developer to devise a protocol capable of communicating such\n"
					   "information, if desired.\n\n", error_message);
			}
		}
	}
#endif
}


int ask_user_for_file(char* filename_buffer, size_t filename_buffer_size)
{
	static char user_file[512];
	int result = 0;
	size_t	length;
	char*	ptr = NULL;

	/* Running in non-interactive mode we can't ask for input */
	if (!gInteractive)
		return 1;
		
	user_file[0] = 0;
	diag_printf("Type in filename: ");

	fgets(user_file, sizeof user_file, stdin);

	/* remove trailing newline, if present */
	length = strlen(user_file);
	ptr = user_file;
	if (length > 0 && user_file[length - 1] == '\n')
	{
		length--;
		user_file[length] = '\0';

		if ((user_file[0] == '\"') && (user_file[length-1] == '\"'))
		{
			/* quoted string needs quotes stripped for subsequent file operations to succeed */
			*ptr = '\0';
			ptr++;
			length--;
			user_file[length] = '\0';
		}
	}
	if (filename_buffer_size - 1 >= length)
	{
		strcpy(filename_buffer, ptr);
	}
	else
	{
		result = -1;
	}

	return result;
}


static void set_conf(int argc, char* argv[])
{
	char	buffer[256] = "";
	char*	value = NULL;

	if (argc == 0)
		return;

	if (argc == 1)
	{
		/* get the value for the configuration key */
		diag_printf("%s: ",argv[0]);
		fgets(buffer, sizeof(buffer),stdin);
		/* get rid of the \n */
		if (strlen(buffer))
			buffer[strlen(buffer)-1] = '\0';
		value = buffer;
	}
	else
		value = argv[1];

	set_configuration(argv[0],value);
}


static void disp_conf(int argc, char* argv[])
{
	disp_configuration();
}


static void save_conf(int argc, char* argv[])
{
	save_configuration();
}


static void	 redirect_input(int argc, char* argv[])
{
	gn_char_t		input_redir_filename[FILENAME_MAX] = "";
	int				result = 0;

	if (argc == 1)
		strcpy(input_redir_filename,argv[0]);
	else
	{
		result = ask_user_for_file(input_redir_filename,sizeof(input_redir_filename));
		if (result != 0)
			return;
	}

	gRepeating = GN_TRUE;
	/* sit here and process all commands */
	process_commands(input_redir_filename);
	gRepeating = GN_FALSE;
}


void wait(int argc, char* argv[])
{
	gn_uint32_t		sec = 0;

	if ((argc == 0) || strnicmp(argv[0], "keystroke", strlen(argv[0])) == 0)
	{
		/* this won't work properly if output is redirected */
		diag_printf("\nPress enter to continue");
		getchar();
	}
	else
	{
		sec = atol(argv[0]);
		shell_sleep(sec);
	}
}


void print(int argc, char* argv[])
{
	int		i = 0;

	for(i = 0; i < argc; i++)
		diag_printf("%s ", argv[i]);
	diag_printf("\n");
}


static void set_lkup_type(int argc, char* argv[])
{
	char	buffer[256] = "";
	char*	value = NULL;

	if (argc == 0)
	{
		/* get the value */
		diag_printf("Lookup Type : ");
		fgets(buffer, sizeof(buffer),stdin);
		/* get rid of the \n */
		if (strlen(buffer))
			buffer[strlen(buffer)-1] = '\0';
		value = buffer;
	}
	else if (argc == 1)
		value = argv[0];

	set_lookup_type_str(value);
}


static void disp_lkup_type(int argc, char* argv[])
{
	display_lookup_type();
}


#if !defined(GN_NO_UPDATES)
static void		get_free_disc_space(int argc, char* argv[])
{
	gn_error_t		error = GNERR_NoError;
	gn_uint64_t		free_space = {0};
	gn_uint32_t		fs = {0};

	error = gnfs_get_disk_free_space(GN_NULL,&free_space);
	if (error != GNERR_NoError)
		return;

#ifdef PLATFORM_WIN32
	diag_printf("%25s%I64d\n","Free disk space [bytes]: ",free_space);
#elif defined PLATFORM_UNIX
	diag_printf("Free disk space [bytes]: %lld\n", free_space);
#endif

	fs = (gn_uint32_t)(free_space/1024);
	diag_printf("%25s%ld", "[KB]: ",fs);
	fs = (gn_uint32_t)(((free_space%1024)*100)/1024);
	diag_printf(".%02ld\n",fs);

	fs = (gn_uint32_t)(free_space/(1024*1024));
	diag_printf("%25s%ld", "[MB]: ",fs);
	fs = (gn_uint32_t)(((free_space%(1024*1024))*100)/(1024*1024));
	diag_printf(".%02ld\n",fs);

	fs = (gn_uint32_t)(free_space/(1024*1024*1024));
	diag_printf("%25s%ld","[GB]: ",fs);
	fs = (gn_uint32_t)(((free_space%(1024*1024*1024))*100)/(1024*1024*1024));
	diag_printf(".%02ld\n",fs);
}


#ifdef _BLD_TH_
static void		set_free_disc_space(int argc, char* argv[])
{
	gn_uint64_t		temp = {0};
	gn_error_t		error = GNERR_NoError;
	gn_uint64_t		free_space = {0};
	gn_uint64_t		file_size = {0};
	int				eatdisk = -1;
	char			buffer[256] = "";
	gn_uint64_t		seek_res = -1;
	int				write_res = 0;
	char*			arg0 = NULL;
	char*			arg1 = NULL;
	struct _stati64	file_stat = {0};


	if (argc < 1)
	{
		diag_printf("\nEnter maximum disc space: ");

		if (fgets(buffer, sizeof(buffer), stdin) == NULL)
			return;

		/* get rid of the \n */
		buffer[strlen(buffer)-1] = 0;

		arg0 = buffer;
		arg1 = strstr(buffer," ");
		if (arg1)
			arg1++;
	}
	else
	{
		arg0 = argv[0];
		if (argc > 1)
			arg1 = argv[1];
	}

	if (!arg0)
		return;

	temp = _atoi64(arg0);

	if (arg1)
	{
		if (!stricmp(arg1,"KB"))
		{
			temp *= 1024;
		}
		else if (!stricmp(arg1,"MB"))
		{
			temp *= (1024*1024);
		}
		else if (!stricmp(arg1,"GB"))
		{
			temp *= (1024*1024*1024);
		}
	}

	error = gnfs_get_disk_free_space(GN_NULL,&free_space);
	if (error != GNERR_NoError)
		return;

	diag_printf("Setting disk free space to %I64d bytes.\n", temp);

	file_size = free_space - temp;
	if (file_size < 0)
	{
		diag_printf("\nSorry, free disk space is already at %I64d.\n",free_space);
		return;
	}

	if (_access(eatdisk_filename,00) != -1)
	{
		_stati64(eatdisk_filename,&file_stat);
		file_size += file_stat.st_size;
	}

	eatdisk = _creat(eatdisk_filename,_S_IREAD|_S_IWRITE);
	if (eatdisk == -1)
		return;

	seek_res = _lseeki64(eatdisk,file_size,SEEK_SET);
	if (seek_res == -1)
	{
		_close(eatdisk);
		return;
	}

	write_res = _write(eatdisk,buffer,1);

	_commit(eatdisk);
	_close(eatdisk);

	diag_printf("\n Successful setting\n");
}


static void reset_free_disc_space(int argc, char* argv[])
{
	remove(eatdisk_filename);
}

#endif


static void reset_db(int argc, char* argv[])
{
	gn_error_t	error = GNERR_NoError;

	error = gnsys_reset_db();

	diag_printf("Reset DB return %#X\n", error);
}


static void revert_db(int argc, char* argv[])
{
	gn_error_t	error = GNERR_NoError;

	error = gnsys_revert_db();

	diag_printf("Revert DB return %#X\n", error);
}
#endif

#if !defined(GN_NO_LOGGING)
static void display_logging(int argc, char* argv[])
{
	do_display_logging(argc, argv);
}

static void enable_logging(int argc, char *argv[])
{
	do_enable_logging(argc, argv);
}

static void disable_logging(int argc, char *argv[])
{
	do_disable_logging(argc, argv);
}

#endif  /* GN_NO_LOGGING */
