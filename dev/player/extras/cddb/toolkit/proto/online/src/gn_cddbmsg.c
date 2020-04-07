/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_cddbmsg.c - code related to CDDBMSG elements
 */


/*
 * Dependencies
 */
#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include <extras/cddb/toolkit/proto/online/proto_online.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_utils.h>


/*
 * Local Compiler Flags
 */
#define PROTOCOL_DIAGNOSTICS 0


/*
 * Private Function Declarations
 */

static gn_error_t
create_status_element(gn_cstr_t status_str, gn_error_t status_code,
                      gn_cstr_t proto_id, XMLTagRef* status_element);

static gn_error_t
create_data_element(XMLTagRef requests_element, gn_cstr_t proto_id, XMLTagRef* data_element);
				
/*
 * Public Functions
 */

gn_error_t
create_cddbmsg_element(XMLTagRef protocol_element, XMLTagRef requests_element,
                       gn_cstr_t proto_id, gn_cstr_t status, XMLTagRef* cddbmsg)
{
	XMLTagRef	status_element = 0;
	XMLTagRef	data_element = 0;
	gn_error_t	error = 0;

	if (protocol_element == 0 || requests_element == 0 ||
	    proto_id == 0 || *proto_id == 0 || status == 0 || *status == 0 || cddbmsg == 0) {
		return OPERR_InvalidArg;
	}

	/* create an empty CDDBMSG element */
	*cddbmsg = CreateXMLTagFromStr(STR_CDDBMSG, 0);

	if (*cddbmsg == 0) {
		error = OPERR_NoMemory;
	}

	/* set the proto_id attribute of the PROTOCOL element */
	if (error == 0) {
		error = SetXMLTagAttrFromStr(protocol_element, STR_ID, proto_id);
	}
	
	/* add the PROTOCOL element to the CDDBMSG element */
	if (error == 0) {
		AddXMLSubTag(*cddbmsg, protocol_element);
	}

	/* create a STATUS element and add it to the CDDBMSG */
	if (error == 0) {

		error = create_status_element(status, 0, proto_id, &status_element);

		if (error == 0) {
			AddXMLSubTag(*cddbmsg, status_element);
		}
	}

	/* create a DATA element and add it to the CDDBMSG */
	if (error == 0) {

		error = create_data_element(requests_element, proto_id, &data_element);

		if (error == 0) {
			AddXMLSubTag(*cddbmsg, data_element);
		}
	}

	if (error != 0) {
		SmartDisposeXMLTag(cddbmsg);
	}

	return error;
}

gn_error_t
extract_data_from_cddbmsg(XMLTagRef cddbmsg, XMLTagRef* results_element)
{
	gn_error_t error = 0;
	XMLTagRef protocol_element = 0;
	XMLTagRef status_element = 0;
	XMLTagRef data_element = 0;
	gn_bool_t should_extract_data = 0;

	if (cddbmsg == 0 || results_element == 0) {
		return OPERR_InvalidArg;
	}

	error = unpack_cddbmsg(cddbmsg, &protocol_element, &status_element, &data_element);

	if (error == 0) {
		error = analyze_status(status_element, protocol_element, &should_extract_data);
	}

	if (error == 0 && should_extract_data) {

			XMLTagRef results = GetXMLSubTagFromStr(data_element, STR_RESULTS);

			if (results == 0) {
				error = OPERR_NoDataContents;
			}

			if (error == 0) {
				RemoveXMLSubTag(data_element, results, GN_FALSE);
				*results_element = results;
			}
	}

	return error;
}

gn_error_t
unpack_cddbmsg(XMLTagRef cddbmsg, XMLTagRef* protocol_element,
               XMLTagRef* status_element, XMLTagRef* data_element)
{
	if (cddbmsg == 0 || protocol_element == 0 || status_element == 0 || data_element == 0) {
		return OPERR_InvalidArg;
	}

	*protocol_element = 0;
	*status_element = 0;
	*data_element = 0;

	if (!gn_str_eq(GetXMLTagName(cddbmsg), STR_CDDBMSG)) {
		return OPERR_NotCDDBMSG;
	}

	*protocol_element = GetXMLSubTagFromStr(cddbmsg, STR_PROTOCOL);

	if (*protocol_element == 0) {
		return OPERR_NoPROTOCOLElement;
	}

	*status_element = GetXMLSubTagFromStr(cddbmsg, STR_STATUS);

	if (*status_element == 0) {
		return OPERR_NoSTATUSElement;
	}

	*data_element = GetXMLSubTagFromStr(cddbmsg, STR_DATA);

	if (*data_element == 0) {
		return OPERR_NoDATAElement;
	}

	return 0;
}

gn_error_t
analyze_status(XMLTagRef status_element, XMLTagRef protocol_element,
               gn_bool_t* should_extract_data)
{
	gn_error_t error = 0;
	gn_cstr_t code = 0;
	gn_error_t server_error = 0;

	if (status_element == 0 || protocol_element == 0) {
		return OPERR_InvalidArg;
	}

	code = GetXMLTagAttrFromStr(status_element, STR_CODE);

#if PROTOCOL_DIAGNOSTICS
	printf("STATUS CODE = %s, contents = %s\n", code, GetXMLTagData(status_element));
#endif

	if (code == 0) {
		return OPERR_MissingSTATUSCODE;
	}

	server_error = parse_buf_to_int32(code, strlen(code));

	if (server_error != 0) {

		/* we just map all other server errors to a generic value */
		error = OPERR_ServerError;

	}

	if (error == 0 && should_extract_data != 0) {
		*should_extract_data = 1;
	}

	return error;
}


/*
 * Private Functions
 */

static gn_error_t
create_status_element(gn_cstr_t status_str, gn_error_t status_code,
                      gn_cstr_t proto_id, XMLTagRef* status_element)
{
	gn_error_t error = 0;

	if (status_str == 0 || proto_id == 0 || status_element == 0) {
		return OPERR_InvalidArg;
	}

	*status_element = CreateXMLTagFromStr(STR_STATUS, status_str);

	if (*status_element == 0) {
		error = OPERR_NoMemory;
	}

	if (error == 0) {

		char code_str[20];

		sprintf(code_str, "0x%X", status_code);

		error = SetXMLTagAttrFromStr(*status_element, STR_CODE, code_str);
	}

	if (error == 0) {
		error = SetXMLTagAttrFromStr(*status_element, STR_PROTO_ID, proto_id);
	}

	return error;
}

static gn_error_t
create_data_element(XMLTagRef requests_element, gn_cstr_t proto_id, XMLTagRef* data_element)
{
	gn_error_t error = 0;

	if (requests_element == 0 || proto_id == 0 || data_element == 0) {
		return OPERR_InvalidArg;
	}

	*data_element = CreateXMLTagFromStr(STR_DATA, 0);

	if (*data_element == 0) {
		return  OPERR_NoMemory;
	}

	AddXMLSubTag(*data_element, requests_element);

	error = SetXMLTagAttrFromStr(*data_element, STR_PROTO_ID, proto_id);

	return error;
}
