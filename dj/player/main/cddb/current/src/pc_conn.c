/*
 * Copyright (c) 2000, 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 * pc_conn.c - Implementation of the gn_transporter interface for PC 
 * connectivity. This code only does real work under Windows. On other
 * platforms, it compiles, but does nothing.
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_transporter.h>
#include <extras/cddb/gn_registrar.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_utils.h>
#include GN_STRING_H
#include <extras/cddb/gn_fs.h>

#ifdef WIN32
#include <winsock.h>
#else
/* on non-Windows platforms, fake what we need */
typedef unsigned int SOCKET;
#define INVALID_SOCKET ((SOCKET)~0)
#endif


/*
 * Data Types
 */

typedef struct gn_pc_transporter_t {
	gn_transporter_t	base;
	int					initialized;
	SOCKET				socket_handle;
	gn_str_t			pc_address;
	unsigned short		pc_port;
} gn_pc_transporter_t;

/*
 * Private Function Declarations
 */

/* The following three functions comprise the interface between the Gracenote 
 * device code and the customer device code, at least for PC connectivity.
 */

static void
pc_transporter_destructor(gn_transporter_t* transporter);

static gn_error_t
pc_transporter_transmit(gn_transporter_t* transporter,
                        gn_cstr_t send_buf, gn_size_t send_len,
                        gn_str_t* receive_buf, gn_size_t* receive_len);

static gn_error_t
pc_transporter_download(gn_transporter_t* transporter,
                        gn_cstr_t url, gn_cstr_t* local_filename);

/* The following two functions satisfy the requirements of the existing 
 * Gracenote online lookup and update code. Customers should not have to 
 * modify them, since device registration is performed automatically by 
 * the DLL on the PC.
 */
static void
pc_registrar_destructor(gn_registrar_t* registrar);

static gn_error_t
pc_obtain_reg_data(gn_registrar_t* registrar, gn_device_reg_t* reg_data);

#ifdef WIN32
/* The declarations in this WIN32 block comprise a sample implementation 
 * of a communication mechanism between the device and the PC. While real 
 * devices will likely use a mechanism like USB or a wireless protocol,
 * to keep things simple and reasonably portable for this demonstration, 
 * we will use a TCP/IP socket implementation. Customers should replace 
 * this code with something appropriate for their device.
 */
int
initialize(gn_pc_transporter_t* pct);

static int
resolve_name(const char* name, struct sockaddr_in* addr);

static int
socket_wait(SOCKET sock, unsigned long timeout_secs, int is_read);

static int
connect_to_pc(gn_pc_transporter_t* pct);

static int
disconnect_from_pc(gn_pc_transporter_t* pct);

static int
send_to_pc(SOCKET socket_handle, const char* data, size_t len);

static int
receive_from_pc(SOCKET socket_handle, char* buf, int* len);

static int
send_cmd_to_pc(SOCKET socket_handle, char cmd, gn_size_t data_len);
#endif

/*
 * Public Functions
 */

gn_transporter_t*
gn_create_pc_transporter(gn_cstr_t address, unsigned short port)
{
	gn_pc_transporter_t* pct = (gn_pc_transporter_t*)gnmem_malloc(sizeof(gn_pc_transporter_t));

	if (pct == 0)
		return 0;

	pct->base.size = sizeof(gn_pc_transporter_t);
	pct->base.type = PC_TRANSPORTER;
	pct->base.destroy = pc_transporter_destructor;
	pct->base.transmit = pc_transporter_transmit;
	pct->base.download = pc_transporter_download;

	pct->initialized = 0;
	pct->socket_handle = INVALID_SOCKET;
	pct->pc_address = gn_strdup(address);
	pct->pc_port = port;

	return (gn_transporter_t*)pct;
}

gn_registrar_t*
gn_create_pc_registrar(void)
{
	gn_registrar_t* r = (gn_registrar_t*)gnmem_malloc(sizeof(gn_registrar_t));

	if (r == 0)
		return 0;

	r->size = sizeof(gn_registrar_t);
	r->type = PC_REGISTRAR;
	r->destroy = pc_registrar_destructor;
	r->obtain_reg_data = pc_obtain_reg_data;

	return r;
}

/*
 * Private Implementations
 */

static void
pc_transporter_destructor(gn_transporter_t* transporter)
{
	if (transporter == 0) {
		return;
	}

	if (transporter->type == PC_TRANSPORTER)
	{
		gn_pc_transporter_t* pct = (gn_pc_transporter_t*)transporter;

#ifdef WIN32
		if (pct->socket_handle != INVALID_SOCKET)
			disconnect_from_pc(pct);

		if (pct->initialized)
			WSACleanup();
#endif

		gnmem_free((void*)pct->pc_address);
	}

	gnmem_free((void*)transporter);
}

