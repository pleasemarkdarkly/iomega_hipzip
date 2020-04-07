//==========================================================================
//
//      net_config.c
//
//      RedBoot - Network config store support
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
// Author(s):    
// Contributors: 
// Date:         
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>

#ifdef CYGIMP_REDBOOT_CONFIG_STORE_NET
#include <config.h>

bool
cs_init(void)
{
    return have_net;
}

void
cs_write_config(struct _config *config)
{
    struct sockaddr_in host;
    int res, err;

    memset((char *)&host, 0, sizeof(host));
    host.sin_len = sizeof(host);
    host.sin_family = AF_INET;
    host.sin_addr = my_bootp_info.bp_siaddr;
    host.sin_port = 0;

    res = tftp_put(my_bootp_info.bp_file, &host, (char *)config, sizeof(*config), TFTP_OCTET, &err);
    if (res < 0) {
	printf("Can't write '%s': %s\n", my_bootp_info.bp_file, tftp_error(err));
	return;
    }
}

void
cs_load_config(struct _config *config)
{
    struct sockaddr_in host;
    int res, err;
    
    memset((char *)&host, 0, sizeof(host));
    host.sin_len = sizeof(host);
    host.sin_family = AF_INET;
    host.sin_addr = my_bootp_info.bp_siaddr;
    host.sin_port = 0;

    res = tftp_get(my_bootp_info.bp_file, &host, (char *)config, sizeof(*config), TFTP_OCTET, &err);
    if (res < 0) {
	printf("Can't load '%s': %s\n", my_bootp_info.bp_file, tftp_error(err));
	return;
    }
}

#endif
