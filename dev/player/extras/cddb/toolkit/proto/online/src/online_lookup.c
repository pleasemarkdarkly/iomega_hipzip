/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * online_lookup.c - implementation of the online TOC Lookup module.
 */


/*
 * Dependencies.
 */
#include <extras/cddb/gn_platform.h>
#include GN_STDIO_H

#include <extras/cddb/gn_lookup.h>
#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_dyn_buf.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/toolkit/proto/online/proto_online.h>
#include GN_STRING_H
#include GN_STDIO_H
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_configmgr.h>

/*
 * Constants.
 */
static const char* STR_lookup_id = "tl-1";
static const char* STR_format_id = "fmt-1";
static const char* STR_LOOKUP    = "LOOKUP";
static const char* STR_TOC       = "TOC";
static const char* STR_DISCID	 = "DISCID";
static const char* STR_MUID		 = "MUID";
static const char* STR_MEDIAID	 = "MEDIAID";
static const char* STR_ALB       = "ALB";
static const char* STR_ALBS      = "ALBS";
static const char* STR_first     = "first";
static const char* STR_list      = "list";
static const char* STR_NONE      = "NONE";
static const char* STR_EXACT     = "EXACT";
static const char* STR_MULTIPLE  = "MULTIPLE";

/*
 * Local functions.
 */

static gn_error_t
compose_toc_lookup_requests(gn_toc_info_t* toc, XMLTagRef* requests, gn_tlu_options_t lu_options,
                            gn_size_t max_results_size, gn_cstr_t id);

static gn_error_t
create_toc_lookup_request_element(gn_toc_info_t* toc, gn_cstr_t id, gn_tlu_options_t lu_options,
                                  gn_cstr_t result_format_id, XMLTagRef* request);

static gn_error_t
compose_toc_id_lookup_requests(gn_cstr_t toc_id, gn_cstr_t media_id, XMLTagRef* requests, gn_cstr_t id);

static gn_error_t
create_toc_id_lookup_request_element(gn_cstr_t toc_id, gn_cstr_t media_id, gn_cstr_t id,
									 gn_cstr_t result_format_id, XMLTagRef* request);

static gn_error_t
convert_toc_to_string(gn_toc_info_t* toc, gn_str_t* toc_str);

static gn_error_t
extract_lookup_result_from_results(XMLTagRef results, gn_cstr_t id,
                                   XMLTagRef* lookup_result, gn_tlu_match_t* match_type);

static gn_error_t
create_result_format(XMLTagRef* format, gn_cstr_t id);

/*
 * Variables.
 */
static gn_char_t server_url[URLSIZE] = "http://eCDDB.gracenote.com:8282";
static gn_char_t result_charset[DESCSIZE] = {0};
static gn_char_t result_encoding[DESCSIZE] = {0};


/*
 * Public functions.
 */

gn_error_t
gntlu_configure_online_lookup(gn_cstr_t new_server_url, gn_cstr_t new_result_charset,
                              gn_cstr_t new_result_encoding)
{
	gn_error_t error = 0;
	
	if (new_server_url && *new_server_url && strlen(new_server_url) >= sizeof(server_url))
		return OPERR_InvalidArg;

	if (new_result_charset && *new_result_charset && strlen(new_result_charset) >= sizeof(result_charset))
		return OPERR_InvalidArg;

	if (new_result_encoding && *new_result_encoding && strlen(new_result_encoding) >= sizeof(result_encoding))
		return OPERR_InvalidArg;

	strcpy(server_url, new_server_url);
	strcpy(result_charset, new_result_charset);
	strcpy(result_encoding, new_result_encoding);

	return error;
	}

/* Set callback function for online lookups. */
gn_tlu_error_t
gntlu_set_lookup_cb(gncomm_callback_t callback, gn_uint32_t user_data)
{
	gn_error_t error = 0;
	install_callback(callback, user_data);
	return error;
}

