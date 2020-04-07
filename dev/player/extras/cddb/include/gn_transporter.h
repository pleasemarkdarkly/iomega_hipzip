/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * gn_transporter.h - defines an abstract interface to a protocol-agnostic 
 * transport package. First, create a transporter reference with one of 
 * the creation functions, then use it by calling one of its function 
 * pointers, and when you are done, delete it by calling its destroy 
 * function. Depending on which kind of transporter you create, you will 
 * get different behavior.
 */

#ifndef GN_TRANSPORTER_H
#define GN_TRANSPORTER_H

/*
 * Dependencies
 */

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C" {
#endif


/*
 * Constants
 */

#define INTERNET_TRANSPORTER	0
#define PC_TRANSPORTER			1


/*
 * Data Types
 */

typedef gn_uint32_t	gn_transporter_type;

typedef struct gn_transporter_t {
	gn_size_t			size;	/* this size might be greater than sizeof(gn_transporter_t) */
	gn_transporter_type	type;	/* will be the transporter's true type */

	void				(*destroy)(struct gn_transporter_t* transporter);

	gn_error_t			(*transmit)(struct gn_transporter_t* transporter,
	                                gn_cstr_t send_buf, gn_size_t send_len,
                                    gn_str_t* receive_buf, gn_size_t* receive_len);

	gn_error_t			(*download)(struct gn_transporter_t* transporter,
	                                gn_cstr_t url, gn_cstr_t* local_filename);
} gn_transporter_t;


/*
 * Creation Functions
 */

gn_transporter_t*
gn_create_internet_transporter(gn_cstr_t server_url);

/* Customers will want to change the parameter list for gn_create_pc_transporter
 * as necessary. For simplicity and portability, the sample application uses 
 * TCP/IP as its transport medium. Real implementations will want to use USB, 
 * wireless, or something else.
 */
gn_transporter_t*
gn_create_pc_transporter(gn_cstr_t address, unsigned short port);

#ifdef __cplusplus
}
#endif

#endif
