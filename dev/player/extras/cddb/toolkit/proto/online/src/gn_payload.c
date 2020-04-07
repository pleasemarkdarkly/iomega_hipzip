/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_payload.c - code relating to the payload of a CDDBMSG element:
 *                REQUESTS, REQUESTs, RESULTS, and RESULTs.
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
 * Private Function Declarations
 */

static gn_error_t
check_for_hello_errors(XMLTagRef results);

static gn_error_t
create_hello_request(XMLTagRef* hello, gn_cstr_t id);

				
/*
 * Public Functions
 */

gn_error_t
create_request_element(gn_cstr_t cmd, gn_cstr_t type, gn_cstr_t opts, gn_cstr_t id,
                       gn_cstr_t format_id, gn_cstr_t result_format_id,
                       XMLTagRef contents, XMLTagRef content2, XMLTagRef* request)
{
	gn_error_t error = 0;

	if (cmd == 0 || type == 0 || id == 0 || request == 0) {
		return OPERR_InvalidArg;
	}

	*request = CreateXMLTagFromStr(STR_REQUEST, 0);

	if (*request == 0) {
		return OPERR_NoMemory;
	}

	if (contents != 0) {
		AddXMLSubTag(*request, contents);
	}

	if (content2 != 0) {
		AddXMLSubTag(*request, content2);
	}

	error = SetXMLTagAttrFromStr(*request, STR_COMMAND, cmd);

	if (error == kXMLNoError) {
		error = SetXMLTagAttrFromStr(*request, STR_TYPE, type);
	}

	if (error == kXMLNoError && opts != 0) {
		error = SetXMLTagAttrFromStr(*request, STR_OPTS, opts);
	}
	
	if (error == kXMLNoError) {
		error = SetXMLTagAttrFromStr(*request, STR_ID, id);
	}
	
	if (error == kXMLNoError && format_id != 0) {
		error = SetXMLTagAttrFromStr(*request, STR_FMT_ID, format_id);
	}
	
	if (error == kXMLNoError && result_format_id != 0) {
		error = SetXMLTagAttrFromStr(*request, STR_RSLTFMT_ID, result_format_id);
	}
	
	if (error != kXMLNoError) {
		SmartDisposeXMLTag(request);
	}

	return error;
}

gn_error_t
create_format_element(gn_cstr_t id, gn_cstr_t charset, gn_cstr_t encoding, XMLTagRef* format)
{
	XMLError error = kXMLNoError;

	if (id == 0 || charset == 0 || encoding == 0 || format == 0) {
		return 0;
	}

	*format = CreateXMLTagFromStr(STR_FORMAT, 0);

	if (*format == 0) {
		error = OPERR_NoMemory;
	}

	if (error == 0) {

		XMLTagRef charset_element = CreateXMLTagFromStr(STR_CHARSET, charset);

		if (charset_element != 0) {
			AddXMLSubTag(*format, charset_element);
			charset_element = 0;	/* we no longer own charset_element */
		} else {
			error = kXMLOutOfMemoryError;
		}
	}

	if (error == kXMLNoError) {

		XMLTagRef encoding_element = CreateXMLTagFromStr(STR_ENCODING, encoding);

		if (encoding_element != 0) {
			AddXMLSubTag(*format, encoding_element);
			encoding_element = 0;	/* we no longer own encoding_element */
		} else {
			error = kXMLOutOfMemoryError;
		}
	}

	if (error == kXMLNoError) {
		error = SetXMLTagAttrFromStr(*format, STR_ID, id);
	}

	if (error != kXMLNoError) {
		SmartDisposeXMLTag(format);
	}

	return error;
}

static gn_char_t	protocol_tag[] = "<PROTOCOL NAME=\"eCDDB\" VERSION=\"0.0.9\" ID=\"abcd\"><ENCODING ALG=\"none\"></ENCODING><ENCRYPTION ALG=\"none\"></ENCRYPTION><COMPRESSION ALG=\"none\"></COMPRESSION><CHECKSUM ALG=\"none\"></CHECKSUM></PROTOCOL>";


