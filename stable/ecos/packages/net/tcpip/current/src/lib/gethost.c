//==========================================================================
//
//      lib/gethost.c
//
//      gethostbyname(), gethostbyaddr()
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
#include <dns.h>
#include <stdlib.h>    // malloc, free
#include <pkgconf/net.h>

struct hostent *
gethostbyname(const char *host)
{
#if (CYGPKG_NET_DNSCLIENT==1)
	int len = strlen( host );
	struct hostent* p;

	if( host == NULL ) {
		return (struct hostent*) NULL;
	}

	p = (struct hostent*)malloc( sizeof( struct hostent ) );

	if( p == NULL )
	{
		return p;
	}
	p->h_aliases = 0;
	p->h_addrtype = 0;  // probably needs a better value

	p->h_name = (char*)malloc( len+1 );
	if( p->h_name == NULL )
	{
		free( p );
		return NULL;
	}
	memcpy( p->h_name, host, len+1 );
	
	p->h_addr_list = (char**) malloc( sizeof(char*) * 2 );
	if( p->h_addr_list == NULL )
	{
		free( p->h_name ); free( p );
		return NULL;
	}

	p->h_addr_list[1] = 0;

	// ASSUMPTION: ipv4 only
	p->h_length = 4;
	p->h_addr = (char*)malloc( 4 );
	if( p->h_addr == NULL )
	{
		free( p->h_name ); free( p->h_addr_list ); free( p );
		return NULL;
	}

	if( dns_resolve( p->h_name, (struct in_addr*)p->h_addr ) != 0 ) {
		endhostent( p );
		p = NULL;
	}

	return p;
#else
    return (struct hostent *)NULL;
#endif
}

struct hostent *
gethostbyaddr(const char *addr, int len, int type)
{
    return (struct hostent *)NULL;
}

void
endhostent( struct hostent* host )
{
	if( host ) {
		free( host->h_name );
		free( host->h_addr );
		free( host->h_addr_list );
		free( host );
	}
}
