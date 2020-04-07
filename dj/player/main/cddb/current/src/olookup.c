/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * olookup.c:	Sample code demonstrating looking up disc information
 *				from a TOC using the online lookup API.
 */

/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include GN_STDLIB_H
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_lookup.h>
#include <extras/cddb/toc_util.h>
#include <extras/cddb/gn_memory.h>
#include "lookup.h"
#include "display.h"
#include "shell.h"
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_configmgr.h>
#include "branding.h"
#include <util/debug/debug.h>



/*
 * Variables
 */


/* Actually do the lookup of a TOC structure from the online database */
#if defined(GN_NO_ONLINE)
int
do_toc_lookup_online(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	diag_printf("Online lookups not supported.\n");
	return 0;
}
#else
int
do_toc_lookup_online(gn_toc_info_t* toc, display_discinfo_proc display_proc)
{
	gn_tlu_error_t		lookup_err;
	gn_xlt_error_t		xlate_err;
	int					our_err = 0;
	gn_discinfo_t*		disc_info = 0;
	XMLTagRef			xml = 0;
	XMLTagRef			xml_choice = 0;
	XMLTagRef			xml_full = 0;
	XMLTagRef			discid_tag = 0;
	XMLTagRef			muid_tag = 0;
	XMLTagRef			mediaid_tag = 0;
	gn_tlu_match_t		match_code = TLMATCH_None;
 	gn_lookup_t			type_lookup = AUTOFUZZY;
	gn_tlu_options_t	lu_options = TLOPT_AutoFuzzy;
  
	/* get configuration information to determine lookup options */
	type_lookup = get_lookup_type();
	if (!(type_lookup & AUTOFUZZY))
		lu_options = TLOPT_Default;

	/* branding display on lookup */
	brand_display_on_lookup();

	lookup_err = gntlu_lookup_online_ex(toc, lu_options, 0, &xml, &match_code);

	if (lookup_err == 0 && xml != 0) {

		if (match_code == TLMATCH_Fuzzy) {
			our_err = resolve_online_fuzzy(xml, &xml_choice);

			/* upon choice by user, get full disc info from online database */
			if (our_err == SUCCESS && xml_choice != NULL) {

				/* xml_choice should be a partial ALB tag which has a DISCID tag */
				/* the DISCID tag contains the MUID and MEDIAID we need to get the */
				/* full disc information for this TOC */
				if (strcmp(GetXMLTagName(xml_choice), GN_STR_ALBUM))
					our_err = kXMLInvalidParamError;

				if (our_err == SUCCESS) {
					discid_tag = GetXMLSubTagFromStr(xml_choice, GN_STR_DISCID);

					/* extract toc_id(MUID) and media_id(MEDIAID) from XML */
					if (discid_tag) {
						muid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MUID);
						mediaid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MEDIAID);
					}
					if (muid_tag == 0 || mediaid_tag == 0)
						our_err = kXMLInvalidParamError;
				}

				/* if still okay, get full disc information */
				if (our_err == SUCCESS) {

					/* branding display on lookup */
					brand_display_on_lookup();

					lookup_err = gntlu_lookup_id_online(GetXMLTagData(muid_tag), GetXMLTagData(mediaid_tag), &xml_full);
					if (lookup_err == SUCCESS && xml_full) {
						diag_printf("Successfully got full info online.\n");
					}
					our_err = lookup_err;
				}
			}
			else if (our_err == SUCCESS) {
				diag_printf("No selection chosen from online list.\n");
				DisposeXMLTag(xml);
				xml = NULL;
			}
		}
		/* if we did a fuzzy match, replace original XML */
		if (xml_full) {
			if (xml)
				DisposeXMLTag(xml);
			xml = xml_full;
			xml_full = NULL;
		}

		if (xml) {
			/* diagnostic output */
			if (gDumpRaw == GN_TRUE) {
				gn_str_t	raw_xml;

				raw_xml = RenderXMLTagToStrEx(xml, GN_FALSE, GN_TRUE, GN_TRUE);
				if (raw_xml) {
					diag_printf("Raw XML:\n");
					diag_printf("%s\n", raw_xml);
					diag_printf("End XML\n");
					gnmem_free(raw_xml);
				}
			}

			/* convert XML data to disc info structure */
			xlate_err = gnxlt_convert_xml_alloc(xml, &disc_info);
			
			if (xlate_err == 0) {
				/* call the display handler */
   				display_proc(disc_info);

				/* and release the C structure */
				if (disc_info != 0) {
					gnxlt_release(disc_info);
					gnmem_free((void*)disc_info);
					disc_info = 0;
				}
			}
			else {
				diag_printf("Error translating data (%X)\n", xlate_err);
			}

			/* and release the XML text */
			DisposeXMLTag(xml);

			our_err = xlate_err;
		}
	}
	else if (lookup_err) {
		our_err = lookup_err;
		/* call the display handler with NULL if the lookup was not successful */
   		display_proc(NULL);
	}
	else if (match_code == TLMATCH_None) {
		diag_printf("No match found for TOC online\n");
	}

	return our_err;
}