/* Cancel active online lookup. */
gn_tlu_error_t
gntlu_cancel_lookup(void)
{
	return cancel_online_transaction();
}


gn_tlu_error_t
gntlu_lookup_online(gn_toc_info_t* toc, XMLTagRef* xml, gn_tlu_match_t* match_type)
{
	return gntlu_lookup_online_ex(toc, TLOPT_AutoFuzzy, 0, xml, match_type);
}

gn_error_t
gntlu_lookup_online_ex(gn_toc_info_t *toc, gn_tlu_options_t lu_options,
                       gn_size_t max_results_size, XMLTagRef *xml, gn_tlu_match_t* match_type)
{
	XMLTagRef requests = 0;
	gn_error_t error = 0;
	XMLTagRef results = 0;

	if (toc == 0 || xml == 0 || match_type == 0) {
		return TLERR_InvalidArg;
	}

	/* compose XML query */
	if (error == 0) {
		error = compose_toc_lookup_requests(toc, &requests, lu_options, max_results_size, STR_lookup_id);
	}

	/* transmit query to the server and get result XML */
	if (error == 0) {
		error = transmit_requests(requests, server_url, &results);
	}

	/* we no longer need the REQUESTS */
	SmartDisposeXMLTag(&requests);

	/* fetch lookup results from XML */
	if (error == 0) {
		error = extract_lookup_result_from_results(results, STR_lookup_id, xml, match_type);
	}

	SmartDisposeXMLTag(&results);

	return error;
}

/* Look up a disc using the toc_id/media_id online. These values are returned    */
/* from a TOC lookup with fuzzy results (MUID/MEDIAID tags of the DISC).         */
gn_tlu_error_t
gntlu_lookup_id_online(gn_cstr_t toc_id, gn_cstr_t media_id, XMLTagRef* xml)
{
	XMLTagRef		requests = 0;
	gn_error_t		error = 0;
	XMLTagRef		results = 0;
	gn_tlu_match_t	dummy;

	if (toc_id == 0 || xml == 0 || media_id == 0 || *toc_id == 0 || *media_id == 0) {
		return TLERR_InvalidArg;
	}

	/* compose XML query */
	if (error == 0) {
		error = compose_toc_id_lookup_requests(toc_id, media_id, &requests, STR_lookup_id);
	}

	/* transmit query to the server and get result XML */
	if (error == 0) {
		error = transmit_requests(requests, server_url, &results);
	}

	/* we no longer need the REQUESTS */
	SmartDisposeXMLTag(&requests);

	/* parse results string into XML */
	if (error == 0) {
		error = extract_lookup_result_from_results(results, STR_lookup_id, xml, &dummy);
	}

	SmartDisposeXMLTag(&results);

	return error;
}



/*
 * Private functions.
 */

static gn_error_t
compose_toc_lookup_requests(gn_toc_info_t* toc, XMLTagRef* requests, gn_tlu_options_t lu_options,
                            gn_size_t max_results_size, gn_cstr_t id)
{
	XMLTagRef lookup_request = 0;
	gn_error_t error = 0;

	if (toc == 0 || requests == 0 || id == 0) {
		return TLERR_InvalidArg;
	}

	error = create_basic_requests_element(max_results_size, requests);

	/* now create the TOC lookup request */
	if (error == 0) {

		error = create_toc_lookup_request_element(toc, id, lu_options, 0, &lookup_request);
	}

	/* now that the lookup REQUEST is fully formed, add it to the REQUESTS */
	if (error == 0) {

		error = add_request_to_requests(lookup_request, *requests);

		if (error == 0) {
			lookup_request = 0;	/* we no longer own lookup_requests */
		}
	}

	/* clean up the REQUEST elements, if necessary */
	SmartDisposeXMLTag(&lookup_request);

	if (error != 0) {
		SmartDisposeXMLTag(requests);
	}

	return error;
}

