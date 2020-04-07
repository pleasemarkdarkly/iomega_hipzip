/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * toc_util.h - handy stuff for dealing with TOCs.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_tocinfo.h>

/*
 * Constants.
 */

/* Return values for "read toc" functions. */
#define RT_OK			0
#define	RT_DONE			1	/* no more TOC in file */
#define RT_TOOMANY		2
#define RT_TOOFEW		3
#define RT_INVAL		4
#define	RT_NOTFOUND		5

#ifdef __cplusplus
extern "C"{
#endif 

/*
 * Prototypes.
 */

/* read toc string from standard input and parse into offset table */
int
read_toc_stdin(gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz);

/* read toc string from passed file and parse into offset table */
int
read_toc_file(gn_handle_t handle, gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz);

/* Convert the passed TOC string into a table of offsets. */
int
convert_toc_string_to_array(const char * tocstr, gn_toc_info_t *toc, gn_str_t errbuf, gn_size_t errsz);

#ifdef __cplusplus
}
#endif 
