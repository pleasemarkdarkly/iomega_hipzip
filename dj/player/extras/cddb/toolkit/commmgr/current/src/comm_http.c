/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * comm_http.c
 *
 * Implements the higher-level HTTP facilities on top of the basic
 * comm layer functionality.
 */

#include <extras/cddb/gn_platform.h>
#include	GN_STRING_H
#include	GN_CTYPE_H
#include	GN_STDLIB_H
#include <extras/cddb/gn_comm.h>
#include <extras/cddb/toolkit/commmgr/comm_native.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_fs.h>


/* size of the biggest available network host name, as per RFC? */
#ifndef 	MAXHOSTNAMELEN
	#define 	MAXHOSTNAMELEN		256
#endif	  

/* how big a buffer to allocate to hold a single reeived header (to start) */
#ifndef HEADER_BUFF_SIZE
	#define 	HEADER_BUFF_SIZE		1024
#endif

/* default port used by HTTP */
#define 	HTTP_PORT				80

/* size of buffer for receiving chunks at a time */
#ifndef RECEIVE_BUFFER_SIZE
	#define 	RECEIVE_BUFFER_SIZE 	4096
#endif

/* default size to use when allocating (and reallocating) the buffer for 
** save-to-buffer */
#ifndef SAVE_TO_BUFFER_SIZE
	#define 	SAVE_TO_BUFFER_SIZE 	4096
#endif	

/* 
** Strings, strings, strings 
*/

/* available request methods */
static const char * gRequestMethods[] = 
{
	"GET", "POST"
};

/* newline sequence */
static const char	gCRLF[] 				= "\x0d\x0a";

/* Names of various headers */
static const char gUserAgentFieldName[]		= "User-Agent";
static const char gContentLengthFieldName[]	= "Content-Length";
static const char gRedirectFieldName[]		= "Location";

/*
 * Numbers, numbers, numbers
 */
static const int	gHTTPMajorVersion		= 1;	/* HTTP 1.0 here */
static const int	gHTTPMinorVersion		= 0;

int gCancelled = 0;

/* This is the value of the last HTTP status code from the server, accessed
 * through gncomm_HTTP_get_status()
 */
static gn_uint32_t http_status = 0;


/* Default HTTP params, used when NULL is passed */
static gncomm_HTTP_req_t	gDefaultHTTPParams =
{
	HTTP_Get,		/* method */
	GN_NULL,		/* POST body data */
	0,			/* POST body length */
	GN_NULL,		/* proxy server */
	0,			/* proxy server port */
	GN_NULL,		/* user agent string */
	GN_NULL,		/* other headers */
};

/* structure that allows us to chain callbacks, encapsulating user data */
typedef struct
{
	gncomm_callback_t	prev_callback;
	ui32_t				prev_user_data;
	ui32_t				user_data;
	gn_error_t			error;
} chained_callback_t;

/* structure for managing the ever-growing buffer used by HTTPToBuffer() */
typedef struct
{
	void *		buffer;
	gn_size_t	allocated_size;
	gn_size_t	current_size;

} http_to_buff_t;


