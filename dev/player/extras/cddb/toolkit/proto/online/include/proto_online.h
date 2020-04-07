/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_online.h - Application interface definition for the online protocol module.
 */
#ifndef _GN_ONLINE_H_
#define _GN_ONLINE_H_

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/microxml.h>
#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_comm.h>


#ifdef __cplusplus
extern "C" {
#endif

/*
 * protocol string constants
 */
extern const char* STR_CDDBMSG;
extern const char* STR_STATUS;
extern const char* STR_DATA;
extern const char* STR_PROTOCOL;
extern const char* STR_ENCODING;
extern const char* STR_ENCRYPTION;
extern const char* STR_COMPRESSION;
extern const char* STR_CHECKSUM;
#define STR_CRC_1 "crc32"

extern const char* STR_REQUESTS;
extern const char* STR_REQUEST;
extern const char* STR_RESULTS;
extern const char* STR_RESULT;
extern const char* STR_MAX_RESULTS_SIZE;

extern const char* STR_COMMAND;
extern const char* STR_TYPE;
extern const char* STR_OPTS;
extern const char* STR_ID;
extern const char* STR_IDTAG;

extern const char* STR_FORMAT;
extern const char* STR_CHARSET;
extern const char* STR_FMT_ID;
extern const char* STR_RSLTFMT_ID;

extern const char* STR_ALG;
extern const char* STR_base64;
extern const char* STR_none;
extern const char* STR_zip;

extern const char* STR_eCDDB;

extern const char* STR_NAME;
extern const char* STR_VERSION;
extern const char* STR_PROTO_ID;
extern const char* STR_CODE;
extern const char* STR_OK;

extern const char* STR_REG_REQ_ID;
extern const char* STR_HELLO_REQ_ID;
extern const char* STR_REGISTER;
extern const char* STR_DEVICE;
extern const char* STR_DEVID;
extern const char* STR_DEVID_TAG;
extern const char* STR_HELLO;
extern const char* STR_CLASS;
extern const char* STR_VER;
extern const char* STR_SW;
extern const char* STR_HW;
extern const char* STR_ECDDB;
extern const char* STR_CLIENT;
extern const char* STR_LICENSEE;

/*
 * in gn_payload.c
 */

gn_error_t
create_request_element(gn_cstr_t cmd, gn_cstr_t type, gn_cstr_t opts, gn_cstr_t id,
                       gn_cstr_t format_id, gn_cstr_t result_format_id,
                       XMLTagRef contents, XMLTagRef content2, XMLTagRef* request);

gn_error_t
create_format_element(gn_cstr_t id, gn_cstr_t charset, gn_cstr_t encoding, XMLTagRef* format);

gn_error_t
create_protocol_element(XMLTagRef* protocol);

gn_error_t
create_empty_requests_element(XMLTagRef* requests);

gn_error_t
create_basic_requests_element(gn_size_t max_results_size, XMLTagRef* requests);

gn_error_t
add_request_to_requests(XMLTagRef request, XMLTagRef requests);

gn_error_t
add_format_to_requests(XMLTagRef format, XMLTagRef requests);

gn_error_t
get_result_from_results(XMLTagRef results, XMLTagRef* result, gn_cstr_t id);

gn_error_t
get_successful_result_from_results(XMLTagRef results, XMLTagRef* result, gn_cstr_t id);

gn_error_t
transmit_requests(XMLTagRef requests, gn_cstr_t server_url, XMLTagRef* results);

gn_error_t
get_result_error(XMLTagRef result);


/*
 * in gn_cddbmsg.c
 */

gn_error_t
create_cddbmsg_element(XMLTagRef protocol_element, XMLTagRef requests_element,
                       gn_cstr_t proto_id, gn_cstr_t status, XMLTagRef* cddbmsg);

gn_error_t
extract_data_from_cddbmsg(XMLTagRef cddbmsg, XMLTagRef* results_element);

gn_error_t
unpack_cddbmsg(XMLTagRef cddbmsg, XMLTagRef* protocol_element,
               XMLTagRef* status_element, XMLTagRef* data_element);

gn_error_t
analyze_status(XMLTagRef status_element, XMLTagRef protocol_element,
               gn_bool_t* should_extract_data);


/*
 * in gn_transmit.c
 */

gn_error_t
online_configure_proxy_info(gn_cstr_t new_server, gn_uint16_t new_port);

gn_error_t
proto_protocol_transmission(XMLTagRef requests, gn_cstr_t server_url, XMLTagRef *results);


gn_error_t
post_buffer(gn_cstr_t send_buf, gn_size_t send_len, gn_cstr_t server_url,
            gn_str_t* receive_buf, gn_size_t* receive_len);

gn_error_t
download_file(gn_cstr_t url, gn_cstr_t local_filename);

gn_error_t
install_callback(gncomm_callback_t callback, gn_uint32_t user_data);

gn_error_t
cancel_online_transaction(void);


#ifdef __cplusplus
}
#endif

#endif	/* #ifndef _GN_ONLINE_H_ */