static gn_error_t
pc_transporter_transmit(gn_transporter_t* transporter,
                        gn_cstr_t send_buf, gn_size_t send_len,
                        gn_str_t* receive_buf, gn_size_t* receive_len)
{
	gn_error_t error = 0;

	if (transporter == 0 || send_buf == 0 || receive_buf == 0 || receive_len == 0) {
		/* if this code were part of a package, we'd use it here. */
		return GENERR_InvalidArg;
	}

	*receive_buf = 0;
	*receive_len = 0;

#ifdef WIN32
	if (transporter->type == PC_TRANSPORTER)
	{
		gn_pc_transporter_t* pct = (gn_pc_transporter_t*)transporter;

		error = initialize(pct);

		if (error == 0)
			error = connect_to_pc(pct);

		if (error == 0)
			error = send_cmd_to_pc(pct->socket_handle, 'm', send_len);

		if (error == 0)
			error = send_to_pc(pct->socket_handle, send_buf, send_len);

		while (error == 0)
		{
			char buf[1024];
			int data_len = sizeof(buf);

			error = receive_from_pc(pct->socket_handle, buf, &data_len);

			if (error == 0)
			{
				if (data_len > 0)
					error = gn_copy_to_expandable_buffer(buf, data_len, receive_buf, receive_len);
				else
					break;
			}
		}

		disconnect_from_pc(pct);
	}

	return error;
#else
	return -1;
#endif
}

#if !defined(GN_READONLY)
static gn_error_t
pc_transporter_download(gn_transporter_t* transporter,
                        gn_cstr_t url, gn_cstr_t* local_filename)
{
#ifdef WIN32
	gn_error_t error = 0;

	if (transporter == 0 || url == 0 || local_filename == 0) {
		/* if this code were part of a package, we'd use it here. */
		return GENERR_InvalidArg;
	}

	*local_filename = 0;

	if (transporter->type == PC_TRANSPORTER)
	{
		gn_pc_transporter_t* pct = (gn_pc_transporter_t*)transporter;

		error = initialize(pct);

		if (error == 0)
			error = connect_to_pc(pct);

		if (error == 0)
			error = send_cmd_to_pc(pct->socket_handle, 'd', strlen(url));

		if (error == 0)
			error = send_to_pc(pct->socket_handle, url, strlen(url));

		if (error == 0)
			error = create_unique_filename(get_filename_from_url(url), local_filename);

		if (error == 0)
		{
			gn_handle_t f = gnfs_create(*local_filename, FSMODE_WriteOnly, FSATTR_ReadWrite);

			if (f != FS_INVALID_HANDLE)
			{
				while (error == 0)
				{
					char buf[1024];
					int data_len = sizeof(buf);

					error = receive_from_pc(pct->socket_handle, buf, &data_len);

					if (error == 0)
					{
						if (data_len > 0)
							gnfs_write(f, buf, data_len);
						else
							break;
					}
				}

				gnfs_close(f);
			}
		}

		disconnect_from_pc(pct);
	}

	if (error != 0)
		gn_smart_free((void**)local_filename);

	return error;
#else
	return -1;
#endif
}
#else /* #if !defined(GN_READONLY) */
static gn_error_t
pc_transporter_download(gn_transporter_t* transporter,
                        gn_cstr_t url, gn_cstr_t* local_filename)
{
	return -1;
}
#endif /* #if !defined(GN_READONLY) */

static void
pc_registrar_destructor(gn_registrar_t* registrar)
{
	if (registrar != 0) {
		gnmem_free((void*)registrar);
	}
}

static gn_error_t
pc_obtain_reg_data(gn_registrar_t* registrar, gn_device_reg_t* reg_data)
{
	strcpy(reg_data->id, "unregistered");
	strcpy(reg_data->tag, "unregistered");

	return 0;
}

#ifdef WIN32
static int
initialize(gn_pc_transporter_t* pct)
{
	int error = 0;

	if (pct == 0) {
		/* if this code were part of a package, we'd use it here. */
		return GENERR_InvalidArg;
	}

	if (!pct->initialized)
	{
		WSADATA wsa_data;

		error = WSAStartup(MAKEWORD(1, 1), &wsa_data);

		if (error == 0)
			pct->initialized = 1;
	}

	return error;
}