static gn_char_t *setup_host_info(gncomm_HTTP_req_t *params, const gn_char_t *URL, gn_char_t *host, gn_int32_t *port)
{
	gn_char_t		*request;
	gn_char_t 		*p;
	const gn_char_t	*org_url = URL;

	/* Pull off any heading url-method (i.e. https://) */
	p = strstr(URL, "://");
	if (p != NULL)
	{
		URL = p + 3;
	}
		
	/* Get the name of the server to connect to.  This will be determined
	 * by the proxy, if any.
	 * The presence of a proxy determines the format of the URI, so we'll
	 * do that as well.
	 */
	gnmem_memset(host, 0, MAXHOSTNAMELEN);  /* NTD: DANGEROUS: send in length of host */
	if (params -> proxy_server != NULL && *(params -> proxy_server) != 0)
	{
		/* we'll connect to the proxy as an intermediary */
		strncpy(host, params->proxy_server, MAXHOSTNAMELEN - 1);  /* NTD: DANGEROUS: send in length of host */
		*port = params->proxy_port;

		/* with a proxy, the request contains the remote server name */
		request = (gn_char_t *)org_url;
	}
	else
	{
		gn_char_t	*host_end;
		gn_int32_t	copy_size;
		
		/* no proxy, we connect directly to the remote machine */
		
		/* Find the end of the server name in the URL.	The format is
		 * server_name[:port]path
		 */
		/* start by looking for the port */
		p = strchr(URL, ':');
		if (p != GN_NULL)
		{
			/* mark the end for copying */
			host_end = p;

			/* after this will be the port we connect to, overriding the default */
			p++;
			*port = atoi(p);

			/* after the port comes the path */
			while (isdigit(*p))
			{
				p++;
			}

			request = p;
		}
		else
		{
			/* no port spec, URL is server/path */
			p = strchr(URL, '/');
			if (p == GN_NULL)
			{
				/* no path specified */
				p = strchr(URL, 0);
			}
			/* mark the end for copying */
			host_end = p;
		
			/* comes now the path */
			request = p;
			
			/* default port */
			*port = HTTP_PORT;
		}

		/* copy the server name - overflow the buffer not */
		copy_size = host_end - URL;
		if (copy_size > MAXHOSTNAMELEN)
		{
			copy_size = MAXHOSTNAMELEN;
		}

		strncpy(host, URL, copy_size);
		
		/* if there is no path specifed, we need to get the root */
		if (!*request)
		{
			request = "/";
		}
	}

	return (request);
}

static gn_char_t *generate_headers(gncomm_HTTP_req_t *params, const gn_char_t *request)
{
	gn_char_t	*buffer;
	gn_int32_t	length;
	gn_char_t	*p;

	/* allocate a buffer based on the length of the supplied headers,
	 * the length of the request, and a bunch of padding.
	 */
	length = strlen(gRequestMethods[params->method]) + strlen(request);
	if(params->user_agent != NULL)
	{
		length += strlen(params->user_agent) + strlen(gUserAgentFieldName);
	}

	if(params->other_headers != NULL)
	{
		length += strlen(params -> other_headers);
	}
	length += 256;	/* padding for all the other WS, crlfs, etc. */  /* NTD: Check: is this safe? */

	/* If we're sending data to the server, we need to include the 
	 * content-length header.
	 */
	if(params->body_size != 0 && params->body != GN_NULL)
	{
		length += strlen(gContentLengthFieldName);
	}
	
	/* get a buffer or die */
	buffer = gnmem_malloc(length);
	if(buffer == GN_NULL)
	{
		/* blast you, low memory condition! */
		return GN_NULL;
	}

	/* OK, go ahead and make the header string.  This is in the format
	 *	Method SP Request-URI SP HTTP-Version CRLF Headers CRLF
	 */
	p = buffer + sprintf(buffer, "%s %s HTTP/%d.%d%s", 
						gRequestMethods[params -> method],
						request,
						gHTTPMajorVersion,
						gHTTPMinorVersion,
						gCRLF);
	
	/* add on headers if we have them */
	if(params->user_agent != NULL)
	{
		p += sprintf(p, "%s: %s%s", gUserAgentFieldName, params -> user_agent, gCRLF);
	}

	if(params->body_size != 0 && params->body != GN_NULL)
	{
		p += sprintf(p, "%s: %ld%s", gContentLengthFieldName, params->body_size, gCRLF);
	}
				
	/* add any other headers provided by the application */
	/* these are assumed to have the proper format, crlfs, etc. */	  
	if(params->other_headers != NULL)
	{
		strcat(p, params->other_headers);
	}

	/* terminate with an empty header */
	strcat(p, gCRLF);
	
	/* all done, pass back the buffer */
	return (buffer);	
}

static gn_char_t *copy_header(gn_char_t **headers, gn_int32_t *headers_size, gn_char_t *header, gn_int32_t header_len)
{
	gn_char_t	*p;

	if (header_len == 0)
		return *headers;

	/* If the new header won't fit, you must reallocit! */
	if((gn_int32_t)strlen(*headers) + header_len + 1 > *headers_size)
	{
		*headers_size += header_len + HEADER_BUFF_SIZE;
		p = gnmem_realloc(*headers, *headers_size);

		if (p == GN_NULL)
			return GN_NULL;

		*headers = p;	 
	}

	p = strchr(*headers, 0);
	strncpy(p, header, header_len);
	p[header_len] = 0;

	return (*headers);
}							   

