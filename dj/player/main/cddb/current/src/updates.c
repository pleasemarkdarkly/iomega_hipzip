/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * updates.c
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include "updates.h"
#include <extras/cddb/gn_upd.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_memory.h>
//#include "branding.h"
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_checksum.h>
#include <util/debug/debug.h>

#include <main/cddb/CDDBEvents.h>
#include <core/events/SystemEvents.h>

DEBUG_USE_MODULE(CDDBHELPER);

/*
 * Typedefs
 */

#if !defined(GN_NO_UPDATES)

typedef struct
{
	gn_upd_state		state;
	const char* 		state_str;
} upd_state_str_t;

#endif


/*
 * Constants
 */

#if !defined(GN_NO_UPDATES)

static upd_state_str_t state_strings[] =
{
	UPDSTATE_Default,			"Default",			
	UPDSTATE_Update_Start,		"Update_Start",		
	UPDSTATE_Download_Start,	"Download_Start",		
	UPDSTATE_Downloading,		"Downloading",		
	UPDSTATE_Download_End,		"Download_End",		
	UPDSTATE_Install_Start,		"Install_Start",		
	UPDSTATE_Installing,		"Installing",			
	UPDSTATE_Install_End,		"Install_End",		
	UPDSTATE_Integrate_Start,	"Integrate_Start",	
	UPDSTATE_Integrating,		"Integrating",		
	UPDSTATE_Integrate_End,		"Integrate_End",		
	UPDSTATE_Backup_Start,		"Backup_Start",		
	UPDSTATE_BackingUp,			"BackingUp",			
	UPDSTATE_Backup_End,		"Backup_End",			
	UPDSTATE_Update_End,		"Update_End",
	0, 0
};

#endif


/*
 * Prototypes
 */

static gn_error_t
process_update_file(const char* filename);

static gn_upd_error_t
display_install_profile(gn_uint16_t interactive_level);

#if !defined(GN_NO_ONLINE_UPDATES) || !defined(GN_NO_PCUPDATES)

static void
print_update_diagnostics(gn_size_t index, gn_upd_file_list_ref_t update_files);

static gn_error_t
process_remote_update_files(gn_upd_file_list_ref_t update_files, gn_transporter_t* transporter);

static gn_error_t
do_remote_update(gn_registrar_t* registrar, gn_transporter_t* transporter);

#endif /* #if !defined(GN_NO_ONLINE_UPDATES) || !defined(GN_NO_PCUPDATES) */


/*
 * Implementations
 */

#if !defined(GN_NO_UPDATES)

static const char *update_status_str(gn_upd_state state)
{
	upd_state_str_t*	p = &state_strings[0];

	/* special case: Default */
	if (state == UPDSTATE_Default)
		return p->state_str;

	p++;

	while (p->state)
	{
		if (p->state == state)
			return p->state_str;
		p++;
	}
	return "Unknown";
}


/* callback routine for displaying status while processing updates */
gn_upd_error_t updateCallback(gn_upd_state state, gn_uint32_t count1, gn_uint32_t total1, gn_uint32_t count2, gn_uint32_t total2)
{
	const char*		status = update_status_str(state);

	if (total1 && total2)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "Update status: %s (%d of %d, %d of %d)\n", status, count1, total1, count2, total2);
	}
	else if (total1)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "Update status: %s (1: %d of %d)\n", status, count1, total1);
	}
	else if (total2)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "Update status: %s (2: %d of %d)\n", status, count2, total2);
	}
	else
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "Update status: %s\n", status);
	}
	return UPDERR_NoError;
}


void do_local_update(const char* filename)
{
	gn_error_t	error = GENERR_NoError;

	if (!filename)
		return;

	/* branding display on update */
//	brand_display_on_update();

	error = process_update_file(filename);

	if (error != GNERR_NoError)
	{
		gn_str_t	error_message = GN_NULL;

        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error updating local from %s: 0x%x: %s\n", filename, error, gnerr_get_error_message(error));
	}
}


