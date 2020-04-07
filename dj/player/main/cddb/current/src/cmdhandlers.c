/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * cmdhandlers.c - implementation of commands called from command_table.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include "cmdhandlers.h"
#include "lookup.h"
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_tocinfo.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_utils.h>
#include "shell.h"
#include GN_STRING_H
#include <util/debug/debug.h>

FILE*	g_misfile = NULL;

/*
 * Local functions
 */

static char*
our_readln(gn_handle_t gn_file, FILE* file, gn_char_t* buff, gn_size_t size)
{
	if (gn_file != FS_INVALID_HANDLE)
		return gnfs_readln(gn_file, buff, size);
	else if (file != NULL)
		return fgets(buff, size, file);
	else
		return 0;
}

static int
our_close(gn_handle_t gn_file, FILE* file)
{
	if (gn_file != FS_INVALID_HANDLE)
		gnfs_close(gn_file);
	if (file)
		fclose(file);
	return 0;
}

/*
 * Implementations
 */

static char*
set_location(int location)
{
	if (IS_LOCAL(location))
		return "local database";
	else if (IS_ONLINE(location))
		return "online database";
	else if (IS_CACHE(location))
		return "local cache";
	else
		return "wherever";
}

int lookup_id(const char* id, int location)
{
	int     status;

	diag_printf("Looking up ID \"%s\" from %s:\n", id, set_location(location));

	status = lookup_id_info(id, location);
	if (status != 0)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("Error %X looking up id info\n", status);
		GNERR_LOG_ERR(status);

		error_message = gnerr_get_error_message(status);
		if(error_message != GN_NULL)
		{
			diag_printf("%s\n\n", error_message);
		}
	}

	return status;
}

int lookup_toc(const char* tocbuff, int location)
{
	int 	status;
	
	diag_printf("Looking up TOC \"%s\" from %s:\n", tocbuff, set_location(location));

	status = lookup_toc_info(tocbuff, location);
	if (status != 0)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("Error %X looking up toc info\n", status);
		GNERR_LOG_ERR(status);
		error_message = gnerr_get_error_message(status);
		if(error_message != GN_NULL)
		{
			diag_printf("%s\n\n", error_message);
		}
	}

	return status;
}

int lookup_toc_from_file(const char* file, int location)
{
	int             status = 0;
	gn_handle_t		infile;
	FILE*			stdfile = NULL;
	char            tocbuff[TOCBUFSIZE];
	char			artist_buff[512] = "";
	char			title_buff[512] = "";
	char			input_buff[32];
	int				num_fuzzies = 0;
	int				num_nomatch = 0;
	int				num_mismatch = 0;
	int				i = 0;

	diag_printf("Looking up TOC in file \"%s\" from %s:\n", file, set_location(location));
	
	infile = gnfs_open(file, FSMODE_ReadOnly);
	if (infile == -1)
	{
		stdfile = fopen(file, "r");
		if (stdfile == NULL)
		{
			diag_printf("cannot open input file [%s]\n", file);
			return (-1);
		}
	}
	
	/* Determine whether this is a sample toc file intended for lookup validation. */
	if (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) == NULL)
	{
		our_close(infile, stdfile);
		return (status);
	}

	if (strcmpi(tocbuff,"SAMPLE_TOCS"))
	{
		/* the file is a regular toc list */
		/* do the first lookup of regular toc file contents here */
		diag_printf("Looking up TOC: %s\n", tocbuff);
		status = lookup_toc_info(tocbuff, location);

		/* 
		 * Keep reading TOCs from the passed file, one per line, until
		 * we run out of input.
		 */
		while ((status == 0) && (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) != NULL))
		{
			if (*tocbuff == '#')
				/* this line is a comment */
				continue;
			if (gPaging == GN_TRUE)
			{
				input_buff[0] = 0;
				diag_printf("Press <RETURN> for next lookup, 'q' to stop lookups...\n");
				fgets(input_buff, sizeof(input_buff), stdin);
				if (input_buff[0] == 'q' || input_buff[0] == 'Q')
					break;
			}
			diag_printf("Looking up TOC: %s\n", tocbuff);
			status = lookup_toc_info(tocbuff, location);
		}
	}
	else
	{
		/* the file is a sample toc list, intended for validation */
		/* Read TOCs and validation info until we run out of input. */
		g_misfile = fopen("nomatch.toc", "w");
		for (i = 0; status >= 0; i++)
		{
			/* progress indicator for large files */
			if (i && ((i%100) == 0))
				diag_printf("\rProcessed %d TOCs", i);

			if (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) == NULL)
			{
				break;
			}
			if (our_readln(infile, stdfile, artist_buff, sizeof(artist_buff)) == NULL)
			{
				status = 1;
				break;
			}
			if (our_readln(infile, stdfile, title_buff, sizeof(title_buff)) == NULL)
			{
				status = 1;
				break;
			}
			status = lookup_toc_info_and_validate(tocbuff, location, artist_buff, title_buff, &num_fuzzies, &num_nomatch, &num_mismatch);
		}
		if (g_misfile)
		{
			fclose(g_misfile);
			g_misfile = NULL;
		}
		diag_printf("\rProcessed %d TOCs\n", i);
	}

	if (status != 0)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("Error (%X) looking up toc info.\n", status);
		GNERR_LOG_ERR(status);
		error_message = gnerr_get_error_message(status);
		if(error_message != GN_NULL)
		{
			diag_printf("%s\n\n", error_message);
		}
	}

	if (i > 0)
		diag_printf("\rProcessed %d TOCs\nFuzzies: %d, NoMatch: %d, MisMatch: %d\n\n", i, num_fuzzies, num_nomatch, num_mismatch);

	our_close(infile, stdfile);
    
    return (status);
}