static gn_int32_t copy_headers(gn_char_t **headers, gn_int32_t *headers_size, gn_char_t *inbuff, gn_int32_t *buff_size)
{
	/* Copy server response headers from 'buffer' to 'headers', reallocating
	 * 'headers' as necessary.
	 */

	gn_char_t	*p;
	gn_char_t	*buff_start;
	gn_char_t	*cur_tail;
	gn_int32_t	total;
	gn_int32_t	done;
	
	/* Go through the buffer for the end of the headers, symbolized by
	 * \r\n\r\n.  Look until we find it or run out of data.
	 */
	cur_tail = NULL;
	buff_start = inbuff;
	
	for(done = 0, total = 0, p = buff_start; !done && total < *buff_size; total++)
	{
		/* 3 possibilities:
		 * find \n with chars after
		 * find \n with no chars after (end of buffer)
		 * find no \n
		 */
		if (*p == '\n')
		{
			/* found one, copy it off */
			if (!copy_header(headers, headers_size, buff_start, p - buff_start + 1))
			{
				/* outa memory */
				return (-1);
			}

			/* are we done yet? */
			cur_tail = strchr(*headers, 0);
			if (cur_tail - *headers >= 4)
			{
				/* we have at least enough for the \r\n\r\n */
				if (*(cur_tail - 4) == '\r' &&
					*(cur_tail - 3) == '\n' &&
					*(cur_tail - 2) == '\r' &&
					*(cur_tail - 1) == '\n')
				{
					/* we're done - yay! */
					done = 1;
				}
			}
			
			/* skip to the next thing in the buffer */
			buff_start = p + 1;
		}
		p++;
	}	 

	if (!done)
	{
		/* if we got to the end of the buffer without seeing all the headers,
		 * we need to copy the rest of the buffer from the last \n to the 
		 * end of data.
		 */
		if(!copy_header(headers, headers_size, buff_start, p - buff_start))
		{
			/* outa memory */
			return (-1);
		}
	}

	/* copy what's left over what we used */
	gnmem_memmove(inbuff, p, *buff_size - total);

	/* update the size left in the buffer */
	*buff_size -= total;

	/* did we get them all or what? */
	return (done);
}

static gn_int32_t handle_headers(gn_char_t *headers, gn_int32_t *status, gn_char_t **redirect, gn_int32_t *content_length, gncomm_callback_t callback, ui32_t user_data)
{
	/* We have all the headers by now.	Time to parse through them, looking 
	 * for various information of interest.  These include the server response
	 * code, the content length field, and a redirection, if so provided.
	 */

	gn_char_t	*p;
	gn_int32_t	length;
	
	/* First, the response code.  It will be the first thing in the response,
	 * in the form
	 *
	 *	HTTP/x.y SP NNN SP reason text\r\n
	 *
	 */
	
	/* go past the HTTP/1.0 */
	p = headers;
	while (!isspace(*p))
	{
		p++;
	}

	/* spaces, then 3 digits */
	*status = atoi(p + 1);
	
	/* mark the text of the status message */
	headers = strchr(p + 1, ' ');
	if (headers != NULL)
	{
		while (isspace(*headers))
		{
			headers++;
		}
	}

	/* to the end of this one */
	p = strchr(p, '\n');
	if (p == NULL)
	{
		/* this should never happen */
		return (1);
	}

	/* nul-term by removing crlf */
	*(p - 1) = 0;

	/* call the notifier with the HTTP response code, as well */
	if(callback(0, CBNOT_HTTPStatus, *status, (ui32_t)headers, user_data) == CBRET_Cancel)
	{
		/* caller didn't like the status code */
		return (COMMERR_HTTPCancelled);
	}
	
	/* Now that that's done, we'll go through each header, passing each to the
	 * notification proc, just for fun, and looking for certain ones.
	 */
	do
	{
		gn_char_t	*val;

		/* get the next header, nul-terminate (replacing newlines) */
		headers = p + 1;
		p = strchr(headers, '\n');
		*(p - 1) = 0;
		length = p - headers - 1;

		/* call the notifier, which may ask us to cancel. */
		if(callback(0, CBNOT_HTTPRecvHdr, (ui32_t)headers, length, user_data) == CBRET_Cancel)
		{
			/* the caller wants to bail */
			return (COMMERR_HTTPCancelled);
		}
		
		/* look at the header value */
		val = strchr(headers, ':');
		if(val == NULL)
		{
			/* this should never happen */
			return (1);
		}
		*val++ = 0;
		while (isspace(*val))
		{
			val++;
		}
			
		/* now look at the header field */
		/* XXX: Should this be case-insensitive? */
		if(strcmp(headers, gContentLengthFieldName) == 0)
		{
			/* Content-Length: # of bytes in response entity body */
			*content_length = atoi(val);
		}
		else
		if(strcmp(headers, gRedirectFieldName) == 0)
		{
			/* resource is at a different location */
			*redirect = val;
		}
		/* others later... */
	
	  /* headers end with empty header ("\r\n" only) */
	} while (length > 0);
	
	return (1);
}