gn_error_t
create_protocol_element(XMLTagRef* protocol)
{
	gn_error_t error = 0;

	if (protocol == 0) {
		return OPERR_InvalidArg;
	}

	error = ParseStrToXMLTag(protocol_tag, protocol);
	
	return error;
}


gn_error_t
create_empty_requests_element(XMLTagRef* requests)
{
	if (requests == 0) {
		return OPERR_InvalidArg;
	}

	*requests = CreateXMLTagFromStr(STR_REQUESTS, 0);

	return (*requests == 0) ? OPERR_NoMemory : 0;
}

gn_error_t
create_basic_requests_element(gn_size_t max_results_size, XMLTagRef* requests)
{
	gn_error_t error = 0;
	XMLTagRef hello = 0;
	
	if (requests == 0) {
		return OPERR_InvalidArg;
	}

	error = create_empty_requests_element(requests);

	if (error == 0 && max_results_size > 0) {

		char max_results_size_str[32];

		sprintf(max_results_size_str, "%d", max_results_size);

		error = SetXMLTagAttrFromStr(*requests, STR_MAX_RESULTS_SIZE, max_results_size_str);
	}

	error = create_hello_request(&hello, STR_HELLO_REQ_ID);

	if (error == 0) {
		error = add_request_to_requests(hello, *requests);
	}

	return error;
}

gn_error_t
add_request_to_requests(XMLTagRef request, XMLTagRef requests)
{
	if (request == 0 || requests == 0)
		return OPERR_InvalidArg;

	AddXMLSubTag(requests, request);

	return 0;
}

gn_error_t
add_format_to_requests(XMLTagRef format, XMLTagRef requests)
{
	if (format == 0 || requests == 0)
		return OPERR_InvalidArg;

	AddXMLSubTag(requests, format);

	return 0;
}

gn_error_t
get_result_from_results(XMLTagRef results, XMLTagRef* result, gn_cstr_t id)
{
	gn_size_t result_count = 0;
	gn_size_t i = 0;

	if (results == 0 || result == 0 || id == 0) {
		return OPERR_InvalidArg;
	}

	*result = 0;

	if (!gn_str_eq(GetXMLTagName(results), STR_RESULTS)) {
		return OPERR_NotRESULTS;
	}

	result_count = GetXMLSubTagCount(results);

	for (i = 0; i < result_count; i++) {

		XMLTagRef tag = GetXMLSubTag(results, i);

		if ((tag != 0) && gn_str_eq(GetXMLTagName(tag), STR_RESULT)) {

			gn_cstr_t tag_id = GetXMLTagAttrFromStr(tag, STR_ID);

			if (gn_str_eq(tag_id, id)) {

				*result = tag;
				return 0;
			}
		}
	}

	return OPERR_RESULTNotFound;
}

gn_error_t
get_successful_result_from_results(XMLTagRef results, XMLTagRef* result, gn_cstr_t id)
{
	gn_error_t error = 0;

	if (results == 0 || result == 0 || id == 0) {
		return OPERR_InvalidArg;
	}

	error = get_result_from_results(results, result, id);

	if (error == 0) {
		error = get_result_error(*result);
	}

	return error;
}

gn_error_t
transmit_requests(XMLTagRef requests, gn_cstr_t server_url, XMLTagRef* results)
{
	gn_error_t error = 0;
	gn_cstr_t req_str = 0;
	gn_str_t res_buf = 0;
	gn_size_t res_len = 0;

	if (requests == 0 || results == 0) {
		return OPERR_InvalidArg;
	}

	*results = 0;

	/* transmit the requests and get a string back */
	error = proto_protocol_transmission(requests, server_url, results);

	/* check for hello errors */
	if (error == 0) {
		error = check_for_hello_errors(*results);
	}

	return error;
}