int lookup_id_from_file(const char* file, int location)
{
	int             status = 0;
	gn_handle_t		infile;
	FILE*			stdfile = NULL;
	char            idbuff[TOCBUFSIZE];
	char			artist_buff[512] = "";
	char			title_buff[512] = "";
	char			input_buff[32];

	diag_printf("Looking up ID in file \"%s\" from %s:\n", file, set_location(location));
	
	infile = gnfs_open(file, FSMODE_ReadOnly);
	if (infile == -1)
	{
		stdfile = fopen(file, "r");
		if (stdfile == NULL)
		{
			diag_printf("cannot open input file [%s]\n", file);
			return (-1);
		}
	}
	
	/* Determine whether this is a sample id file intended for lookup validation. */
	if (our_readln(infile, stdfile, idbuff, sizeof(idbuff)) == NULL)
	{
		our_close(infile, stdfile);
		return (status);
	}

	if (strcmpi(idbuff, "SAMPLE_IDS"))
	{
		/* the file is a regular ID list */
		/* do the first lookup of regular ID file contents here */
		diag_printf("Looking up ID: %s\n", idbuff);
		status = lookup_id_info(idbuff, location);
		/* keep going if not found error */
		if (status == 0x80030002)
		{
			diag_printf("ID %s not found\n", idbuff);
			status = 0;
		}

		/* 
		 * Keep reading IDs from the passed file, one per line, until
		 * we run out of input.
		 */
		while ((status == 0) && (our_readln(infile, stdfile, idbuff, sizeof(idbuff)) != NULL))
		{
			if (gPaging == GN_TRUE)
			{
				input_buff[0] = 0;
				diag_printf("Press <RETURN> for next lookup, 'q' to stop lookups...\n");
				fgets(input_buff, sizeof(input_buff), stdin);
				if (input_buff[0] == 'q' || input_buff[0] == 'Q')
					break;
			}
			diag_printf("Looking up ID: %s\n", idbuff);
			status = lookup_id_info(idbuff, location);
			/* keep going if not found error */
			if (status == 0x80030002)
			{
				diag_printf("ID %s not found\n", idbuff);
				status = 0;
			}
		}
	}
	else
	{
		/* the file is a sample ID list, intended for validation */
		/* Read IDs and validation info until we run out of input. */
		while (status == 0)
		{
			if (our_readln(infile, stdfile, idbuff, sizeof(idbuff)) == NULL)
			{
				break;
			}
			if (our_readln(infile, stdfile, artist_buff, sizeof(artist_buff)) == NULL)
			{
				status = 1;
				break;
			}
			if (our_readln(infile, stdfile, title_buff, sizeof(title_buff)) == NULL)
			{
				status = 1;
				break;
			}
			status = lookup_id_info_and_validate(idbuff, location, artist_buff, title_buff);
		}
	}

	if (status != 0)
	{
		gn_str_t	error_message = GN_NULL;

		diag_printf("Error (%X) looking up id info.\n", status);
		GNERR_LOG_ERR(status);

		error_message = gnerr_get_error_message(status);
		if(error_message != GN_NULL)
		{
			diag_printf("%s\n\n", error_message);
		}
	}

	our_close(infile, stdfile);
    
	return (status);
}

