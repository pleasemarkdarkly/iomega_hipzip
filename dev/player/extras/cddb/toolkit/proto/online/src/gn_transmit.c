/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_transmit.c - data transmission for the online protocol module.
 */

/*
 * Dependencies
 */
#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include GN_STDIO_H
#include <extras/cddb/toolkit/proto/online/proto_online.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_comm.h>
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/gn_fs.h>
#include <extras/cddb/gn_configmgr.h>

#include <util/debug/debug.h>

/*
 * Local Compiler Flags
 */
#define PROTOCOL_DIAGNOSTICS 1


/*
 * Externally Visible Variables
 */
const char* STR_REQUESTS       = "REQUESTS";
const char* STR_RESULTS        = "RESULTS";
const char* STR_RESULT         = "RESULT";
const char* STR_ID             = "ID";
const char* STR_IDTAG         = "IDTAG";
const char* STR_base64         = "base64";
const char* STR_none           = "none";
const char* STR_zip            = "zip";
const char* STR_eCDDB          = "eCDDB";
const char* STR_OK             = "OK";
const char* STR_COMPRESSION    = "COMPRESSION";
const char* STR_CHECKSUM       = "CHECKSUM";
const char* STR_ALG            = "ALG";
const char* STR_ENCRYPTION     = "ENCRYPTION";
const char* STR_ENCODING       = "ENCODING";
const char* STR_PROTOCOL       = "PROTOCOL";
const char* STR_NAME           = "NAME";
const char* STR_VERSION        = "VERSION";
const char* STR_CDDBMSG        = "CDDBMSG";
const char* STR_STATUS         = "STATUS";
const char* STR_DATA           = "DATA";
const char* STR_PROTO_ID       = "PROTO_ID";
const char* STR_CODE           = "CODE";
const char* STR_REQUEST        = "REQUEST";
const char* STR_COMMAND        = "CMD";
const char* STR_TYPE           = "TYPE";
const char* STR_OPTS           = "OPTS";
const char* STR_FORMAT         = "FORMAT";
const char* STR_CHARSET        = "CHARSET";
const char* STR_FMT_ID         = "FMT_ID";
const char* STR_RSLTFMT_ID     = "RSLTFMT_ID";
const char* STR_REG_REQ_ID     = "dr-1";
const char* STR_HELLO_REQ_ID   = "dh-1";
const char* STR_REGISTER       = "REG";
const char* STR_DEVICE         = "DEVICE";
const char* STR_DEVID          = "DEVID";
const char* STR_DEVID_TAG      = "DEVIDTAG";
const char* STR_HELLO          = "HELLO";
const char* STR_CLASS          = "CLASS";
const char* STR_VER            = "VER";
const char* STR_SW		       = "SW";
const char* STR_HW             = "HW";
const char* STR_ECDDB          = "ECDDB";
const char* STR_MAX_RESULTS_SIZE = "MAX_RESULTS_SIZE";
const char* STR_user_agent     = "eCDDB Online Protocol";	/* TBD REC: Consider embedding more version and/or identification information into this string */

/*
 * Private Variables
 */
static gn_char_t proxy_server[URLSIZE];
static gn_uint16_t proxy_port = 0;
static gncomm_callback_t our_callback = NULL;
static gn_uint32_t our_user_data = 0;
static gn_bool_t http_busy = GN_FALSE;

/*
 * Private Function Declarations
 */

static gn_error_t
transmit_cddbmsg(XMLTagRef cddbmsg_to_send, gn_cstr_t server_url, XMLTagRef* cddbmsg_received);

static gn_error_t
transmit_buffer(gn_cstr_t send_buf, gn_size_t send_len, gn_cstr_t server_url,
                gn_str_t* receive_buf, gn_size_t* receive_len);

				
/*
 * Public Functions
 */

gn_error_t
online_configure_proxy_info(gn_cstr_t new_server, gn_uint16_t new_port)
{
	if (new_server && *new_server && strlen(new_server) >= sizeof(proxy_server))
		return OPERR_InvalidArg;

	proxy_port = new_port;

	strcpy(proxy_server, new_server);
	return OPERR_NoError;
}

gn_error_t
install_callback(gncomm_callback_t callback, gn_uint32_t user_data)
{
	our_callback = callback;
	our_user_data = user_data;
	return OPERR_NoError;
}

gn_error_t
cancel_online_transaction(void)
{
	if (http_busy != GN_TRUE)
		return OPERR_NoTransaction;

	return gncomm_HTTP_cancel();
}


gn_error_t
proto_protocol_transmission(XMLTagRef requests, gn_cstr_t server_url, XMLTagRef *results)
{
	gn_error_t	error = 0;
	XMLTagRef	protocol_element = 0;
	XMLTagRef	cddbmsg_element = 0;
	XMLTagRef	cddbmsg_result = 0;

	if (requests == 0 || results == 0) {
		return OPERR_InvalidArg;
	}

	error = create_protocol_element(&protocol_element);
	if (error != 0)
		return error;

	error = create_cddbmsg_element(protocol_element, requests, "abcd", STR_OK, &cddbmsg_element);

	if (error == 0) {
		protocol_element = 0;

		error = transmit_cddbmsg(cddbmsg_element, server_url, &cddbmsg_result);
	}

	RemoveXMLSubTag(cddbmsg_element, requests, GN_FALSE);
		
	if (error == 0) {

		error = extract_data_from_cddbmsg(cddbmsg_result, results);

		SmartDisposeXMLTag(&cddbmsg_result);
	}
		
	return error;
}

