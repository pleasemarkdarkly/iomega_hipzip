/* Copyright (c) 2000 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/* comm_native.h
 *
 * Interface to platform-specific routines for the communications layer
 *
 */

#ifndef _COMM_NATIVE_H_
#define _COMM_NATIVE_H_

#include <extras/cddb/toolkit/commmgr/comm_core.h>

#define GNCOMM_INVALID_SOCKET	(-1)

/*
 * Prototypes
 */

/* Do any initialization or cleanup that may be required by the platform */
gn_error_t gncomm_native_initialize(void);
gn_error_t gncomm_native_shutdown(void);

/* connect to a remote machine */
gn_error_t gncomm_native_connect(const gn_uchar_t *address, gn_int32_t port, connection_info_t *conn);

/* disconnect from a remote machine */								  
gn_error_t gncomm_native_disconnect(connection_info_t *conn);

/* send data to a remote machine */
gn_error_t gncomm_native_send(const gn_uchar_t *buffer, gn_int32_t send_size, gn_int32_t *size_sent, connection_info_t *conn);

/* receive data from  a remote machine */
gn_error_t gncomm_native_receive(gn_uchar_t *buffer, gn_int32_t receive_size, gn_int32_t *size_received, connection_info_t *conn);


#endif	/* ifndef _COMM_NATIVE_H_ */