static gn_error_t
create_toc_lookup_request_element(gn_toc_info_t* toc, gn_cstr_t id, gn_tlu_options_t lu_options,
                                  gn_cstr_t result_format_id, XMLTagRef* request)
{
	gn_error_t error = 0;
	XMLTagRef toc_tag = 0;
	gn_str_t toc_str = 0;

	if (toc == 0 || id == 0 || request == 0) {
		return OPERR_InvalidArg;
	}

	error = convert_toc_to_string(toc, &toc_str);

	if (error == 0) {

		toc_tag = CreateXMLTagFromStr(STR_TOC, toc_str);
		if (toc_tag == 0) {
			error = OPERR_NoMemory;
		}
	}

	/* free the string since it was copied into toc_tag */
	if (toc_str != 0) {
		gnmem_free(toc_str);
		toc_str = 0;
	}

	if (error == 0) {

		gn_cstr_t opts = (lu_options == TLOPT_AutoFuzzy) ? STR_first : STR_list;

		/* TBD REC: With the logic above, the fuzzy-match resolution in an */
		/* AutoFuzzy lookup is done by the server. This is different than  */
		/* a local lookup and could cause problems. */

		error = create_request_element(STR_LOOKUP, STR_TOC, opts, id, 0, result_format_id, toc_tag, 0, request);

		if (error == 0) {
			toc_tag = 0;	/* we no longer own toc_tag */
		}
	}

	/* clean up toc_tag, if necessary */
	SmartDisposeXMLTag(&toc_tag);

	return error;
}


static gn_error_t
compose_toc_id_lookup_requests(gn_cstr_t toc_id, gn_cstr_t media_id, XMLTagRef* requests, gn_cstr_t id)
{
	XMLTagRef	lookup_request = 0;
	gn_error_t	error = 0;

	if (toc_id == 0 || media_id == 0 || requests == 0 || id == 0) {
		return TLERR_InvalidArg;
	}

	error = create_basic_requests_element(0, requests);

	/* now create the TOC lookup request */
	if (error == 0) {

		error = create_toc_id_lookup_request_element(toc_id, media_id, id, 0, &lookup_request);
	}

	/* now that the lookup REQUEST is fully formed, add it to the REQUESTS */
	if (error == 0) {

		error = add_request_to_requests(lookup_request, *requests);

		if (error == 0) {
			lookup_request = 0;	/* we no longer own lookup_requests */
		}
	}

	/* clean up the REQUEST elements, if necessary */
	SmartDisposeXMLTag(&lookup_request);

	if (error != 0) {
		SmartDisposeXMLTag(requests);
	}

	return error;
}

static gn_error_t
create_toc_id_lookup_request_element(gn_cstr_t toc_id, gn_cstr_t media_id, gn_cstr_t id,
									 gn_cstr_t result_format_id, XMLTagRef* request)
{
	gn_error_t	error = 0;
	XMLTagRef	toc_id_tag = 0;
	XMLTagRef	media_id_tag = 0;
	XMLTagRef	disc_id_tag = 0;

	if (toc_id == 0 ||media_id == 0 || id == 0 || request == 0) {
		return OPERR_InvalidArg;
	}

	disc_id_tag = CreateXMLTagFromStr(STR_DISCID, 0);
	if (disc_id_tag == 0) {
		return OPERR_NoMemory;
	}
	toc_id_tag = CreateXMLTagFromStr(STR_MUID, toc_id);
	if (toc_id_tag == 0) {
		SmartDisposeXMLTag(&disc_id_tag);
		return OPERR_NoMemory;
	}

	media_id_tag = CreateXMLTagFromStr(STR_MEDIAID, media_id);

	if (media_id_tag == 0) {
		SmartDisposeXMLTag(&disc_id_tag);
		SmartDisposeXMLTag(&toc_id_tag);
		return OPERR_NoMemory;
	}

	AddXMLSubTag(disc_id_tag, media_id_tag);
	AddXMLSubTag(disc_id_tag, toc_id_tag);
	toc_id_tag = 0;	/* we no longer own toc_id_tag */
	media_id_tag = 0;	/* we no longer own media_id_tag */

	error = create_request_element(STR_LOOKUP, STR_ID, 0, id, 0, result_format_id, disc_id_tag, 0, request);
	if (error == 0) {
			disc_id_tag = 0;	/* we no longer own media_id_tag */
	}

	/* clean up disc_id_tag, if necessary */
	SmartDisposeXMLTag(&disc_id_tag);

	return error;
}