static gn_error_t
process_update_file(const char* filename)
{
	gn_upd_error_t		error = UPDERR_NoError;
	gn_uint16_t			summary_element_count = 0;
	gn_uint16_t			interactive_level = 1;

	if (!filename)
		return GENERR_InvalidArg;

	DEBUG(CDDBHELPER, DBGLEV_INFO, "Processing update file %s:\n", filename);

	/* initialize the subsystem */
	error = gn_upd_init();
	if (error != UPDERR_NoError)
		return error;

	error = display_install_profile(interactive_level);
	if (error != GNERR_NoError)
		return error;

	/* figure out what's installed on the device, and what we've got to offer. */
	error = gn_upd_determine_required_updates(filename,GN_NULL,&summary_element_count);
	if (error != UPDERR_NoError)
		return error;

	if (!summary_element_count)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "The database does not require any updates from %s.\n", filename);
		goto cleanup;
	}

	DEBUG(CDDBHELPER, DBGLEV_INFO, "Updating database...\n");

	error = gn_upd_update(filename);

	if ((error == SUCCESS) && (interactive_level > 0))
	{
		display_install_profile(interactive_level);
	}

cleanup:
	if (error == GNERR_NoError)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "Successfully processed update file %s\n",filename);
	}

	gn_upd_shutdown();

	return error;
}


static gn_upd_error_t
display_install_profile(gn_uint16_t interactive_level)
{
	gn_upd_error_t				error = UPDERR_NoError;
	gn_uint16_t					i = 0;
	gn_upd_install_profile_t	profile = {0};


	if (interactive_level < 1)
		return error;

	error = gn_upd_load_install_profile(&profile);
	if (error != UPDERR_NoError)
		return error;

	DEBUGP(CDDBHELPER, DBGLEV_INFO, "Install Profile:\n    Data Revision: %d\n    Update Level Count: %d\n",
		profile.data_revision,profile.update_level_element_count);

	if (profile.update_level_element_count)
	{
		DEBUGP(CDDBHELPER, DBGLEV_INFO, "        Update Level(s):");

		for (i = 0; i < profile.update_level_element_count; i++)
			DEBUGP(CDDBHELPER, DBGLEV_INFO, " %d",profile.update_level_list[i]);

		DEBUGP(CDDBHELPER, DBGLEV_INFO, "\n");
	}

	gn_upd_cleanup_profile(&profile);

	return error;
}


#if !defined(GN_NO_ONLINE_UPDATES) || !defined(GN_NO_PCUPDATES)
static void
print_update_diagnostics(gn_size_t index, gn_upd_file_list_ref_t update_files)
{
	gn_error_t error = 0;
	gn_uint32_t date = 0;
	gn_size_t size = 0;
	gn_cstr_t file_url = 0;
	gn_cstr_t checksum_alg = 0;
	gn_cstr_t checksum_value = 0;

	DEBUGP(CDDBHELPER, DBGLEV_INFO, "File %d of %d", index + 1, gn_upd_get_file_count(update_files));

	error = gn_upd_get_update_file_date(update_files, index, &date);

	if (error == 0) {
		int year = date / 10000;
		int month = (date / 100) % 100;
		int day = date % 100;
		char *month_str[] = { "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December" };

		DEBUGP(CDDBHELPER, DBGLEV_INFO, " -- %s %d, %d", month_str[month - 1], day, year);
	}

	error = gn_upd_get_update_file_size(update_files, index, &size);

	if (error == 0) {
		DEBUGP(CDDBHELPER, DBGLEV_INFO, " -- %d bytes", size);
	}

	DEBUGP(CDDBHELPER, DBGLEV_INFO, "\n");

	error = gn_upd_extract_update_file_url(update_files, index, &file_url);

	if (error == 0) {
		DEBUGP(CDDBHELPER, DBGLEV_INFO, "Download URL: %s\n", file_url);
	}

	error = gn_upd_extract_update_file_checksum(update_files, index, &checksum_alg, &checksum_value);

	if (error == 0)
	{
		if (checksum_alg != 0 && checksum_value != 0) {
			DEBUGP(CDDBHELPER, DBGLEV_INFO, "Checksum: %s (%s)\n", checksum_value, checksum_alg);
		} else {
			DEBUGP(CDDBHELPER, DBGLEV_INFO, "Checksum not available.\n");
		}
	}

	gn_smart_free((void**)&file_url);
	gn_smart_free((void**)&checksum_alg);
	gn_smart_free((void**)&checksum_value);
}

