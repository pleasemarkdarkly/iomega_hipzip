/*
* Copyright (c) 2001 Gracenote.
*
* This software may not be used in any way or distributed without
* permission. All rights reserved.
*
* Some code herein may be covered by US and international patents.
*/

/*
* gn_log.c - General logging / tracing functionality.
*/


/*******************
 *	PORTING
 *******************

	File output:
		gnlog_open
			fopen
		gnlog_close
			fclose
			fflush
		gnlog_write
			fputs

	Formatted strings:
		gnlog_logf
			_vsnprintf

	Variable arguement lists:
		gnlog_logf
			va_start
			va_end

 *******************/


/*
* Dependencies
*/

#include <extras/cddb/gn_log.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_fs.h>
#include GN_STRING_H
#include GN_STDIO_H
#include GN_STDARG_H
#include GN_TIME_H


/*
 * Typedefs
 */

/* Maps log ids to string */
typedef struct gn_logid_desc
{
	gn_uint32_t		code;
	gn_cstr_t		string;
}
gn_logid_desc_t;


/*
 * Constants
 */

#define		GNLOG_BUFFER_SIZE					(5*1024)

#define		BUFFER_OVERFLOW_ERROR_MESSAGE		"!ERROR: LOG BUFFER TOO SMALL!\n"

#if 0
id[ERROR(0x0001)]\tfile[d:\cddb\cedev\devel\common\ecddb\edb_integ_mgr.c(1024)]\tpackage[Abstraction Layer(0x02)]\terror[Initialization failed(0x000C)]\ttime[yyyy-mm-dd hh:mm:ss.mmm]
id[0x0001]\tfile[edb_integ_mgr.c(1024)]\tpackage[0x02]\terror[0x000C]\ttime[yyyy-mm-dd hh:mm:ss.mmm]

logid = 0x000002 (FTRACE)	d:\cddb\cedev\devel\common\edb_integ_mgr.c(1024)	pkgid = 0x02 (Abstraction Layer)	yyyy-mm-dd hh:mm:ss.mmm
logid = 0x000002	edb_integ_mgr.c(1024)	pkgid = 0x02	yyyy-mm-dd hh:mm:ss.mmm
#endif

#ifdef PLATFORM_WIN32
#define		DIR_DEMARK							"\\"
#elif defined (PLATFORM_UNIX)
#define		DIR_DEMARK							"/"
#else
#define		DIR_DEMARK							"/"
#endif

#define		GNLOG_MAX_PKG_ID					(MAX_GNERR_PKG_ID - BASE_GNERR_PKG_ID + 1)

#define		GNLOG_DEFAULT_LOG_FILE				"ecddb.log"

#ifndef PLATFORM_WIN32
#define		_vsnprintf		vsnprintf
#endif

/*
 * Variables
 */

static gn_bool_t	logging_initialized = GN_FALSE;

static gn_char_t	logging_buffer[GNLOG_BUFFER_SIZE] = "";
static gn_char_t*	logging_buffer_curr_ptr = GN_NULL;
static gn_bool_t	logging_buffer_overflow = GN_FALSE;

static gn_handle_t* log_file = NULL;
static gn_char_t	log_file_name[GN_MAX_PATH] = GNLOG_DEFAULT_LOG_FILE;

static gn_uint32_t	enable_masks[GNLOG_MAX_PKG_ID] = {0};	/* one mask per package id */

static
gn_logid_desc_t logid_strings[] = {

	GNLOG_ERROR,		"ERROR",
	GNLOG_FTRACE,		"FTRACE",
	GNLOG_CTRACE,		"CTRACE",
	GNLOG_CTRACE2,		"CTRACE2",
	GNLOG_DTRACE,		"DTRACE",
	GNLOG_DTRACE2,		"DTRACE2",
	GNLOG_DTRACE3,		"DTRACE3",
	GNLOG_DTRACE4,		"DTRACE4",

	0,					GN_NULL

};