static gn_cstr_t
get_notification_name(int notification)
{
	switch (notification)
	{
	case CBNOT_Complete:
		return "CBNOT_Complete";
		break;
	case CBNOT_Status:
		return "CBNOT_Status";
		break;
	case CBNOT_Error:
		return "CBNOT_Error";
		break;
	case CBNOT_Timeout:
		return "CBNOT_Timeout";
		break;
	case CBNOT_HTTPRecvHdr:
		return "CBNOT_HTTPRecvHdr";
		break;
	case CBNOT_HTTPRecvBody:
		return "CBNOT_HTTPRecvBody";
		break;
	case CBNOT_HTTP_ContentLength:
		return "CBNOT_HTTP_ContentLength";
		break;
	case CBNOT_HTTPStatus:
		return "CBNOT_HTTPStatus";
		break;
	case CBNOT_HTTPSendHdr:
		return "CBNOT_HTTPSendHdr";
		break;
	case CBNOT_HTTPSendBody:
		return "CBNOT_HTTPSendBody";
		break;
	default:
		return "unknown";
		break;
	}
}

/* sample implementation of callback function used in online lookups */
gncomm_cbret_t
sampleCallback(gn_connection_t handle, int notification, gn_uint32_t p1, gn_uint32_t p2, gn_uint32_t user_data)
{
	int		test_cancel = 0;

	diag_printf("sampleCallback: notification=%d (%s), p1=%u, p2=%u\n", notification, get_notification_name(notification), p1, p2);

	if (test_cancel == 1)
		return CBRET_Cancel;
	else if (test_cancel == 2)
		gntlu_cancel_lookup();

	return CBRET_Continue;
}

#endif /* #if defined(GN_NO_ONLINE) */



#if defined(GN_NO_ONLINE)
int
do_id_lookup_online(gn_cstr_t toc_id, gn_cstr_t media_id, display_discinfo_proc display_proc)
{
	diag_printf("Online lookups not supported.\n");
	return 0;
}
#else
/* Actually do the lookup of IDs from the online database */
int
do_id_lookup_online(gn_cstr_t toc_id, gn_cstr_t media_id, display_discinfo_proc display_proc)
{
	gn_tlu_error_t	lookup_error;
	int				our_error = 0;
	XMLTagRef		xml = 0;

	/* branding display on lookup */
	brand_display_on_lookup();

	lookup_error = gntlu_lookup_id_online(toc_id, media_id, &xml);

	if (lookup_error == 0 && xml != 0) {

		gn_xlt_error_t	xlate_error;
		gn_discinfo_t*	info = 0;

	    /* convert XML data to disc info structure */
		xlate_error = gnxlt_convert_xml_alloc(xml, &info);
		
		if (xlate_error == 0) {
			/* call the display handler */
   			display_proc(info);

			/* and release the C structure */
			if (info != 0) {
				gnxlt_release(info);
				gnmem_free((void*)info);
				info = 0;
			}
		} else {
			diag_printf("Error translating data (%d)\n", xlate_error);
		}

		/* and release the XML text */
		DisposeXMLTag(xml);

		our_error = xlate_error;
	} else {
		our_error = lookup_error;
	}

	return our_error;
}

#endif /* #if defined(GN_NO_ONLINE) */

