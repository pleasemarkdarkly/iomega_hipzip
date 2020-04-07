/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * comm_core.c
 *
 * Implements the guts of the communications interface portion of the platform abstraction layer.
 *
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_platform.h>
#include GN_STRING_H
#include <extras/cddb/toolkit/commmgr/comm_core.h>
#include <extras/cddb/toolkit/commmgr/comm_native.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_memory.h>


/*
 * Constants.
 */

/* controls whether the raw socket I/O data is printed to stderr */
#define PRINT_IO 0

/*	How many bytes to send or receive at a time (for callback) */
#ifndef TRANSFER_QUANTUM
	#define 		TRANSFER_QUANTUM		1024
#endif

/* 
 * Default values for tineouts when waiting on connect/send/recv.
 * Should come from config...
 *
 * Values are in SECONDS.
 *
 * Note that timeouts are not currently supported for connects.
 */
#ifndef DEFAULT_CONNECTION_TIMEOUT
	#define 		DEFAULT_CONNECTION_TIMEOUT		30
#endif	
#ifndef DEFAULT_TRANSFER_TIMEOUT
	#define 		DEFAULT_TRANSFER_TIMEOUT		30
#endif


/*
 * Variables
 */

/* global list of connection info */
static	connection_list_t gConnectionList =
{
	0, 0, GN_NULL, DEFAULT_CONNECTION_TIMEOUT, DEFAULT_TRANSFER_TIMEOUT
};

/* Are we currently iniitialized? */
static gn_bool_t gInitialized = GN_FALSE;


/*
 * Prototypes
 */

/* Call the notification proc, if any, for this connection */
static gncomm_cbret_t call_callback(connection_info_t *conn, gn_int32_t notification, gn_uint32_t p1, gn_uint32_t p2);
static void clear_connection(connection_info_t * conn);
static void free_connection(connection_info_t * conn);
static connection_info_t *get_free_connection(void);
static connection_info_t *find_connection(gn_connection_t handle);


/*
 * Implementations
 */

static void clear_connection(connection_info_t * conn)
{
	conn->native_handle = -1;
	conn->in_use = 0;
	conn->callback = NULL;
	conn->user_data = 0;
}

static void free_connection(connection_info_t * conn)
{
	clear_connection(conn);
	conn->in_use = 0;
}

static connection_info_t *find_connection(gn_connection_t handle)
{
	connection_info_t *conn;

	/* Look in our connection table for one that matches this handle.
	 * Currently, handles are just indices (1-based) into the table.
	 */

	/* First, check for reasonable values */
	if ((int)handle <= 0 || (int)handle > gConnectionList.max_connections)
	{
		return GN_NULL;
	}

	/* It's a valid index, but is it a valid connection? */
	conn = &gConnectionList.connections[(int)handle - 1];

	return (conn->in_use ? conn : GN_NULL);
}

static connection_info_t *get_free_connection(void)
{
	/* Look through out info table looking for the first entry that
	 * is not in use.
	 */

	gn_int16_t	i;

	for(i = 0; i < gConnectionList.max_connections; i++)
	{
		connection_info_t *conn;
		
		conn = &gConnectionList.connections[i];
		
		if(!conn->in_use)
		{
			/* got it - clear it out and mark it as in use */
			clear_connection(conn);
			conn->in_use = 1;

			return conn;
		}
	}

	/* no such */
	return GN_NULL;
}

/* Call the notification proc, if any, for this connection */
static gncomm_cbret_t call_callback(connection_info_t *conn, gn_int32_t notification, gn_uint32_t p1, gn_uint32_t p2)
{
	/* is there a callback at all? */
	if(conn->callback == GN_NULL)
	{
		return CBRET_Continue;
	}

	/* call away, return what it returns */
	return(conn->callback(conn->handle, notification, p1, p2, conn->user_data));
}


/*
 * Public Routines
 */
gn_int32_t gncomm_get_connect_timeout(void)
{
	return(gConnectionList.connection_timeout);
}

gn_int32_t gncomm_get_transfer_timeout(void)
{
	return(gConnectionList.transfer_timeout);
}

gn_error_t gncomm_set_connect_timeout(gn_int32_t timeout)
{
	gn_error_t	error = COMMERR_NoError;

	if(timeout < 0)
	{
		return GNERR_ERR_CODE(COMMERR_InvalidArg);
	}

	gConnectionList.connection_timeout = timeout;

	return GNERR_ERR_CODE(error);
}