gn_error_t
get_result_error(XMLTagRef result)
{
	gn_error_t error = 0;
	gn_cstr_t name = 0;
	gn_cstr_t code = 0;

	if (result == 0) {
		return OPERR_InvalidArg;
	}

	name = GetXMLTagName(result);
	code = GetXMLTagAttrFromStr(result, STR_CODE);

	if (!gn_str_eq(name, STR_RESULT)) {
		error = OPERR_NotRESULT;
	}

	if (error == 0 && code == 0) {
		error = OPERR_MissingRESULTCODE;
	}

	if (error == 0) {
		error = parse_buf_to_int32(code, strlen(code));
	}

	return error;
}


/*
 * Private Functions
 */

static gn_error_t
check_for_hello_errors(XMLTagRef results)
{
	/* TBD REC: Re-evaluate the assumptions made here. */

	gn_error_t error = 0;
	XMLTagRef hello = 0;

	if (results == 0) {
		return OPERR_InvalidArg;
	}

	error = get_result_from_results(results, &hello, STR_HELLO_REQ_ID);

	/* if there is no hello result, assume it's OK */
	if (error == OPERR_RESULTNotFound) {
		return 0;
	}

	/* get the result code from the result */
	if (error == 0) {
		error = get_result_error(hello);
	}

	/* if there is no result code, assume that it's OK */
	if (error == OPERR_MissingRESULTCODE) {
		error = 0;
	}

	return error;
}

#if	defined(PLATFORM_WINDOWS)
#define CUR_CLASS "Windows Prototype Kit"
#define CUR_ID "123456789A"
#define CUR_IDTAG "FE34AE9EB3763B18DF040824F24B421F"
#define CUR_DEVID "10000000000027162227"
#define CUR_DEVIDTAG "0E3E7EBD6F8E4E47E26552E4B60C143E"
#elif defined(PLATFORM_LINUX)
#define CUR_CLASS "Linux Prototype Kit"
#define CUR_ID "123456789A"
#define CUR_IDTAG "F089CA37B8C1ED69C97DCCCA32CA1A1E"
#define CUR_DEVID "10000000000027181553"
#define CUR_DEVIDTAG "1A0994D3081540EE90CF99B7161EB342"
#elif defined(PLATFORM_WINDOWS_MUZE)
#define CUR_CLASS "Windows Muze Proto Kit"
#define CUR_ID "123456789A"
#define CUR_IDTAG "6D61722A37E98F27B05455E1B3965521"
#define CUR_DEVID "10000000000027168652"
#define CUR_DEVIDTAG "A1D4053556C73961FAB70FAA9B15037D"
#elif defined(PLATFORM_DHARMA)
#define CUR_CLASS "Dharma Proto Kit"
#define CUR_ID "123456789A"
#define CUR_IDTAG "FE34AE9EB3763B18DF040824F24B421F"
#define CUR_DEVID "10000000000027162227"
#define CUR_DEVIDTAG "0E3E7EBD6F8E4E47E26552E4B60C143E"

#define	ECDDB_VERSION	"3.0.1.3"
#define	DEVICE_CLASS	"Windows Prototype Kit"
#define	ID				"123456789A"
#define	SW_VERS			"1"
#define	HW_VERS			"1"

#endif

static gn_char_t hello_req[] = "<REQUEST CMD=\"HELLO\" TYPE=\"DEVICE\" ID=\"dh-1\">"
                                 "<DEVID>" CUR_DEVID "</DEVID>"
                                 "<DEVIDTAG>" CUR_DEVIDTAG "</DEVIDTAG>"
                                 "<DEVICE>"
                                   "<ID>" ID "</ID>"
                                   "<IDTAG>" CUR_IDTAG "</IDTAG>"
                                   "<CLASS>" DEVICE_CLASS "</CLASS>"
                                   "<VER>"
                                     "<SW>" SW_VERS "</SW>"
                                     "<HW>" HW_VERS "</HW>"
                                     "<ECDDB>" ECDDB_VERSION "</ECDDB>"
                                   "</VER>"
                                 "</DEVICE>"
                               "</REQUEST>";


gn_error_t
create_hello_request(XMLTagRef* hello, gn_cstr_t id)
{
	gn_error_t error = 0;

	if (hello == 0) {
		return OPERR_InvalidArg;
	}

	error = ParseStrToXMLTag(hello_req, hello);
	
	return error;
}