#if defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE)
int add_toc_from_file(const char* file)
{
	diag_printf("Not implemented.\n");
	return 0;
}
#else
int add_toc_from_file(const char* file)
{
	int             status = 0;
	gn_handle_t		infile;
	FILE*			stdfile = NULL;
	char            tocbuff[TOCBUFSIZE];

	diag_printf("Adding TOCs from file \"%s\":\n", file);
	
	infile = gnfs_open(file, FSMODE_ReadOnly);
	if (infile == -1)
	{
		stdfile = fopen(file, "r");
		if (stdfile == NULL)
		{
			diag_printf("cannot open input file [%s]\n", file);
			return (-1);
		}
	}
	
	/* Determine whether this is a sample id file intended for lookup validation. */
	if (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) == NULL)
	{
		our_close(infile, stdfile);
		return status;
	}

	/* do the first lookup of regular toc file contents here */
	status = add_toc_2_cache(tocbuff);

	/* 
	 * Keep reading TOCs from the passed file, one per line, until
	 * we run out of input.
	 */
	while ((status == 0) && (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) != NULL))
	{
		if (*tocbuff == '#')
			/* this is a comment line; go on to the next line */
			continue;
		diag_printf("Adding TOC: %s\n", tocbuff);
		status = add_toc_2_cache(tocbuff);
	}

	if (status != 0)
	{
		diag_printf("Error (%X) adding TOC.\n", status);
	}

	our_close(infile, stdfile);

	return status;
}
#endif /* #if defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE) */

#if defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE)
int delete_toc_from_file(const char* file)
{
	diag_printf("Not implemented.\n");
	return 0;
}
#else
int delete_toc_from_file(const char* file)
{
	int             status = 0;
	gn_handle_t		infile;
	FILE*			stdfile = NULL;
	char            tocbuff[TOCBUFSIZE];

	diag_printf("Deleting TOC(s) from file \"%s\":\n", file);
	
	infile = gnfs_open(file, FSMODE_ReadOnly);
	if (infile == -1)
	{
		stdfile = fopen(file, "r");
		if (stdfile == NULL)
		{
			diag_printf("cannot open input file [%s]\n", file);
			return (-1);
		}
	}
	
	/* Determine whether this is a sample id file intended for lookup validation. */
	if (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) == NULL)
	{
		our_close(infile, stdfile);
		return status;
	}

	/* do the first lookup of regular toc file contents here */
	status = delete_toc_from_cache(tocbuff);
	/* 
	 * Keep reading TOCs from the passed file, one per line, until
	 * we run out of input.
	 */
	while ((status == 0) && (our_readln(infile, stdfile, tocbuff, sizeof(tocbuff)) != NULL))
	{
		if (*tocbuff == '#')
			/* comment line */
			continue;
		diag_printf("Deleting TOC: %s\n", tocbuff);
		status = delete_toc_from_cache(tocbuff);
	}

	if (status != 0)
	{
		diag_printf("Error (%X) deleting TOC.\n", status);
	}

	our_close(infile, stdfile);

	return status;
}
#endif /* #if defined(GN_NO_CACHE) && !defined(GN_MEM_CACHE) */
