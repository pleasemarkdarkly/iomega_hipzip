/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * cmdhandlers.h - Declaration of functions called to process commands.
 */

#ifndef _CMDHANDLERS_H_
#define _CMDHANDLERS_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Prototypes.
 */

/* Lookup helpers */
int lookup_id(const char* id, int location);
int lookup_id_from_file(const char* file, int location);
int lookup_toc(const char* toc, int location);
int lookup_toc_from_file(const char* file, int location);
int add_toc_from_file(const char* file);
int delete_toc_from_file(const char* file);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _CMDHANDLERS_H_ */

