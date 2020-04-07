/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_comm.h
 *
 * Implements the communications interface portion of the platform abstraction layer.
 *
 */

#ifndef	__GN_COMM_H__
#define	__GN_COMM_H__


/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/port.h>


#ifdef	__cplusplus
extern "C"
{
#endif	/* ifdef __cplusplus */


/*
 * Constants
 */

/* Max number of connections at a time */
#define		GNCOMM_MaxConnections			64

/* Default number of connections at a time */
#define		GNCOMM_DefaultConnections		4

/* Value of an invalid connection */
#define		GNCOMM_InvalidHandle			-1


/* Types of connections that we can have */
#define		GNCOMM_TCP						0 /* TCP socket connection */

/* Values returned from a callback function, see below */
#define		CBRET_Continue					0
#define		CBRET_Cancel					1


/* Types of notifications we can receive.  Untyped to allow user-defined notifications. */
#define	CBNOT_Complete				0
#define	CBNOT_Status				1	/* p1: int receive body bytes received so far; p2: int total bytes to receive */
#define	CBNOT_Error					2
#define	CBNOT_Timeout				3
	/* notifications from high-level HTTP connections */
#define	CBNOT_HTTPRecvHdr			4	/* p1: char* null-terminated HTTP header line; p2: int length of p1 */
#define	CBNOT_HTTPRecvBody			5	/* p1: char* bytes just received; p2: int size of p1 buffer */
#define	CBNOT_HTTP_ContentLength	6	/* p1: int HTTP content length of receive body; p2: zero */
#define	CBNOT_HTTPStatus			7	/* p1: int HTTP status code; p2: char* HTTP headers */
#define	CBNOT_HTTPSendHdr			8	/* p1: int total bytes sent (header); p2: int total bytes to send (header + body) */
#define	CBNOT_HTTPSendBody			9	/* p1: int total bytes sent (header + body); p2: int total bytes to send (header + body) */

#define	BASE_GN_COMM_PKG_ERR				0


/* Types of HTTP operetions we support */
#define HTTP_Get		0
#define HTTP_Post		1


/*
 * Typedefs
 */

/* Handle to an abstract connection */
typedef gn_int32_t		gn_connection_t;

typedef gn_uint32_t		gn_commtype_t;
typedef gn_uint32_t		gncomm_cbret_t;

/* Callback proc used by blocking functions. See the list of notifiers */
/* above for details on the values of p1 and p2 for each type of notifier. */
typedef		gncomm_cbret_t 
			(* gncomm_callback_t)(gn_connection_t handle, int notification, ui32_t p1, ui32_t p2, ui32_t user_data);

typedef gn_uint32_t		gncomm_HTTP_t;

/*
 * HTTP request structure that encapsulates method, caller-supplied
 * headers, and anything else that may be required (proxy information, etc.).
 */
typedef struct gncomm_HTTP_req
{
	gncomm_HTTP_t	method; 		/* GET, POST */
	const void		*body;			/* Body of request for POST */
	gn_uint32_t		body_size;		/* # bytes to send */
	const gn_char_t	*proxy_server;	/* intermediate server & port */
	gn_int16_t		proxy_port;
	const gn_char_t	*user_agent; 	/* to identify ourselves to the server */

	/* more headers to be determined as necessary */

	const gn_char_t	*other_headers;	/* any other HTTP headers, to be copied
									 * directly into the request
									 */
} gncomm_HTTP_req_t;


/*
 * Prototypes
 */

/* Initialization and shutdown routines */

/* Intitalize the communications layer before first use */
gn_error_t
gncomm_initialize(gn_int32_t num_connections);

/* Shutdown the communications layer after last use */
gn_error_t
gncomm_shutdown(void);

/* Report initialization status */
gn_bool_t
gncomm_is_initialized(void);


/* Connection management routines */

/* Initialize and open a connection to a remote machine */
gn_error_t
gncomm_connect(const gn_uchar_t *address, gn_int32_t port, gn_connection_t *conn_handle, gn_commtype_t type);

/* Close down a connection and free its resources */
gn_error_t
gncomm_disconnect(gn_connection_t connection);

/* Set or get current connection timeouts. The values passed are in seconds */
gn_int32_t gncomm_get_connect_timeout(void);
gn_int32_t gncomm_get_transfer_timeout(void);
gn_error_t gncomm_set_connect_timeout(gn_int32_t timeout);
gn_error_t gncomm_set_transfer_timeout(gn_int32_t timeout);

/* Associate a callback and optional user data with a connection */
gn_error_t
gncomm_set_callback(gn_connection_t connection, gncomm_callback_t callback,
					ui32_t user_data, gncomm_callback_t *old_callback);


/* Data transfer routines */

/* Send data to the remote machine */
gn_error_t
gncomm_send(gn_connection_t connection, const void *buffer, gn_int32_t size, gn_int32_t *size_sent);

/* Receive data from the remote machine */
gn_error_t
gncomm_receive(gn_connection_t connection, void *buffer, gn_int32_t size, gn_int32_t *size_received);


/* Prototypes for HTTP routines */

/* Cancel any outstanding HTTP request */
gn_error_t
gncomm_HTTP_cancel(void);

/* Returns the last HTTP status code processed, so if the comm manager
 * returns COMMERR_HTTPClientError (4xx) or COMMERR_HTTPServerError (5xx)
 * we can find out what the whole error value was.
 */
gn_uint32_t gncomm_HTTP_get_status();

/* Perform an HTTP request, relying upon a user-supplied callback to
 * handle received data.
 */
gn_error_t
gncomm_HTTP(gncomm_HTTP_req_t * params, const gn_char_t *URL, 
            gn_char_t *buffer, gn_int32_t buffer_size, gn_int32_t *total_size,
            gncomm_callback_t callback, gn_uint32_t user_data);

/* Perform an HTTP request, storing the results in a file */
gn_error_t
gncomm_HTTPToFile(gncomm_HTTP_req_t *params, const gn_char_t *URL, gn_handle_t fhandle,
                  gncomm_callback_t callback, gn_uint32_t user_data);

/* Perform an HTTP request, allocation a buffer for the results. The caller
 * is then responsible for freeing the buffer.
 */
gn_error_t
gncomm_HTTPToBuffer(gncomm_HTTP_req_t *params, const gn_char_t * URL, 
                    void **buffer, gn_uint32_t *total_size,
                    gncomm_callback_t callback, gn_uint32_t user_data);



#ifdef	__cplusplus
}
#endif	/* ifdef __cplusplus */

#endif	/* ifndef	__GN_COMM_H__ */