/*
 * Prototypes
 */

static gn_error_t
gnlog_open(void);

static void
gnlog_close(void);


/*
 * Implementations
 */

/* Logging functions called from the guts of the code. */

gn_bool_t
gnlog_filter(gn_uint16_t package_id, gn_uint32_t mask)
{
	if (((mask & GNLOG_MASK_OVERRIDE) & GNLOG_FORCE) != 0)
		/* force logging regardless of enable flags */
		return GN_TRUE;

	if (package_id > GNLOG_MAX_PKG_ID)
		/* no filtering for this id */
		return GN_TRUE;

	if ((enable_masks[(package_id-BASE_GNERR_PKG_ID)] & (mask & ~GNLOG_MASK_OVERRIDE)) == 0)
		return GN_FALSE;

	return GN_TRUE;
}


void
gnlog_header(gn_uint16_t package_id, gn_uint32_t mask, gn_uint32_t line, gn_str_t file)
{
	gn_char_t*	file_ptr = "";

	if (logging_initialized == GN_FALSE)
		return /*GENERR_NotInited*/;

	if (
		(
			((mask & GNLOG_MASK_OVERRIDE) | (enable_masks[(package_id-BASE_GNERR_PKG_ID)] & GNLOG_MASK_OVERRIDE))
			&
			GNLOG_VERBOSE
		)
		!=
			0
		)
	{
		/* verbose */
		if (file != NULL)
			file_ptr = file;

		gnlog_logf("logid = %0#8X (%s)\t%s(%ld)\tpkgid = %0#4X (%s)",mask,gnlog_getid_string(mask),
			file_ptr,line,package_id,gnerr_get_package_desc(package_id));
	}
	else
	{
	/* non-verbose */
		if (file != NULL)
		{
			file_ptr = strrchr(file,*DIR_DEMARK);
			if (file_ptr != NULL)
				file_ptr++;
			else
				file_ptr = file;
		}

		gnlog_logf("logid = %0#8X\t%s(%ld)\tpkgid = %0#4X",mask,file_ptr,line,package_id);
	}

	if (
		(
			((mask & GNLOG_MASK_OVERRIDE) | (enable_masks[(package_id-BASE_GNERR_PKG_ID)] & GNLOG_MASK_OVERRIDE))
			&
			GNLOG_NO_TIME_STAMP
		)
		==
			0
		)
	{
		/* print the current time */
		time_t		ltime;

		time(&ltime);

		/* note that ctime puts it's own \n at the end of the string */
		gnlog_logf("\t%s",ctime(&ltime));
	}
	else
	{
		/* now, print the newline */
		gnlog_logf("\n");
	}
}


void
gnlog_write(gn_size_t message_length)
{
	gn_size_t	n = 0;
	gn_char_t	buffer[256] = "";

	if (logging_initialized == GN_FALSE)
		return /*ERR_NotInited*/;
	if (log_file == NULL)
		return /*ERR_NotInited*/;

	if (strlen(logging_buffer))
	{
		n = gnfs_write(log_file,(void*)logging_buffer,strlen(logging_buffer));
		gnfs_commit(log_file);  /* so it shows up with tail -f right away */

		/* reset the buffer */
		*logging_buffer = '\0';
		logging_buffer_curr_ptr = logging_buffer;
		logging_buffer_overflow = GN_FALSE;
	}
}