gn_error_t gncomm_set_transfer_timeout(gn_int32_t timeout)
{
	gn_error_t	error = COMMERR_NoError;

	if(timeout < 0)
	{
		return GNERR_ERR_CODE(COMMERR_InvalidArg);
	}

	gConnectionList.transfer_timeout = timeout;

	return GNERR_ERR_CODE(error);
}


/* Initialization and shutdown routines */


/* Intitalize the communications layer before first use */

/* Report initialization status */
gn_bool_t gncomm_is_initialized(void)
{
	return gInitialized;
}


/*
 * NOTE FOR FUTURE DEVELOPMENT: 
 *
 * Values for timeouts (gConnectionList.connection_timeout, 
 * gConnectionList.connection_timeout) should be pulled from the 
 * configuration manager at initialization instead of being hardcoded
 * in the structure declaration here.
 */
gn_error_t gncomm_initialize(gn_int32_t num_connections)
{
	gn_error_t	error = COMMERR_NoError;
	gn_int32_t	i;

	/* Do the requisite sanity checking */
	if(gInitialized == GN_TRUE)
	{
		return GNERR_ERR_CODE(COMMERR_AlreadyInitialized);
	}

	if(num_connections <= 0 || num_connections > GNCOMM_MaxConnections)
	{
		return GNERR_ERR_CODE(COMMERR_InvalidArg);
	}

	/* OK, params look sane.  Let's try to allocate the internal structures */
	gConnectionList.connections = (connection_info_t *)gnmem_malloc(num_connections * sizeof(connection_info_t));

	if(gConnectionList.connections == GN_NULL)
	{
		return GNERR_ERR_CODE(COMMERR_NoMemory);
	}
	
	/* That worked, now initalize */
	gConnectionList.max_connections = num_connections;
	gConnectionList.num_connections = 0;
	
	for(i = 0; i < num_connections; i++)
	{
		/* handles are currently just the indices into this table */
		gConnectionList.connections[i].handle = i + 1;  /* NTD: Make these i, not i+1 */
		clear_connection(&gConnectionList.connections[i]);
	}
	
	/* Do any platform-specific initialization before we go on */
	error = gncomm_native_initialize();
	if (error != COMMERR_NoError)
	{
		gnmem_free(gConnectionList.connections);
		gConnectionList.connections = NULL;

		return GNERR_ERR_CODE(error);
	}
	
	
	/* Lastly, mark ourselves as initialized */
	gInitialized = GN_TRUE;
	
	/* success! */
	return GNERR_ERR_CODE(error);
}

/* Shutdown the communications layer after last use */
gn_error_t gncomm_shutdown(void)
{
	gn_error_t	error = COMMERR_NoError;

	/* Do the requisite sanity checking */
	if(gInitialized != GN_TRUE)
	{
		return GNERR_ERR_CODE(COMMERR_NotInited);
	}

	/* Attempt any platform-specific cleanup before dumping our buffers */
	error = gncomm_native_shutdown();
	if (error != COMMERR_NoError)
	{
		return GNERR_ERR_CODE(error);
	}
	
	/* If there are open connections still, close them */
	/* Or should we fail? */
	
	/* Free our buffer */
	gnmem_free(gConnectionList.connections);
	gConnectionList.connections = GN_NULL;
	
	gInitialized = GN_FALSE;
	
	/* success! */
	return GNERR_ERR_CODE(error);
}


/* Connection management routines */

