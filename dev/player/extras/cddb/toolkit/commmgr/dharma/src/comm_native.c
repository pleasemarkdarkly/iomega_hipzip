/*
 * Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/* comm_native.c
 *
 * Implementation of platform-specific routines for the communications layer
 *
 * Parts of this module will need to change, depending on the platform specifics.
 *
 * This version supports Unix (all Berkeley-conforming versions).
 *
 */

/* By default, if we have BSD sockets, we have select(), used for determining 
 * if a socket is readable or writable, or whether a connnect() has completed.	
 * Turn this off in platform.h (included below via gn_platform.h) if your 
 * platform does not provide this or an equivalent.
 */
#define 	GN_COMM_HAVE_SELECT

#include <extras/cddb/gn_platform.h>
#include <extras/cddb/toolkit/commmgr/comm_native.h>
#include <extras/cddb/gn_errors.h>
#include <netdb.h>
//#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
//#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>   /* for struct timeval */

#include <pkgconf/net.h>
#include <network.h>
#include <eth_drv.h>
#include <netdev.h>

#include <dns.h>

#define SOCK_WAIT_INTERVAL	250

/* MSG_NOSIGNAL is not defined on sun */
#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif


/*
 * Local prototypes
 */
static gn_error_t gncomm_native_gethostbyname(const gn_uchar_t *name, gn_uint32_t *addr);
static gn_error_t gncomm_native_wait(gn_int32_t sock, gn_uint32_t timeout, gn_int32_t is_read);
static gn_error_t gncomm_map_native_error(int error);


/* blank stub on unix systems */
gn_error_t gncomm_native_initialize()
{
	gn_error_t	error = COMMERR_NoError;

	return GNERR_ERR_CODE(error);
}

/* blank stub on unix systems */
gn_error_t gncomm_native_shutdown()
{
	gn_error_t	error = COMMERR_NoError;

	return GNERR_ERR_CODE(error);
}


/* connect to a remote machine */
gn_error_t gncomm_native_connect(const gn_uchar_t *address, gn_int32_t port, connection_info_t *conn)
{
	gn_error_t		error = COMMERR_NoError;
	struct sockaddr_in	saddr;
	gn_int32_t		retval;

	/* First thing we need to do is resolve the name to an IP address */
	error = gncomm_native_gethostbyname(address, &saddr.sin_addr.s_addr);
	if(error == COMMERR_NoError)
	{
		/* Name resolved OK, continue filling out the address struct */
		saddr.sin_port = htons((short)port);   /* put into network order */
		saddr.sin_family = AF_INET;

		/* Try to get a socket to connect with */
		retval = socket(PF_INET, SOCK_STREAM, 0);
		if(retval != -1)
		{
			conn->native_handle = retval;

			/* Got a socket OK, now try to connect it */
			retval = connect(conn->native_handle, (struct sockaddr *)&saddr, sizeof(saddr));
			if (retval == -1)
			{
				error = gncomm_map_native_error(errno);

				/* if the connect fails we need to remember to close the
				 * socket
				 */
				close(conn->native_handle);
			}
		}
		else
		{
			error = gncomm_map_native_error(errno);
		}
	}
	
	return GNERR_ERR_CODE(error);
}

/* disconnect from a remote machine */								  
gn_error_t gncomm_native_disconnect(connection_info_t *conn)
{
	gn_error_t	error = COMMERR_NoError;
	gn_int32_t 	retval;

	/* do we need to do anything other than close the socket? */
	if(conn->native_handle != GNCOMM_INVALID_SOCKET)
	{
		retval = close(conn->native_handle);
		if(retval == -1)
		{
			error = gncomm_map_native_error(errno);
		}
		conn->native_handle = GNCOMM_INVALID_SOCKET;
	}
	else
	{
		error = COMMERR_InvalidHandle;
	}

	return GNERR_ERR_CODE(error);
}


/* send data to a remote machine */
gn_error_t gncomm_native_send(const gn_uchar_t *buffer, gn_int32_t send_size, gn_int32_t *size_sent, connection_info_t *conn)
{
	gn_error_t	error = COMMERR_NoError;
	gn_int32_t	sent;

#ifdef	GN_COMM_HAVE_SELECT
	/* If the platform supports it, do a select() on the socket to make sure
	 * it is ready to go.
	 */
	error = gncomm_native_wait(conn->native_handle, gncomm_get_transfer_timeout(), 0);
	if (error != COMMERR_NoError)
	{
		/* we timed out without getting anything */
		return GNERR_ERR_CODE(error);
	}
#endif	/* ifdef GN_COMM_HAVE_SELECT */    

	/* Attempt the send.  This will return the number of bytes received. */
	sent = send(conn->native_handle, buffer, send_size, MSG_NOSIGNAL);
	if(sent == -1)
	{
		error = gncomm_map_native_error(errno);
		sent = 0;
	}
	
	/* If the caller provided a pointer, store the number of bytes sent */
	if (size_sent != GN_NULL)
	{
		*size_sent = sent;
	}

	return GNERR_ERR_CODE(error);
}						  