gn_size_t
gnlog_logf(gn_cstr_t format, ...)
{
	gn_size_t	n = 0;
	va_list		ap = NULL;
	gn_char_t*	buffer_ptr = logging_buffer_curr_ptr;
	gn_size_t	buffer_left = 0;

	buffer_left = GNLOG_BUFFER_SIZE - (buffer_ptr - logging_buffer);

	if (logging_buffer_overflow == GN_TRUE)
		/* there's no more room, man */
		return 0;
	if (logging_initialized == GN_FALSE)
		/* not initialized */
		return 0;
	if (logging_buffer_curr_ptr == GN_NULL)
		/* not initialized */
		return 0;

	/* Try to print in the allocated space. */
	va_start(ap, format);
	n = _vsnprintf(buffer_ptr, buffer_left,format,ap);
	va_end(ap);

	/* If that worked, return the amount written. */
	if ((n != -1) && (n < buffer_left))
	{
		logging_buffer_curr_ptr += n;
		return n;
	}

	/* logging buffer is full */
	logging_buffer_overflow = GN_TRUE;

	/* Else we want to tack on the error message to the end of the buffer. */
	/* But first we have to get to the right location in the buffer */
	buffer_ptr += buffer_left - 1 - strlen(BUFFER_OVERFLOW_ERROR_MESSAGE);
	if (buffer_ptr != NULL)
		strcpy(buffer_ptr,BUFFER_OVERFLOW_ERROR_MESSAGE);

	return n;
}


gn_size_t
gnlog_dumpf(gn_cstr_t description, void* buffer, gn_size_t number_of_bytes)
{
	gn_size_t	n = 0;
	gn_size_t	i = 0;
	gn_size_t	k = 0;

	if ((description != GN_NULL) && (*description != 0))
		n += gnlog_logf("%s",description);

	for(i = 0; i < number_of_bytes; i++)
	{
		/* print the offset */
		if((i%16) == 0)
		{
			n += gnlog_logf("%.4X   ", i);
		}

		/* print the hex data */
		n += gnlog_logf("%.2X ", ((gn_uchar_t*)buffer)[i]);

		if(((i+1)%8 == 0) && ((i+1)%16 != 0))
		{
			n += gnlog_logf("- ");
		}

		/* print the ASCII data */
		if((i+1)%16 == 0)
		{
			n += gnlog_logf("  |");
			for(k = i-15; k < i+1; k++)
			{
				if(((gn_uchar_t*)buffer)[k] >= ' ' && ((gn_uchar_t*)buffer)[k] <= '~')
				{
					n += gnlog_logf("%c", ((gn_uchar_t*)buffer)[k]);
				}
				else
				{
					n += gnlog_logf(".");
				}
			}
			n += gnlog_logf("|\n");
		}
	}

	/* need to print the ascii for the remainder of the buffer here */
	if(i % 16)
	{
		k = i;
		while(k % 16)
		{
			n += gnlog_logf("   ");
			if(((k+1) % 8 == 0) && ((k+1) % 16 != 0))
			{
				n += gnlog_logf("- ");
			}
			k++;
		}

		n += gnlog_logf("  |");

		for(k = i - (i%16); k < i; k++)
		{
			if(((gn_uchar_t*)buffer)[k] >= ' ' && ((gn_uchar_t*)buffer)[k] <= '~')
			{
				n += gnlog_logf("%c", ((gn_uchar_t*)buffer)[k]);
			}
			else
			{
				n += gnlog_logf(".");
			}
		}
		n += gnlog_logf("|");
	}

	n += gnlog_logf("\n");

	return n;
}


/* Initialization */

gn_error_t
gnlog_init(gn_cstr_t override_log_file_name)
{
	gn_error_t	error = SUCCESS;

	if (logging_initialized == GN_TRUE)
		return GENERR_Busy;

	if ((log_file_name == NULL) || (*log_file_name == 0))
		return GENERR_InvalidArg;

	if ((override_log_file_name != NULL) && (*override_log_file_name != 0))
		strcpy(log_file_name,override_log_file_name);

	error = gnlog_open();
	if (error != SUCCESS)
		return error;

	logging_buffer_curr_ptr = logging_buffer;
	*logging_buffer = '\0';
	logging_buffer_overflow = GN_FALSE;

	gnlog_enable(GNLOG_PKG_ALL,GNLOG_IDS_ALL);

	logging_initialized = GN_TRUE;

	return SUCCESS;
}