/* Callback used by gncomm_HTTPToFile */
static gncomm_cbret_t HTTPToFile_callback(gn_connection_t handle, int notification,
										  ui32_t p1, ui32_t p2, ui32_t user_data)
{
	chained_callback_t *	cc;
	
	/* get the user data we were passed */
	cc = (chained_callback_t *)user_data;
   
	/* Write all non-header data to a file.  user_data holds the file handle
	 * to use.
	 */
	if (notification == CBNOT_HTTPRecvBody)
	{
		/* write this chunk to the file */
		if (gnfs_write((gn_handle_t)cc -> user_data, (void *)p1, (gn_size_t)p2) != p2)
		{
			/* error writing to file - now what? */
			cc -> error = COMMERR_IOError;
			return (CBRET_Cancel);
		}
	}
   
	/* Call the user callback, if any */
	if (cc -> prev_callback != NULL)
	{
		gncomm_cbret_t	cbret;
	
		cbret = cc -> prev_callback(handle, notification, p1, p2, cc -> prev_user_data);
		if (cbret != CBRET_Continue)
			return (cbret);
	}
	
	return (CBRET_Continue);
}

/* Callback used by gncomm_HTTPToBuffer */
static gncomm_cbret_t HTTPToBuffer_callback(gn_connection_t handle, int notification,
											ui32_t p1, ui32_t p2, ui32_t user_data)
{
	chained_callback_t *	cc;
	http_to_buff_t *		hb;
		
	/* get the user data we were passed */
	cc = (chained_callback_t *)user_data;
	hb = (http_to_buff_t *)cc -> user_data;
	
	if (notification == CBNOT_HTTP_ContentLength)
	{
		/* Allocate a buffer for the incoming data.  If we're lucky, the
		 * server sent the Content-Length header, and p1 tells us how many 
		 * bytes we'll need.  If not, p1 will be 0 and we'll have to guess
		 * now and reallocate later.
		 */
		if (p1 == 0)
			p1 = SAVE_TO_BUFFER_SIZE;
			
		hb -> buffer = gnmem_malloc((gn_size_t)p1);
		if (hb -> buffer == NULL)
		{
			/* not so good */
			cc -> error = COMMERR_NoMemory;
			return (CBRET_Cancel);
		}
		
		hb -> allocated_size = (gn_size_t)p1;
		hb -> current_size = 0;
	} 
	else
	if (notification == CBNOT_HTTPRecvBody)
	{
		/* Append this chunk to the buffer, reallocating if necessary */
		/* p1 = buffer, p2 = size */
		if (p2 + hb -> current_size > hb -> allocated_size)
		{
			/* need to realloc */
			hb -> allocated_size = 
				(gn_size_t)(p2 + hb -> current_size + SAVE_TO_BUFFER_SIZE);
			hb -> buffer = gnmem_realloc(hb -> buffer, hb -> allocated_size);
			if (hb -> buffer == NULL)
			{
				/* not so good at all */
				cc -> error = COMMERR_NoMemory;
				return (CBRET_Cancel);
			}
		}
		
		/* append */
		gnmem_memcpy((char *)hb -> buffer + hb -> current_size, (void *)p1, (gn_size_t)p2);
		hb -> current_size += (gn_size_t)p2;
	}
	
	/* Call the user callback, if any */
	if (cc -> prev_callback != NULL)
	{
		gncomm_cbret_t	cbret;
	
		cbret = cc -> prev_callback(handle, notification, p1, p2, cc -> prev_user_data);
		if (cbret != CBRET_Continue)
			return (cbret);
	}
	
	return (CBRET_Continue);
}