static gn_error_t
verify_update_file(gn_upd_file_list_ref_t update_files, gn_size_t index, gn_cstr_t update_file)
{
	gn_error_t error = 0;
	gn_size_t expected_size = 0;
	gn_cstr_t checksum_alg = 0;
	gn_cstr_t expected_value = 0;
	gn_str_t measured_value = 0;

	/* first verify that the downloaded file length matches the expected value */
	error = gn_upd_get_update_file_size(update_files, index, &expected_size);

	if (error == 0)
	{
		gn_handle_t handle = gnfs_open(update_file, FSMODE_ReadOnly);

		if (handle != FS_INVALID_HANDLE)
		{
			gn_error_t close_error = 0;

			gn_foffset_t measured_size = gnfs_get_eof(handle);

			if ((gn_size_t)measured_size != expected_size)
			{
				DEBUG(CDDBHELPER, DBGLEV_ERROR, "File %s is the wrong length; expected %d bytes, measured %d bytes.\n",
				       update_file, (unsigned long)expected_size, (unsigned long)measured_size);

				error = GNERR_ERR_CODE(GENERR_InvalidFormat);
			}

			close_error = gnfs_close(handle);

			if (error == 0)
				error = close_error;
		}
		else
			error = gnfs_get_error();
	}

	/* now verify the file checksum */
	if (error == 0)
		error = gn_upd_extract_update_file_checksum(update_files, index, &checksum_alg, &expected_value);

	if (error == 0)
	{
		if (checksum_alg != 0 && expected_value != 0)
		{
			error = gn_verify_file_checksum(update_file, checksum_alg, expected_value, &measured_value);

			if (error == 0 || error == UTILERR_ChecksumMismatch)
			{
				DEBUG(CDDBHELPER, DBGLEV_INFO, "Checksum (%s) %s: expected %s, measured %s\n", checksum_alg,
				       (error == UTILERR_ChecksumMismatch) ? "MISMATCH" : "MATCH",
					   expected_value, (measured_value != 0) ? measured_value : "n/a");
			}
			else
			{
				DEBUG(CDDBHELPER, DBGLEV_ERROR, "Checksum error 0x%x\n", (long)error);
			}
		}
		else
		{
			error = gn_verify_file_checksum(update_file, CHECKSUM_CRC32, 0, &measured_value);

			if (error == 0)
			{
				DEBUG(CDDBHELPER, DBGLEV_INFO, "Measured checksum (crc32): %s\n", measured_value);
			}
			else
			{
				DEBUG(CDDBHELPER, DBGLEV_ERROR, "Checksum error 0x%x\n", (long)error);
			}
		}
	}

	gn_smart_free((void**)&checksum_alg);
	gn_smart_free((void**)&expected_value);
	gn_smart_free((void**)&measured_value);

	return error;
}

/* It is up to the ecddb customer to decide which files to download.   */
/* As an example, we provide here code to download and install all the */
/* files that the server has returned. Custom implementations may wish */
/* to examine each update file more closely, downloading only those    */
/* files of a particular size, or perhaps giving the user the option   */
/* of choosing which files to download.                                */

static gn_error_t
process_remote_update_files(gn_upd_file_list_ref_t update_files, gn_transporter_t* transporter)
{
	gn_size_t	fileIndex = 0;
	gn_error_t	error = 0;
	gn_size_t	file_count = gn_upd_get_file_count(update_files);

	/* loop through all the files in update_files */
	for (fileIndex = 0; error == 0 && fileIndex < file_count; fileIndex++)
	{
		gn_cstr_t	file_url = 0;
		gn_cstr_t	update_file = 0;

		/* get the file's URL */
		error = gn_upd_extract_update_file_url(update_files, fileIndex, &file_url);

		/* print some diagnostics */
		print_update_diagnostics(fileIndex, update_files);

        /* tell the UI */
        put_event(EVENT_CDDB_ONLINE_UPDATE_DOWNLOADING,
            (void*)((gn_upd_get_file_count(update_files) << 16) | fileIndex + 1));

		/* get the file at that URL */
		if (error == 0 && file_url != 0)
		{
			error = gn_upd_download_update_file(file_url, &update_file, transporter);
		}

		/* install that file */
		if (error == 0 && update_file != 0)
		{
            /* tell the UI */
            put_event(EVENT_CDDB_ONLINE_UPDATE_PROCESSING,
                (void*)((gn_upd_get_file_count(update_files) << 16) | fileIndex + 1));

			error = verify_update_file(update_files, fileIndex, update_file);

			if (error == 0)
				error = process_update_file(update_file);

			/* If there is a crash inside process_update_file, then the  */
			/* file just downloaded will not be deleted. Therefore, we   */
			/* recommend that developers add code to delete all files    */
			/* whose names end in ".pkg" after a crash, or, if it is     */
			/* not feasable to detect such a condition, then they should */
			/* be deleted before performing an update operation.         */
		}

		if (update_file != 0)
		{
			/* We must delete the update file regardless of the  */
			/* success of processing it. However, errors from    */
			/* processing the file are more "interesting" than   */
			/* errors from deleting it. Therefore, if processing */
			/* the file was not successful, we must ignore the   */
			/* result code from the file delete operation.       */

			int delete_error = gnfs_delete(update_file);
			
			if (error == 0)
				error = delete_error;
		}

		/* clean up */
		gn_smart_free((void**)&file_url);
		gn_smart_free((void**)&update_file);
	}

	return error;
}