/* Initialize and open a connection to a remote machine */
gn_error_t gncomm_connect(const gn_uchar_t *address, gn_int32_t port, gn_connection_t *conn_handle, gn_commtype_t type)
{
	/* Note 'type' not currently used - all connections are TCP */

	gn_error_t		error = COMMERR_NoError;
	connection_info_t	*conn;

	if(gInitialized != GN_TRUE)
	{
		return GNERR_ERR_CODE(COMMERR_NotInited);
	}

	/* first-round sanity checking */
	if(address == GN_NULL || *address == 0)
	{
		return GNERR_ERR_CODE(COMMERR_InvalidAddress);
	}
	
	/* try to find a free spot in the connection table */
	conn = get_free_connection();
	if (conn == GN_NULL)
	{
		/* no more room in the table */
		return GNERR_ERR_CODE(COMMERR_NoMoreConnections);
	}

	/* OK, we have a new connection.  Time to try to connect to the other side.
	 * This will return 0 if nothing went wrong or a platform-specific error 
	 * otherwise.  If successful, it will fill out the native_handle field of 
	 * the connection structure.  otherwise, it will set the error fields.
	 */
	error = gncomm_native_connect(address, port, conn);
	if(error != COMMERR_NoError)
	{
		/* free up the connection object */
		free_connection(conn);

		return GNERR_ERR_CODE(error);
	}

	/* If we got here, then the connection worked happily.	Pass pack a valid handle. */

	*conn_handle = conn->handle;

	return GNERR_ERR_CODE(error);
}

/* Close down a connection and free its resources */
gn_error_t gncomm_disconnect(gn_connection_t connection)
{
	gn_error_t		error = COMMERR_NoError;
	connection_info_t	*conn;

	/* Look for the connection object that goes with this handle */
	conn = find_connection(connection);
	if(conn == GN_NULL)
	{
		/* Invalid handle */
		return GNERR_ERR_CODE(COMMERR_InvalidHandle);
	}

	/* Found one successfully. Attempt the disconnection. */
	error = gncomm_native_disconnect(conn);

	/* free up the connection object */
	free_connection(conn);

	/* success, unless disconnect failed */
	return GNERR_ERR_CODE(error);
}

/* Associate a callback and optional user data with a connection */
gn_error_t gncomm_set_callback(gn_connection_t connection, gncomm_callback_t callback, ui32_t user_data, gncomm_callback_t *old_callback)
{
	gn_error_t		error = COMMERR_NoError;
	connection_info_t	*conn;
	
	/* Look for the connection object that goes with this handle */
	conn = find_connection(connection);
	if (conn == GN_NULL)
	{
		/* Invalid handle */
		return GNERR_ERR_CODE(COMMERR_InvalidHandle);
	}	
	
	/* save the previous value */
	*old_callback = conn->callback;

	/* set the new */
	conn->callback = callback;
	conn->user_data = user_data;

	return GNERR_ERR_CODE(error);
}


/* Data transfer routines */

/* Send data to the remote machine */
gn_error_t gncomm_send(gn_connection_t connection, const void *buffer, gn_int32_t size, gn_int32_t *size_sent)
{
	gn_error_t		error = COMMERR_NoError;
	connection_info_t	*conn;
	gn_int16_t		success;
	gn_int16_t		canceled;

	/* Look for the connection object that goes with this handle */
	conn = find_connection(connection);
	if(conn == GN_NULL)
	{
		/* Invalid handle */
		return GNERR_ERR_CODE(COMMERR_InvalidHandle);
	}
	
#if PRINT_IO
	fwrite(buffer, sizeof(char), size, stderr);
#endif

	/* We're going to break big sends up into smaller pieces so we can
	 * call the notifier callback periodically.  This gives us a chance
	 * to keep the app informed about what is happening, and gives the
	 * app a chance to cancel if they want.
	 *
	 * We can optimize things a bit by stipulating that we'll send it 
	 * all in one go if the calback is null.
	 */

	/* assume the best */
	success = 1;
	canceled = 0;
	
	if(conn->callback == GN_NULL)
	{
		/* call the native send routine, which returns 0 on success */
		error = gncomm_native_send(buffer, size, size_sent, conn);
		if(error != COMMERR_NoError)
		{
			success = 0;
		}
	}
	else
	{
		gn_int32_t	size_remaining;
		gn_uchar_t	*send_buff;

		/* We get here, we're splitting the send into pieces. */
		size_remaining = size;
		send_buff = (gn_uchar_t *)buffer;
		
		/* Do the do */
		while(size_remaining > 0)
		{
			gn_int32_t	num_sent;
			gn_int32_t	send_size;
			gncomm_cbret_t	cbret;

			/* how much to send this time? */
			send_size = size_remaining < TRANSFER_QUANTUM ? size_remaining : TRANSFER_QUANTUM;

			/* call the underlying send routine */
			error = gncomm_native_send(send_buff, send_size, &num_sent, conn);
			if (error != COMMERR_NoError)
			{
				/* call the notifier with an error notification */
				/* NTD: This callback param list needs some rewrite */
				call_callback(conn, CBNOT_Error, error, 0 /* conn->last_native_error*/);

				success = 0;
				break;				
			}
		
			/* update the total sent, increment the buffer, etc. */
			send_buff += num_sent;
			size_remaining -= num_sent;
		
			/* call the notifier with the percentage complete */
			cbret = call_callback(conn, CBNOT_Status, size - size_remaining, size);
			
			/* if the callback said to cancel, we cancel */
			if(cbret == CBRET_Cancel)
			{
				canceled = 1;
				break;
			}
		}
	
		/* all done, set the caller's value for total size sent */
		if (size_sent != GN_NULL)
		{
			*size_sent = size - size_remaining;
		}
	}	 
	
	/* Call the callback with a completion notification.  p1 will be any 
	 * error code, p2 will be 0 if the operation completed, non-zero if
	 * the operation was canceled.
	 */
	call_callback(conn, CBNOT_Complete, error, canceled);

	return GNERR_ERR_CODE(error);
}

