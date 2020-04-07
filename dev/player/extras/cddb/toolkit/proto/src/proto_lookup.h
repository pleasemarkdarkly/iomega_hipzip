/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * proto_lookup.h - Interface definition for use with the Prototype
 *					"TOC Lookup" module.
 */


#ifndef _PROTO_LOOKUP_H_
#define _PROTO_LOOKUP_H_

/*
 * Dependencies.
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_tocinfo.h>


#ifdef __cplusplus
extern "C"{
#endif 


/*
 * Constants.
 */


/*
 * Enums.
 */

/*
 * Structures and typedefs.
 */

/* temporary structure to hold artist/title information for user selection */
typedef	struct tlu_choice {
	gn_char_t	artist[128];
	gn_char_t	title[128];
	void		*choice;
} gn_tlu_choice_t;


/* structure holding results from TOC or ID lookups */
struct tlu_result {
	gn_uint32_t			rank;				/* the ranking for sorting - 0 best */
	gn_toc_info_t		toc;				/* TOC information for this result */
	gn_uint32_t			toc_id;				/* the id for this TOC (for fuzzy match selection) */
	void				*info;				/* pointer to raw data for this result */
	gn_size_t			info_size;			/* size of raw data buffer */
	void				*reserved;			/* reserved portion of this result */
};


/*
 * Prototypes.
 */


#ifdef __cplusplus
}
#endif 

#endif /* _PROTO_LOOKUP_H_ */