/*
 * Public routines
 */

/* Cancel any outstanding HTTP request,
 *
 */
gn_error_t
gncomm_HTTP_cancel(void)
{
	gCancelled = 1;

	return GNERR_ERR_CODE(COMMERR_NoError);
}


/* Returns the last HTTP status code processed, so if the comm manager
 * returns COMMERR_HTTPClientError (4xx) or COMMERR_HTTPServerError (5xx)
 * we can find out what the whole error value was.
 */
gn_uint32_t gncomm_HTTP_get_status()
{
	return http_status;
}


/* Perform an HTTP request, relying upon a user-supplied callback to
 * handle received data.
 */
gn_error_t gncomm_HTTP(gncomm_HTTP_req_t * params, const gn_char_t *URL, 
						gn_char_t *buffer, gn_int32_t buffer_size, gn_int32_t *total_size,
						gncomm_callback_t callback, gn_uint32_t user_data)
{
	gn_error_t	error = COMMERR_NoError;
	gn_char_t	host[MAXHOSTNAMELEN];
	gn_int32_t	port;
	gn_char_t	*request;
	gn_char_t	*headers;
	gn_int32_t	headers_size;
	gn_connection_t	conn;
	gncomm_cbret_t	cb_ret = CBRET_Continue;
	gn_char_t	*redirect;
	gn_int32_t	content_length;
	gn_int32_t	status;
	gn_int32_t	headers_complete;
	gn_uint32_t	total_sent = 0;
	gn_uint32_t	total_to_send = 0;
	
	/* since we haven't done any work yet, we should ignore the gCancelled flag */
	gCancelled = 0;

	/* validate params, set up defaults */
	if(URL == GN_NULL || *URL == 0 || buffer == GN_NULL || buffer_size == 0 || callback == GN_NULL)
	{
		return GNERR_ERR_CODE(COMMERR_InvalidArg);
	}

	/* if the params are NULL, use the defaults */
	if (params == NULL)
	{
		params = &gDefaultHTTPParams;
	}
	
	/* Get the name of the server to connect to.  This will be determined
	 * by the proxy, if any.
	 * The presence of a proxy determines the format of the URI, so we'll
	 * do that as well.
	 */
	request = setup_host_info(params, URL, host, &port);
	
	/* Now generate the headers.  This is tricky, as their size can grow 
	 * arbitrarily.
	 */
	headers = generate_headers(params, request);
	if (headers == NULL)
	{
		/* No memory for allocation header buffer. */
		return GNERR_ERR_CODE(COMMERR_NoMemory);
	}

	/* Headers are ready, connection info is set up, we're good to go.
	 * Let's fire up the connection to the remote server.
	 */
	error = gncomm_connect(host, port, &conn, GNCOMM_TCP);
	if(error != COMMERR_NoError)
	{ 
		/* couldn't connect somehow - clean up and disappear */
		gnmem_free(headers);
		return GNERR_ERR_CODE(error);
	}

	if (gCancelled)
	{
		gCancelled = 0;
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(COMMERR_HTTPCancelled);
	}

	/* If we get here, we're successfully connected.  Start by sending the
	 * headers, the follow with the request body, if any.
	 */
	total_sent = 0;
	total_to_send = strlen(headers) + params->body_size;

	if((cb_ret = callback(0, CBNOT_HTTPSendHdr, total_sent, total_to_send, user_data)) != CBRET_Continue)
	{
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(COMMERR_HTTPCancelled);
	}

	error = gncomm_send(conn, headers, strlen(headers), NULL);

	total_sent += strlen(headers);

	if ((cb_ret = callback(0, CBNOT_HTTPSendHdr, total_sent, total_to_send, user_data)) != CBRET_Continue)
	{
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(COMMERR_HTTPCancelled);
	}

	gnmem_free(headers);
	
	if (error == COMMERR_NoError)
	{
		/* send the body, if any */
		if (params -> body_size != 0 && params -> body != NULL)
		{
			/* Send the request body as well.  Should we break it up and 
			 * call the callback, or just assume it will be small 
			 * enough to go without problems?
			 */
			error = gncomm_send(conn, params -> body, params -> body_size, NULL);

			total_sent += params -> body_size;

			if ((cb_ret = callback(0, CBNOT_HTTPSendBody, total_sent, total_to_send, user_data)) != CBRET_Continue)
			{
				gncomm_disconnect(conn);
				return GNERR_ERR_CODE(COMMERR_HTTPCancelled);
			}
		}
	}

	if (error != COMMERR_NoError)
	{
		/* No good, just no good */
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(error);
	}

	if (gCancelled)
	{
		gCancelled = 0;
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(COMMERR_HTTPCancelled);
	}

	/* OK, the send completed successfully.  Let's get the server's response.
	 * This will be the tricky part, as we have to parse the headers,
	 * determine where they end, and possibly take action based on what
	 * the headers say (redirects, etc.).
	 *
	 * The response will be in the form
	 *
	 *	header1\r\nheader2\r\n...headerN\r\n\r\nentity body
	 *
	 * We'll call the notification proc for each header, and periodically, for
	 * each bufferful.
	 */
	
	/* Start by allocating a buffer to hold a complete header, as the buffer
	 * we received may not be big enough.  I hate boundary conditions, 
	 * don't you?
	 */
	headers_size = HEADER_BUFF_SIZE;
	headers = gnmem_malloc(headers_size);
	if(headers == GN_NULL)
	{
		/* sigh */
		gncomm_disconnect(conn);
		return GNERR_ERR_CODE(COMMERR_NoMemory);
	}
	
	/* Do the reads, breaking it up into chunks that fill the caller's buffer. */
	*total_size = 0;
	*headers = 0;
	headers_complete = 0;
	
	redirect = NULL;
	content_length = 0;
	status = 200;	/* HTTP OK */
	
	while (1)	/* read till server closes connection */
	{
		long 	received;
		
		error = gncomm_receive(conn, buffer, buffer_size, &received);
		if (received == 0 || error != COMMERR_NoError)
			break;
		
		if (gCancelled)
		{
			cb_ret = CBRET_Cancel;
			gCancelled = 0;
			break;
		}
		
		/* Are we still parsing headers? */
		if (!headers_complete)
		{
			if (copy_headers(&headers, &headers_size, buffer, &received))
			{
				/* we copied all the headers, parse them and call the callback */
				if (!handle_headers(headers, &status, &redirect, &content_length, 
									callback, user_data))
				{
					/* callback returned CANCEL */
					break;
				}
				headers_complete = 1;

				http_status = status;

				/* Call the notifier to let it know how much data is coming.
				 * Routines that allocate a buffer to hold it will want to 
				 * know.
				 * Note that this value may not have appeard in the headers,
				 * in which case it will be 0 and the caller is on its own.
				 */
				if ((cb_ret = callback(0, CBNOT_HTTP_ContentLength, content_length, 0, user_data)) != CBRET_Continue)
				{
					break;
				}								  
			}
			
			/* if we chewed up all the data, go read some more */
			if (received == 0)
				continue;
		}
		
		/* bump the amount we've received */
		*total_size += received;
		
		/* If we have any data left after the headers, it's the response body.
		 * Call the notifier twice - once with a STATUS (# bytes received) 
		 * and once with an HTTP_RECV, to actually deal with the data.
		 * This gives us two chances to be canceled!
		 */
		if ((cb_ret = callback(0, CBNOT_Status, *total_size, content_length, user_data)) != CBRET_Continue)
			break;
		
		/* Don't send this notification for a redirect - it's not the data 
		 * we're looking for.
		*/
		if (redirect == NULL &&    
			(cb_ret = callback(0, CBNOT_HTTPRecvBody, (ui32_t)buffer, received, user_data)) != CBRET_Continue)
		{
			break;
		}
	}
	
	/* All done.  Close the connection. */
	gncomm_disconnect(conn);

	/* But we're not quite done yet.  If the server returned a redirect, we need to 
	 * do the entire process over again with the new URL.
	 * We'll call ourselves recursively.
	 */
	if (cb_ret != CBRET_Cancel && error == COMMERR_NoError && redirect != NULL)
	{
		*total_size = 0;
		error = gncomm_HTTP(params, redirect, buffer, buffer_size, total_size,
							 callback, user_data);
	}

	/* Now clean up and go away */
	gnmem_free(headers);
	
	/* Were we cancelled by caller? */
	if (cb_ret == CBRET_Cancel)
		error = COMMERR_HTTPCancelled;
	
	/* Error code should reflect returned status from server */
	if (error == COMMERR_NoError)
	{
		/* look at the 100's place digit: 2xx = OK, 3xx = moved (poss. redirect),
		** 4xx = client error, 5xx = server error
		*/

		switch (status / 100)
		{
			case 4:
			
				error = COMMERR_HTTPClientError;
				break;
		 
			case 5:
				
				error = COMMERR_HTTPServerError;
				break;
		}		 
	}
	gCancelled = 0;
	return GNERR_ERR_CODE(error);
}
 
