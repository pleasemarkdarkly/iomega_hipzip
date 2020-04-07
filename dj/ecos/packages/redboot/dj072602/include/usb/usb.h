//==========================================================================
//
//      usb.h
//
//      Stand-alone USB support for RedBoot
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000, 2001 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas
// Date:         2000-07-14
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef _USB_H_
#define _USB_H_

#include <pkgconf/redboot.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/basetype.h>
#include <cyg/io/usb/usbs.h>

extern bool usb_debug;

extern unsigned long do_ms_tick(void);
extern unsigned long get_ms_ticks(void);
#define MS_TICKS() get_ms_ticks()
#define MS_TICKS_DELAY() do_ms_tick()

typedef struct 
{
    cyg_uint32 DataTransferLength;
#define CMD_FLAGS_DIRECTION 0x01
    cyg_uint8 Flags;
} usb_cmd_t;

typedef struct 
{
    cyg_uint32 DataResidue;
#define STATUS_SUCCESS 0x01
#define STATUS_FAILURE 0x00
    cyg_uint8 Status;
} usb_stat_t;

/*
 * USB poll function.
 */
extern void __usb_poll(void);//usbs_control_endpoint *ep);

/*
 * Read up to 'len' bytes with blocking.
 * Returns number of bytes read.
 * If connection is closed, returns -1.
 */
extern int __usb_read(/*usbs_rx_endpoint *rx_endpoint,*/ char *buf, int len);

/*
 * Write up to 'len' bytes with blocking.
 * Returns number of bytes written.
 * If connection is closed, returns -1.
 */
extern int __usb_write(/*usbs_tx_endpoint *tx_endpoint,*/ char *buf, int len);

// Initialize the USB driver
extern void usb_init(void);

// Test for new USB I/O connections
extern void usb_io_test(void);

extern int usb_stream_open(char *filename, int *err);
extern int usb_stream_read(char *buf, int size, int *err);
extern int usb_stream_close(int *err);
extern char *usb_error(int err);

#endif // _USB_H_