gn_error_t
post_buffer(gn_cstr_t send_buf, gn_size_t send_len, gn_cstr_t server_url,
            gn_str_t* receive_buf, gn_size_t* receive_len)
{
	gncomm_HTTP_req_t http_req = { 0 };
	gn_error_t error = 0;

	if (send_buf == 0 || send_len == 0 || server_url == 0 || receive_buf == 0 || receive_len == 0) {
		return OPERR_InvalidArg;
	}

	gnmem_memset(&http_req, 0, sizeof(gncomm_HTTP_req_t));

	http_req.body = send_buf;
	http_req.body_size = send_len;
	http_req.method = HTTP_Post;
	http_req.other_headers = 0;
	http_req.proxy_port = proxy_port;
	http_req.proxy_server = (proxy_server != 0 && *proxy_server != 0) ? proxy_server : 0;
	http_req.user_agent = STR_user_agent;

#if PROTOCOL_DIAGNOSTICS
	diag_printf("about to post %d bytes to %s:\n%s\n", send_len, server_url, send_buf);
#endif

	if (error == 0) {
		http_busy = GN_TRUE;

		error = gncomm_HTTPToBuffer(&http_req, server_url, receive_buf, receive_len, our_callback, our_user_data);

		http_busy = GN_FALSE;
	}

#if PROTOCOL_DIAGNOSTICS
	if (error == 0) {
		diag_printf("received %d bytes:\n", *receive_len);
		if (*receive_len && *receive_buf)
			printf("Data: %s\n", *receive_buf);
	}
	else
		diag_printf("received error %X\n", error);
#endif

	return error;
}

gn_error_t
download_file(gn_cstr_t url, gn_cstr_t local_filename)
{
	gn_error_t error = 0;
	gn_cstr_t slash = 0;
	gncomm_HTTP_req_t http_req = { 0 };
	gn_handle_t file_handle = 0;

	if (url == 0 || local_filename == 0) {
		return OPERR_InvalidArg;
	}

	file_handle = gnfs_create(local_filename, FSMODE_ReadWrite, FSATTR_ReadWrite);

	if (file_handle == FS_INVALID_HANDLE) {
		return gnfs_get_error();
	}

	gnmem_memset(&http_req, 0, sizeof(gncomm_HTTP_req_t));

	http_req.body = 0;
	http_req.body_size = 0;
	http_req.method = HTTP_Get;
	http_req.other_headers = 0;
	http_req.proxy_port = proxy_port;
	http_req.proxy_server = (proxy_server != 0 && *proxy_server != 0) ? proxy_server : 0;
	http_req.user_agent = STR_user_agent;

	if (error == 0) {
		http_busy = GN_TRUE;

		error = gncomm_HTTPToFile(&http_req, url, file_handle, 0, 0);

		http_busy = GN_FALSE;
	}

	gnfs_close(file_handle);

	return error;
}


/*
 * Private Functions
 */

static gn_error_t
transmit_cddbmsg(XMLTagRef cddbmsg_to_send, gn_cstr_t server_url, XMLTagRef* cddbmsg_received)
{
	gn_error_t error = 0;
	gn_str_t cddbmsg_str_to_send = 0;
	gn_str_t cddbmsg_str_received = 0;
	gn_size_t cddbmsg_str_len = 0;

	if (cddbmsg_to_send == 0 || cddbmsg_received == 0 || server_url == 0) {
		return OPERR_InvalidArg;
	}

	*cddbmsg_received = 0;

	/* convert cddbmsg_to_send to a string */
	cddbmsg_str_to_send = RenderXMLTagToStr(cddbmsg_to_send, GN_TRUE);

	if (cddbmsg_str_to_send == 0) {
		error = OPERR_NoMemory;
	}

	/* post the string */
	if (error == 0 && cddbmsg_str_to_send != 0) {
		error = post_buffer(cddbmsg_str_to_send, strlen(cddbmsg_str_to_send), server_url,
		                    &cddbmsg_str_received, &cddbmsg_str_len);
	}

	/* we no longer need the CDDBMSG string */
	if (cddbmsg_str_to_send != 0) {
		gnmem_free((void*)cddbmsg_str_to_send);
		cddbmsg_str_to_send = 0;
	}

	/* parse the response back into an XML structure */
	if (error == 0) {
		if (cddbmsg_str_received != 0 && cddbmsg_str_len > 0) {
			error = ParseBufToXMLTag(cddbmsg_str_received, cddbmsg_str_len, cddbmsg_received);
		} else {
			error = OPERR_NoServerResponse;
		}
	}

	/* we no longer need the received cddbmsg string */
	if (cddbmsg_str_received != 0) {
		gnmem_free((void*)cddbmsg_str_received);
		cddbmsg_str_received = 0;
	}

	return error;
}
