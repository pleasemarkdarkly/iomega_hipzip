/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 *
 * toc_util.c:	Utility routines for dealing with TOCs.
 */


#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include GN_FCNTL_H
#include <extras/cddb/toc_util.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_utils.h>

/*
 * read_toc_stdin()
 *
 * Description:
 *	Read a TOC from standard input.
 *
 * Arguments:
 *	offtab:		The table of TOC offsets.
 *	noff:		The number of offsets in the table (returned).
 *	errbuf:		Buffer to return error string.
 *	errsz:		Size of errbuf.
 *
 * Returns:
 *	Status from reading TOC from console.
 */
int
read_toc_stdin(gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz)
{
	gn_str_t	p;
	gn_char_t	buf[TOCBUFSIZE+1];

	toc->num_offsets = 0;

	/* Read the TOC from stdin. */
	while(fgets(buf, sizeof(buf), stdin) != NULL) {
		p = strtok(buf, " \t\r\n");

		while(p != NULL) {
			if(toc->num_offsets >= MAXOFFSETS) {
				(void)snprintf(errbuf, errsz,
				    "Too many offsets in TOC: %d, max %d\n",
				    (int)(toc->num_offsets + 1), (int)MAXOFFSETS);
				return RT_TOOMANY;
			}

			if(sscanf(p, "%d", &toc->offsets[toc->num_offsets]) != 1) {
				(void)snprintf(errbuf, errsz,
				    "Invalid input: %s\n", p);
				return RT_INVAL;
			}

			toc->num_offsets++;
			p = strtok(NULL, " \t\r\n");
		}
	}

	if(toc->num_offsets < MINOFFSETS)
		return RT_TOOFEW;

	return RT_OK;
}

/*
 * read_toc_file()
 *
 * Description:
 *	Read a TOC from the passed file.
 *
 * Arguments:
 *	handle::	An open file containing TOC offsets, one per line.
 *	offtab:		The table of TOC offsets.
 *	noff:		The number of offsets in the table (returned).
 *	errbuf:		Buffer to return error string.
 *	errsz:		Size of errbuf.
 *
 * Returns:
 *	Status from reading TOC from file.
 */
int
read_toc_file(gn_handle_t handle, gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz)
{
	gn_char_t		buf[TOCBUFSIZE+1];
    int             status;
    
	toc->num_offsets = 0;

	/* Keep reading TOCs from the passed file, one per line. */
	if (gnfs_readln(handle, buf, sizeof(buf)) == NULL) {
		/* file all done */
		return (RT_DONE);
	}
	
	/* Convert TOC into table of offsets. */
	status = convert_toc_string_to_array(buf, toc, errbuf, errsz);

    return (status);
}


/*
 * convert_toc_string_to_array()
 *
 * Description:
 *	Convert the passed TOC string into a table of offsets.
 *
 * Arguments:
 *	toc:		String with TOC.
 *	offtab:		The table of TOC offsets.
 *	noff:		The number of offsets in the table (returned).
 *	errbuf:		Buffer to return error string.
 *	errsz:		Size of errbuf.
 *
 * Returns:
 *	Status from reading TOC from file.
 */
int
convert_toc_string_to_array(const char * tocstr, gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz)
{
	gn_str_t	p;
	gn_char_t	buf[TOCBUFSIZE+1];

	toc->num_offsets = 0;

	strncpy(buf, tocstr, TOCBUFSIZE);
	buf[TOCBUFSIZE] = 0;

	p = strtok(buf, " \t\r\n");

	while(p != NULL) {
		if(toc->num_offsets >= MAXOFFSETS) {
			(void)snprintf(errbuf, errsz,
				"Too many offsets in TOC: %d, max %d\n",
				(int)(toc->num_offsets + 1), (int)MAXOFFSETS);
			return RT_TOOMANY;
		}

		if(sscanf(p, "%d", &toc->offsets[toc->num_offsets]) != 1) {
			(void)snprintf(errbuf, errsz,
				"Invalid input: %s\n", p);
			return RT_INVAL;
		}

		toc->num_offsets++;
		p = strtok(NULL, " \t\r\n");
	}

	if(toc->num_offsets < MINOFFSETS)
		return RT_TOOFEW;

	return RT_OK;
}