/* receive data from  a remote machine */
gn_error_t gncomm_native_receive(gn_uchar_t *buffer, gn_int32_t receive_size, gn_int32_t *size_received, connection_info_t *conn)
{
	gn_error_t	error = COMMERR_NoError;
	gn_int32_t	received;
	
#ifdef	GN_COMM_HAVE_SELECT
	/* If the platform supports it, do a select() on the socket to make sure
	 * it is ready to go.
	 */
	error = gncomm_native_wait(conn->native_handle, gncomm_get_transfer_timeout(), 1);
	if(error != COMMERR_NoError)
	{
		/* we timed out without getting anything */
		return GNERR_ERR_CODE(error);
	}
#endif	/* ifdef GN_COMM_HAVE_SELECT */    
	
	/* Attempt the receive.  This will return the number of bytes received. */
	received = recv(conn->native_handle, buffer, receive_size, 0);
	if (received == -1)
	{
		error = gncomm_map_native_error(errno);
		received = 0;
	}

	/* If the caller provided a pointer, store the number of bytes received */
	if(size_received != NULL)
	{
		*size_received = received;
	}

	return GNERR_ERR_CODE(error);
}	 


/* Convert a name into ip addr */
static gn_error_t gncomm_native_gethostbyname(const gn_uchar_t *name, gn_uint32_t *addr)
{
	/* NOTE: gethostbyname() on unix will auto detect if you pass in a
	 * string representing an IP address (like "192.168.1.2") and just
	 * return that value. Your implementation here needs to detect
	 * this case also and do the right thing. (The Windows version of
	 * gethostbyname will not auto detect this)
	 */
    int a,b,c,d;
    if (sscanf(name, "%d.%d.%d.%d", &a,&b,&c,&d) == 4)
    {
        addr =
            ((a&0xff)<< 0) |
            ((b&0xff)<< 8) |
            ((c&0xff)<<16) |
            ((d&0xff)<<24);
    }
    else if (dns_resolve(name, addr) != 0)
	{
		/* NTD: We could map the 3 or 4 reasons this might fail into
		 * some more return values
		 */
        
		return GNERR_ERR_CODE(COMMERR_InvalidAddress);
	}

    return GNERR_ERR_CODE(COMMERR_NoError);
}


/* Perform a select() (or equivalent) on the provided socket, waiting for a 
 * send/recv/connect to become ready
 */
static gn_error_t gncomm_native_wait(gn_int32_t sock, gn_uint32_t timeout, gn_int32_t is_read)
{
	gn_uint32_t   elapsed;

	/* clear the total time waited */
	elapsed = 0;

	/* turn the timeout interval into milliseconds */
	timeout *= 1000;

	/* do the endless loop */
	while (1)
	{
		fd_set	fds;
		struct	timeval  tv;
		int	retval;

		/* setup to call select() */
		FD_ZERO(&fds);
		FD_SET(sock, &fds);

		/* We'll use a small enough wait time to allow a responsive CANCEL
		 * operation.  If a finer granularity is needed, change the value
		 * of SOCK_WAIT_INTERVAL and remember that it is specified in 
		 * milliseconds.
		 */
		tv.tv_sec = 0;
		tv.tv_usec = SOCK_WAIT_INTERVAL * 1000;

		/* Do the test.  Note that the param we use is based on whether we're 
		 * checking for readability (recv()) or writeability (connect(), send()).
		 */
		retval = is_read ? select(sock + 1, &fds, NULL, NULL, &tv) :
					select(sock + 1, NULL, &fds, NULL, &tv);

		if(retval == 0)
		{
			/* nothing ready yet - add the time we waited to the cumulative timeout */
			elapsed += SOCK_WAIT_INTERVAL;
			if (elapsed > timeout)
			{
				/* timeout exceeded - too bad, so sad */
				return GNERR_ERR_CODE(COMMERR_Timeout);
			}
			continue;
		}

		break;
	}

	/* if we got here, we have data waiting */
	return GNERR_ERR_CODE(COMMERR_NoError);
}


