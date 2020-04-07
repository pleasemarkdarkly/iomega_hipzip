/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * updates.h - Declaration of functions called to process update commands.
 */

#ifndef _UPDATES_H_
#define _UPDATES_H_

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

int do_online_update();

int do_pc_update();

void do_local_update(const char* filename);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef _UPDATES_H_ */

