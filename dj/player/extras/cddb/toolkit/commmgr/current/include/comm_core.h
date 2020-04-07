/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * comm_core.h
 *
 * Interface to core routines for the communications layer
 *
 */

#ifndef _COMM_CORE_H_
#define _COMM_CORE_H_

#include <extras/cddb/gn_comm.h>

/*
 * Internal types and variables
 */

/* Structure to maintain connection information */
typedef struct connection_info
{
	gn_connection_t		handle;
	gn_int32_t		native_handle;
	gn_int32_t		in_use;
	gncomm_callback_t	callback;
	gn_uint32_t		user_data;
} connection_info_t;


typedef struct connection_list
{
	gn_int32_t		max_connections;
	gn_int32_t		num_connections;
	connection_info_t	*connections;
	gn_int32_t		connection_timeout;
	gn_int32_t		transfer_timeout;
} connection_list_t;


extern int gCancelled;


#endif	/* ifndef _COMM_CORE_H_ */