/* Perform an HTTP request, storing the results in a file */
gn_error_t
gncomm_HTTPToFile(gncomm_HTTP_req_t * params, const char * URL, gn_handle_t fhandle,
				  gncomm_callback_t callback, ui32_t user_data)
{
	chained_callback_t	cc;
	char *				recv_buff;
	gn_int32_t				total_size;
	gn_error_t			retval;
	
	/* allocate a buffer for recieving purposes */
	recv_buff = gnmem_malloc(RECEIVE_BUFFER_SIZE);
	if (recv_buff == NULL)
		return GNERR_ERR_CODE(COMMERR_NoMemory);
	
	/* set up to call the supplied callback from within our own */
	cc.prev_callback = callback;
	cc.prev_user_data = user_data;
	
	/* pass the file handle to the callback */
	cc.user_data = (ui32_t)fhandle;

	/* clear the error, too */
	cc.error = COMMERR_NoError;
	
	/* call the main routine to do the work */
	retval = gncomm_HTTP(params, URL, recv_buff, RECEIVE_BUFFER_SIZE, &total_size,
						 HTTPToFile_callback, (ui32_t)&cc);
	
	/* clean up and go home */
	gnmem_free(recv_buff);
	return GNERR_ERR_CODE((cc.error == COMMERR_NoError ? retval : cc.error));
}

 
/* Perform an HTTP request, allocation a buffer for the results.  The caller
 * is then responsible for freeing the buffer.
 */