static gn_error_t
convert_toc_to_string(gn_toc_info_t* toc, gn_str_t* toc_str)
{
	gn_dyn_buf_error_t result = DYN_BUF_NO_ERROR;
	gn_dyn_buf_ref_t toc_str_buf = 0;
	gn_char_t null = '\0';
	gn_size_t i = 0;

	if (toc == 0 || toc_str == 0) {
		return OPERR_InvalidArg;
	}

	*toc_str = 0;

	toc_str_buf = gn_dyn_buf_create(64, 32);

	if (toc_str_buf == 0) {
		return OPERR_NoMemory;
	}

	for (i = 0; result == DYN_BUF_NO_ERROR && i < toc->num_offsets; i++) {
		
		gn_char_t str[16];

		sprintf(str, "%d", (int)toc->offsets[i]);

		result = gn_dyn_buf_append_str(toc_str_buf, str);

		if (result == DYN_BUF_NO_ERROR && i < toc->num_offsets - 1) {
			
			gn_char_t space = ' ';

			result = gn_dyn_buf_append_buf(toc_str_buf, &space, sizeof(space));
		}
	}

	if (result == DYN_BUF_NO_ERROR) {
		result = gn_dyn_buf_append_buf(toc_str_buf, &null, sizeof(null));
	}

	if (result == DYN_BUF_NO_ERROR) {
		*toc_str = gn_dyn_buf_dispose_and_return_buffer(toc_str_buf);
		toc_str_buf = 0;
	}
	
	if (toc_str_buf != 0) {
		gn_dyn_buf_dispose(toc_str_buf);
	}

	return result;
}

static gn_error_t
extract_lookup_result_from_results(XMLTagRef results, gn_cstr_t id,
                                   XMLTagRef* lookup_result, gn_tlu_match_t* match_type)
{
	gn_error_t error = 0;
	XMLTagRef result = 0;

	if (results == 0 || id == 0 || lookup_result == 0 || match_type == 0) {
		return TLERR_InvalidArg;
	}

	*lookup_result = 0;
	*match_type = TLMATCH_None;

	error = get_successful_result_from_results(results, &result, id);

	if (error == 0) {

		gn_cstr_t cmd = GetXMLTagAttrFromStr(result, STR_COMMAND);
		gn_cstr_t type = GetXMLTagAttrFromStr(result, STR_TYPE);

		if (!gn_str_eq(cmd, STR_LOOKUP)) {
			error = TLERR_BadResultFormat;
		}

		if (error == 0) {

			if (gn_str_eq(type, STR_EXACT)) {

				XMLTagRef alb = GetXMLSubTagFromStr(result, STR_ALB);

				if (alb != 0) {

					RemoveXMLSubTag(result, alb, 0);

					*lookup_result = alb;
					*match_type = TLMATCH_Exact;

				} else {
					error = TLERR_BadResultFormat;
				}

			} else if (gn_str_eq(type, STR_MULTIPLE)) {

				XMLTagRef albs = GetXMLSubTagFromStr(result, STR_ALBS);

				if (albs != 0) {

					RemoveXMLSubTag(result, albs, 0);

					*lookup_result = albs;
					*match_type = TLMATCH_Fuzzy;

				} else {
					error = TLERR_BadResultFormat;
				}

			} else if (gn_str_eq(type, STR_NONE)) {

					*lookup_result = 0;
					*match_type = TLMATCH_None;

			} else {

				error = TLERR_BadResultFormat;

			}
		}
	}

	return error;
}