/* Turn a native error code => comm layer error code */
static gn_error_t gncomm_map_native_error(int error)
{
	switch(error)
	{
		case EBADF:		/* (BASEERR+9)		*/
		case EADDRINUSE:	/* (BASEERR+48)		*/
			return GNERR_ERR_CODE(COMMERR_InvalidHandle);

		case EINVAL:		/* (BASEERR+22)		*/
		case ENOTSOCK:		/* (BASEERR+38)		*/
/*		case EFAULT:		/* (BASEERR+14)		*/
			return GNERR_ERR_CODE(COMMERR_InvalidArg);

		case ENOMEM:
		case ENFILE:
		case ENOBUFS:		/* (BASEERR+55)		*/
			return GNERR_ERR_CODE(COMMERR_NoMemory);

		case ECONNREFUSED:	/* (BASEERR+61)		*/
		case ENETUNREACH:	/* (BASEERR+51)		*/
		case EINTR:		/* (BASEERR+4)		*/
		case EIO:
		case EMSGSIZE:		/* (BASEERR+40)		*/
		case EPIPE:
		case ENOTCONN:		/* (BASEERR+57)		*/
			return GNERR_ERR_CODE(COMMERR_IOError);

		case EINPROGRESS:	/* (BASEERR+36)		*/
		case EALREADY:		/* (BASEERR+37)		*/
			return GNERR_ERR_CODE(COMMERR_Busy);

		case EPERM:
		case EACCES:		/* (BASEERR+13)		*/
		case EPROTOTYPE:	/* (BASEERR+41)		*/
		case ENOPROTOOPT:	/* (BASEERR+42)		*/
		case EPROTONOSUPPORT:	/* (BASEERR+43)		*/
		case ESOCKTNOSUPPORT:	/* (BASEERR+44)		*/
		case EOPNOTSUPP:	/* (BASEERR+45)		*/
		case EPFNOSUPPORT:	/* (BASEERR+46)		*/
		case EAFNOSUPPORT:	/* (BASEERR+47)		*/
			return GNERR_ERR_CODE(COMMERR_Unsupported);

		case EMFILE:		/* (BASEERR+24)		*/
		case EAGAIN:
			return GNERR_ERR_CODE(COMMERR_NoMoreConnections);

		case ETIMEDOUT:		/* (BASEERR+60)		*/
/*		case TRY_AGAIN: 	/* (BASEERR+1002)	*/
			return GNERR_ERR_CODE(COMMERR_Timeout);

		case EDESTADDRREQ:	/* (BASEERR+39)		*/
		case EADDRNOTAVAIL:	/* (BASEERR+49)		*/
		case ENETDOWN:		/* (BASEERR+50)		*/
		case ENETRESET:		/* (BASEERR+52)		*/
		case ECONNABORTED:	/* (BASEERR+53)		*/
		case ECONNRESET:	/* (BASEERR+54)		*/
		case EISCONN:		/* (BASEERR+56)		*/
		case ESHUTDOWN:		/* (BASEERR+58)		*/
		case ETOOMANYREFS:	/* (BASEERR+59)		*/
		case ELOOP:		/* (BASEERR+62)		*/
		case ENAMETOOLONG:	/* (BASEERR+63)		*/
		case EHOSTDOWN:		/* (BASEERR+64)		*/
		case EHOSTUNREACH:	/* (BASEERR+65)		*/
/*		case ENOTEMPTY:		/* (BASEERR+66)		*/
/*		case EPROCLIM:		*//* (BASEERR+67)		*/
/*		case EUSERS:		/* (BASEERR+68)		*/
/*		case EDQUOT:		/* (BASEERR+69)		*/
		case ESTALE:		/* (BASEERR+70)		*/
/*		case EREMOTE:		/* (BASEERR+71)		*/
/*		case EDISCON:		*//* (BASEERR+101)	*/
/*		case SYSNOTREADY:	*//* (BASEERR+91)		*/
/*		case VERNOTSUPPORTED:	*//* (BASEERR+92)		*/
/*		case NO_RECOVERY:	/* (BASEERR+1003)	*/
/*		case NO_DATA:		*//* (BASEERR+1004)	*/	/*NO_ADDRESS*/
		default:
			/* dunno what this is */
			return GNERR_ERR_CODE(COMMERR_IckyError);
	}
}