gn_error_t gncomm_HTTPToBuffer(gncomm_HTTP_req_t * params, const char * URL, 
							   void ** buffer, ui32_t * total_size,
							   gncomm_callback_t callback, ui32_t user_data)
{
	chained_callback_t	cc;
	http_to_buff_t		hb;
	char *				recv_buff;
	gn_error_t			retval;
	
	/* allocate a buffer for recieving purposes */
	recv_buff = gnmem_malloc(RECEIVE_BUFFER_SIZE);
	if (recv_buff == NULL)
		return GNERR_ERR_CODE(COMMERR_NoMemory);
	
	/* set up to call the supplied callback from within our own */
	cc.prev_callback = callback;
	cc.prev_user_data = user_data;
	
	/* pass the structure that will define the buffer to the callback */
	memset(&hb, 0, sizeof(hb));
	cc.user_data = (ui32_t)&hb;
	
	/* call the main routine to do the work */
	retval = gncomm_HTTP(params, URL, recv_buff, RECEIVE_BUFFER_SIZE, total_size,
						 HTTPToBuffer_callback, (ui32_t)&cc);
	
	/* transfer the buffer and size into the caller's variables */
	*buffer = hb.buffer;
	
	/* *total_size = hb.current_size; */
	
	/* clean up and go home */
	gnmem_free(recv_buff);
	return GNERR_ERR_CODE(retval);
}
