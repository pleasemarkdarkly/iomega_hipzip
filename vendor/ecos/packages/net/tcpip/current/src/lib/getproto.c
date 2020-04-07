//==========================================================================
//
//      lib/getproto.c
//
//      getprotobyname(), getprotobynumber()
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
// Copyright (C) 1998, 1999, 2000 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//####BSDCOPYRIGHTBEGIN####
//
// -------------------------------------------
//
// Portions of this software may have been derived from OpenBSD or other sources,
// and are covered by the appropriate copyright disclaimers included herein.
//
// -------------------------------------------
//
//####BSDCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas
// Date:         2000-01-10
// Purpose:      
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <sys/param.h>
#include <netdb.h>

static struct protoent protocols[] = {
    { "ip",         0}, // internet protocol, pseudo protocol number
    { "icmp",       1}, // internet control message protocol
    { "igmp",       2}, // Internet Group Management
    { "ggp",        3}, // gateway-gateway protocol
    { "ipencap",    4}, // IP encapsulated in IP (officially ``IP'')
    { "st",         5}, // ST datagram mode
    { "tcp",        6}, // transmission control protocol
    { "egp",        8}, // exterior gateway protocol
    { "pup",       12}, // PARC universal packet protocol
    { "udp",       17}, // user datagram protocol
    { "hmp",       20}, // host monitoring protocol
    { "xns-idp",   22}, // Xerox NS IDP
    { "rdp",       27}, // "reliable datagram" protocol
    { "iso-tp4",   29}, // ISO Transport Protocol class 4
    { "xtp",       36}, // Xpress Tranfer Protocol
    { "ddp",       37}, // Datagram Delivery Protocol
    { "idpr-cmtp", 39}, // IDPR Control Message Transport
    { "rspf",      73}, //Radio Shortest Path First.
    { "vmtp",      81}, // Versatile Message Transport
    { "ospf",      89}, // Open Shortest Path First IGP
    { "ipip",      94}, // Yet Another IP encapsulation
    { "encap",     98}, // Yet Another IP encapsulation
    { 0, 0}
};

struct protoent *
getprotobyname(const char *name)
{
    struct protoent *p = protocols;
    while (p->p_name) {
        if (strcmp(name, p->p_name) == 0) {
            return p;
        }
        p++;
    }
    errno = ENOENT;
    return (struct protoent *)0;
}

struct protoent *
getprotobynumber(const int num)
{
    struct protoent *p = protocols;
    while (p->p_name) {
        if (p->p_proto == num) {
            return p;
        }
        p++;
    }
    errno = ENOENT;
    return (struct protoent *)0;
}