static gn_error_t
do_remote_update(gn_registrar_t* registrar, gn_transporter_t* transporter)
{
	gn_error_t					error = 0;
	gn_upd_install_profile_t	current_profile = { 0 };
	gn_upd_file_list_ref_t		update_files = 0;

	/* branding display on update */
//	brand_display_on_update();

	error = gn_upd_init();

	/* get current update profile */
	if (error == 0)
	{
		error = gn_upd_load_install_profile(&current_profile);
	}

	/* send it to the server and get some URLs back */
	if (error == 0)
	{
		error = gn_upd_get_remote_update_files(&current_profile, registrar, transporter, &update_files);

		/* we no longer need the installed update profile */
		gn_upd_cleanup_profile(&current_profile);
	}

	/* shutdown the update module here because process_update_file */
	/* will initialize it again for us */
	gn_upd_shutdown();

	/* now it's time to process the list */
	if (error == 0 && update_files != 0)
	{
		DEBUG(CDDBHELPER, DBGLEV_INFO, "%d file(s) to download and process.\n", gn_upd_get_file_count(update_files));

		error = process_remote_update_files(update_files, transporter);

		gn_upd_dispose_file_list(update_files);
	}

	return error;
}
#endif /* #if !defined(GN_NO_ONLINE_UPDATES) || !defined(GN_NO_PCUPDATES) */


#if defined(GN_NO_ONLINE) || defined(GN_NO_ONLINE_UPDATES)
int
do_online_update()
{
	DEBUG(CDDBHELPER, DBGLEV_WARNING, "Online updates not supported\n");
	return 0;
}
#else
int
do_online_update()
{
	gn_error_t			error = 0;
	gn_transporter_t*	transporter = 0;
	gn_registrar_t*		registrar = 0;

	transporter = gn_create_internet_transporter(gn_upd_get_server_url());

	if (transporter == GN_NULL)
		return UPDERR_NoMemory;

	registrar = gn_create_online_registrar(transporter);

	if (registrar == GN_NULL) {
		(transporter->destroy)(transporter);
		return UPDERR_NoMemory;
	}

	DEBUG(CDDBHELPER, DBGLEV_INFO, "Doing online update...\n");

	error = do_remote_update(registrar, transporter);

	/* Please read the comment in process_remote_update_files() */
	/* above regarding cleaning up update files after a crash.  */

	(registrar->destroy)(registrar);
	(transporter->destroy)(transporter);

	return error;
}
#endif /* #if	defined(GN_NO_ONLINE) || defined(GN_NO_ONLINE_UPDATES) */


#if defined(GN_NO_PCUPDATES)
do_pc_update()
{
	DEBUG(CDDBHELPER, DBGLEV_WARNING, "PC updates not supported\n");
	return 0;
}
#else /* #if defined(GN_NO_PCUPDATES) */
int
do_pc_update()
{
	gn_error_t			error = 0;
	gn_transporter_t*	transporter = 0;
	gn_registrar_t*		registrar = 0;

	transporter = gn_create_pc_transporter("localhost", 8008);

	if (transporter == GN_NULL)
		return UPDERR_NoMemory;

	registrar = gn_create_pc_registrar();

	if (registrar == GN_NULL) {
		(transporter->destroy)(transporter);
		return UPDERR_NoMemory;
	}

	DEBUG(CDDBHELPER, DBGLEV_INFO, "Doing PC update...\n(PC Connectivity Application should be running on this computer)\n");

	error = do_remote_update(registrar, transporter);

	/* Please read the comment in process_remote_update_files() */
	/* above regarding cleaning up update files after a crash.  */

	(registrar->destroy)(registrar);
	(transporter->destroy)(transporter);

	return error;
}
#endif /* #if defined(GN_NO_PCUPDATES) */

#endif /* #if	!defined(GN_NO_UPDATES) */
