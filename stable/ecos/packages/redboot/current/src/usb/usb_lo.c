//==========================================================================
//
//      xyzModem.c
//
//      RedBoot stream handler for xyzModem protocol
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

#include <redboot.h>
#include <usb/usb.h>

int 
usb_stream_open(char *filename, int *err)
{
    return 0;
}

int 
usb_stream_read(char *buf, int len, int *err)
{
    usb_cmd_t cmd;
    usb_stat_t stat;
    int n;

    // Write command
    cmd.DataTransferLength = len;
    cmd.Flags = 0x00;
    n = __usb_write(&cmd, sizeof(cmd));
    
    // Read data
    n = __usb_read(buf, cmd.DataTransferLength);
    
    // Read status
    n = __usb_read(&stat, sizeof(stat));
    
    return len - stat.DataResidue;
}

int
usb_stream_close(int *err)
{
    return 0;
}

char *
usb_error(int err)
{
    switch (err) {
    default:
        return "Unknown error";
        break;
    }
}
