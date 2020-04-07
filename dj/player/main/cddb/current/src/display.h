/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * display.h - Declarations for display functions used by lookup routines.
 */


#ifndef _DISPLAY_H_
#define _DISPLAY_H_

/*
 * Dependencies
 */

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/gn_translator.h>
#include <extras/cddb/gn_lookup.h>

/*
 * Constants
 */


/*
 * Macros
 */

/*
 * Structures and typedefs.
 */

/* callback proc to display the translated disc info */
typedef     int (* display_discinfo_proc)(gn_discinfo_t *info);


/*
 * Prototypes.
 */
/* display the short info & get selection */
gn_uint32_t
get_selection(gn_sdiscinfo_t** discs, gn_uint32_t count);

/* parse information from local fuzzy/multiple lookup to user */
int
resolve_local_fuzzy(gn_tlu_result_t** results, gn_uint32_t nresults, gn_tlu_result_t** selection);

/* parse information from online fuzzy/multiple lookup to user */
int
resolve_online_fuzzy(XMLTagRef xml, XMLTagRef* xml_choice);

/* print disc information to standard output */
int
display_discinfo(gn_discinfo_t* info);

/* display terse validation information */
int
display_validation_discinfo(gn_discinfo_t* info);

/* print limited disc information to standard output */
int
display_artist_title(void* info, gn_size_t info_size, XMLTagRef xml_info);


#endif  /* #ifndef _DISPLAY_H_ */
