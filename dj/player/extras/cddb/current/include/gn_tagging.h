/*
 * Copyright (c) 2000 - 2002 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_tagging.h -	Application interface for ID3v2 tagging support.
 */

#ifndef _GN_TAGGING_H
#define _GN_TAGGING_H


/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/microxml.h>


#ifdef __cplusplus
extern "C" {
#endif

#define		GN_ID3TAG_FRAME_ID		"UFID"
#define		GN_ID3TAG_MAX_ID		64

/*
 * Prototypes
 */


/* gntag_gen_tagid_from_xml
 *
 * Given an XML structure and a track number, returns a string which can
 * be used as the value for the UFID frame used to hold the Gracenote CDDB
 * file identifier.
 */
gn_error_t
gntag_gen_tagid_from_xml(XMLTagRef xml, gn_int32_t track, gn_str_t* tagid);


/* gntag_gen_tagid
 *
 * Given the ID and TAG attributes from a TID element, returns a string which can
 * be used as the value for the UFID frame used to hold the Gracenote CDDB
 * file identifier.
 */
gn_error_t
gntag_gen_tagid(gn_int32_t track, gn_cstr_t id, gn_cstr_t tag, gn_str_t* tagid);


/* gntag_ufil_ownerid
 *
 * Returns the string which goes in the "owner id" field of the UFID tags used
 * to hold Gracenote CDDB file identifiers.  This buffer should not be freed
 * by the caller.
 */
gn_cstr_t
gntag_ufid_ownerid(void);

#ifdef __cplusplus
}
#endif

#endif
