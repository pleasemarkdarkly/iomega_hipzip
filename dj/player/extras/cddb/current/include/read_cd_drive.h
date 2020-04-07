/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * read_cd_drive.h - Header file for routines reading TOC information.
 */


#ifndef	_READ_CD_DRIVE_H_
#define	_READ_CD_DRIVE_H_

#ifdef  __cplusplus
extern "C"
{
#endif

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>


/*
 * Prototypes.
 */

/* read TOC into a buffer, as text */
int     read_toc_from_default_drive(char* toc_buffer, int toc_buffer_size);
int     read_toc_from_drive(const char * drive_spec, char* toc_buffer, 
                            int toc_buffer_size);

#ifdef  __cplusplus
}
#endif

#endif  /* #ifndef _READ_CD_DRIVE_H_ */