static int
resolve_name(const char* name, struct sockaddr_in* addr)
{
	struct hostent* he = 0;

	/* assume name is a dotted quad, and attempt to convert it into a 32 bit value */
	addr->sin_addr.s_addr = inet_addr(name);

	/* if that failed, or it is the broadcast address, we are done */
	if (addr->sin_addr.s_addr != INADDR_NONE || strcmp(name, "255.255.255.255") == 0)
		return 0;

	/* it must be a real name, so look it up */    
	he = gethostbyname(name);

	addr->sin_addr.s_addr = (he != 0)
	                      ? *(u_long*)he->h_addr
	                      : INADDR_NONE;

	/* if we failed the lookup, we have an invalid address */
	return (addr->sin_addr.s_addr == INADDR_NONE) ? WSAGetLastError() : 0;
}

static int
socket_wait(SOCKET sock, unsigned long timeout_secs, int is_read)
{
	fd_set fds;
	struct timeval tv;
	int retval;

	FD_ZERO(&fds);
	FD_SET(sock, &fds);

	tv.tv_sec = timeout_secs;
	tv.tv_usec = 0;

	retval = is_read
		   ? select(sock + 1, &fds, 0, 0, &tv)
		   : select(sock + 1, 0, &fds, 0, &tv);

	if (retval > 0)
		return 0;					/* socket is ready */
	else if (retval == 0)
		return WSAETIMEDOUT;		/* we timed out */
	else
		return WSAGetLastError();	/* other error */
}

static int
connect_to_pc(gn_pc_transporter_t* pct)
{
	struct sockaddr_in saddr = { 0 };
	int retval = 0;
	
	if (pct == 0) {
		/* if this code were part of a package, we'd use it here. */
		return GENERR_InvalidArg;
	}

	retval = resolve_name(pct->pc_address, &saddr);
	
	if (retval == 0)
	{
		saddr.sin_port = htons(pct->pc_port);
		saddr.sin_family = AF_INET;
		
		pct->socket_handle = socket(PF_INET, SOCK_STREAM, 0);
		
		if (pct->socket_handle != INVALID_SOCKET)
		{
			retval = connect(pct->socket_handle, (struct sockaddr*)&saddr, sizeof(saddr));
			
			if (retval != 0)
			{
				retval = WSAGetLastError();

				closesocket(pct->socket_handle);
				pct->socket_handle = INVALID_SOCKET;
			}
		}
		else
		{
			retval = WSAGetLastError();
		}
	}
	
	return retval;
}

static int
disconnect_from_pc(gn_pc_transporter_t* pct)
{
	int error = 0;
	
	if (pct == 0) {
		/* if this code were part of a package, we'd use it here. */
		return GENERR_InvalidArg;
	}

	error = closesocket(pct->socket_handle);

	pct->socket_handle = INVALID_SOCKET;

	return error;
}

#define TIMEOUT_SECS 300

static int
send_to_pc(SOCKET socket_handle, const char* data, size_t len)
{
	int retval = socket_wait(socket_handle, TIMEOUT_SECS, 0);
	
	if (retval == 0)
	{
		int sent = send(socket_handle, data, len, 0);
		
		if (sent < 0)
		{
			retval = WSAGetLastError();
			sent = 0;
		}
	}
	
	return retval;
}

static int
receive_from_pc(SOCKET socket_handle, char* buf, int* len)
{
	int retval = socket_wait(socket_handle, TIMEOUT_SECS, 1);
	
	if (retval == 0)
	{
		*len = recv(socket_handle, buf, *len, 0);

		if (*len < 0)
			retval = WSAGetLastError();
	}
	
	return retval;
}

/* Device messages are of the form "ab:c", where a is a single character
 * command code, b is the number of bytes of data after the colon, expressed
 * as a decimal ASCII number, and c is the data itself. The currently 
 * defined messages are 'm', meaning a message from the device to the PC,
 * and 'd', meaning a file should be downloaded. The data for an 'm' command
 * is a message to be passed the the Gracenote PC library. The library will
 * return more data which should be the reply to the 'm' command. The data
 * for a 'd' command is a pathname for a file on the PC. The PC should send
 * the contents of this file to the device as the reply to the 'd' command.
 *
 * Don't forget that this simple protocol is just for illustrative purposes
 * only. Gracenote customers are free to implement any protocol they wish
 * for communication between the device and the PC, since you own both ends
 * of the connection.
 */
static int
send_cmd_to_pc(SOCKET socket_handle, char cmd, gn_size_t data_len)
{
	char cmd_str[16];

	sprintf(cmd_str, "%c%u:", cmd, data_len);

	return send_to_pc(socket_handle, cmd_str, strlen(cmd_str));
}
#endif