/* Receive data from the remote machine */
gn_error_t gncomm_receive(gn_connection_t connection, void *buffer, gn_int32_t size, gn_int32_t *size_received)
{
	gn_error_t		error = COMMERR_NoError;
	connection_info_t	*conn;
	gn_int16_t		success;
	gn_int16_t		canceled;

	/* Look for the connection object that goes with this handle */
	conn = find_connection(connection);
	if(conn == GN_NULL)
	{
		/* Invalid handle */
		return GNERR_ERR_CODE(COMMERR_InvalidHandle);
	}	

	/* We're going to break big recieves up into smaller pieces so we can
	 * call the notifier callback periodically.  This gives us a chance
	 * to keep the app informed about what is happening, and gives the
	 * app a chance to cancel if they want.
	 *
	 * We can optimize things a bit by stipulating that we'll get it 
	 * all in one go if the calback is null.
	 */

	/* assume the best */
	success = 1;
	canceled = 0;
	
	if(conn->callback == GN_NULL)
	{
		/* call the native send routine, which returns 0 on success */
		error = gncomm_native_receive(buffer, size, size_received, conn);
		if(error != COMMERR_NoError)
		{
			success = 0;
		}
	}
	else
	{
		gn_int32_t	size_remaining;
		gn_uchar_t	*receive_buff;

		/* We get here, we're splitting the receive into pieces. */
		size_remaining = size;
		receive_buff = (gn_uchar_t *)buffer;

		/* Do the do */
		while (size_remaining > 0)
		{
			gn_int32_t	num_received;
			gn_int32_t	receive_size;
			gncomm_cbret_t	cbret;

			/* how much to receive this time? */
			receive_size = size_remaining < TRANSFER_QUANTUM ? size_remaining : TRANSFER_QUANTUM;
			
			/* call the underlying receive routine */
			error = gncomm_native_receive(receive_buff, receive_size, &num_received, conn);
			if(error != COMMERR_NoError)
			{
				/* call the notifier with an error notification */
				call_callback(conn, CBNOT_Error, error, 0 /*conn -> last_native_error*/);

				success = 0;
				break;				
			}
		
			/* update the total received, increment the buffer, etc. */
			receive_buff += num_received;
			size_remaining -= num_received;
		
			/* call the notifier with the percentage complete */
			cbret = call_callback(conn, CBNOT_Status, size - size_remaining, size);
			
			/* if the callback said to cancel, we cancel */
			if (cbret == CBRET_Cancel)
			{
				canceled = 1;
				break;
			}
			
			/* if we received less than we asked for, the other side is done, 
			 * and so are we 
			 */
			if(num_received < receive_size)
			{
				break;
			}
		}

		/* all done, set the caller's value for total size sent */
		if(size_received != GN_NULL)
		{
			*size_received = size - size_remaining;
		}
	}	 
	
#if PRINT_IO
	fwrite(buffer, sizeof(char), *size_received, stderr);
#endif

	/* Call the callback with a completion notification.  p1 will be any 
	 * error code, p2 will be 0 if the operation completed, non-zero if
	 * the operation was canceled.
	 */
	call_callback(conn, CBNOT_Complete, error, canceled);

	/* success? */
	
	return GNERR_ERR_CODE(error);
}