void
gnlog_shutdown(void)
{
	if (logging_initialized == GN_FALSE)
		return;

	gnlog_close();

	logging_buffer_curr_ptr = GN_NULL;
	*logging_buffer = '\0';
	logging_buffer_overflow = GN_FALSE;

	logging_initialized = GN_FALSE;
}


/* Control functions called during initialization (or at other opportune times). */
/* Control settings are stored on a package-by-package basis. */

gn_error_t
gnlog_enable(gn_uint16_t package_id, gn_uint32_t category_mask)
{
	gn_size_t	i = 0;
	gn_size_t	array_size = GNLOG_MAX_PKG_ID;

	if (package_id == GNLOG_PKG_ALL)
	{
		for (i = 0; i < array_size; i++)
			enable_masks[i] |= category_mask;
	}
	else
		enable_masks[(package_id-BASE_GNERR_PKG_ID)] |= category_mask;

	return SUCCESS;
}


gn_error_t
gnlog_disable(gn_uint16_t package_id, gn_uint32_t category_mask)
{
	gn_size_t	i = 0;
	gn_size_t	array_size = GNLOG_MAX_PKG_ID;

	if (package_id == GNLOG_PKG_ALL)
	{
		for (i = 0; i < array_size; i++)
			enable_masks[i] &= ~category_mask;
	}
	else
		enable_masks[(package_id-BASE_GNERR_PKG_ID)] &= ~category_mask;

	return SUCCESS;
}


/* If output_file == "stdout", then output is to standard out. */
/* If output_file == "stderr", then output is to standard error. */
gn_error_t
gnlog_set_output(gn_cstr_t output_file)
{
	if ((output_file == GN_NULL) || (*output_file == 0))
		return GENERR_InvalidArg;

	gnlog_close();

	strcpy(log_file_name,output_file);

	return gnlog_open();
}


static gn_error_t
gnlog_open(void)
{
	if (!strcmp(log_file_name,"stdout"))
	{
		log_file = stdout;
		return SUCCESS;
	}
	if (!strcmp(log_file_name,"stderr"))
	{
		log_file = stderr;
		return SUCCESS;
	}

	/* last, let's open up the file */
	log_file = gnfs_open(log_file_name, FSMODE_WriteOnly);
	if (log_file == NULL)
		return GENERR_IOError;

	return SUCCESS;
}


static void
gnlog_close(void)
{
	if (log_file != NULL)
	{
		gnfs_commit(log_file);
		if ((log_file != stdout) && (log_file != stderr))
			/* make sure not to close the standard output streams */
			gnfs_close(log_file);
		log_file = NULL;
	}
}


gn_cstr_t
gnlog_getid_string(gn_uint32_t logid)
{
	gn_size_t	i = 0;

	for (i = 0; logid_strings[i].string != GN_NULL; i++)
	{
		if (logid_strings[i].code == (logid & ~GNLOG_MASK_OVERRIDE))
			return logid_strings[i].string;
	}

	return GN_NULL;
}

/* we need to be able to tell if they passed in a valid index,
 * so we return a bool to determine success/fail
 */
gn_bool_t
gnlog_get_pkg_mask(gn_uint32_t package_index, gn_uint32_t *mask)
{
	if((package_index < GNLOG_MAX_PKG_ID) && mask)
	{
		*mask = enable_masks[package_index];
		return GN_TRUE;
	}

	return GN_FALSE;
}


gn_bool_t
gnlog_is_verbose(gn_uint16_t package_id, gn_uint32_t mask)
{
	return (
		(
			((mask & GNLOG_MASK_OVERRIDE) | (enable_masks[(package_id-BASE_GNERR_PKG_ID)] & GNLOG_MASK_OVERRIDE))
			&
			GNLOG_VERBOSE
		)
		!=
			0
		);

}