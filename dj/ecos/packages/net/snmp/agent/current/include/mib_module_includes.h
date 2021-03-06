//==========================================================================
//
//      ./agent/current/include/mib_module_includes.h
//
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
//####UCDSNMPCOPYRIGHTBEGIN####
//
// -------------------------------------------
//
// Portions of this software may have been derived from the UCD-SNMP
// project,  <http://ucd-snmp.ucdavis.edu/>  from the University of
// California at Davis, which was originally based on the Carnegie Mellon
// University SNMP implementation.  Portions of this software are therefore
// covered by the appropriate copyright disclaimers included herein.
//
// The release used was version 4.1.2 of May 2000.  "ucd-snmp-4.1.2"
// -------------------------------------------
//
//####UCDSNMPCOPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    hmt
// Contributors: hmt
// Date:         2000-05-30
// Purpose:      Port of UCD-SNMP distribution to eCos.
// Description:  
//              
//
//####DESCRIPTIONEND####
//
//==========================================================================
/********************************************************************
       Copyright 1989, 1991, 1992 by Carnegie Mellon University

			  Derivative Work -
Copyright 1996, 1998, 1999, 2000 The Regents of the University of California

			 All Rights Reserved

Permission to use, copy, modify and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appears in all copies and
that both that copyright notice and this permission notice appear in
supporting documentation, and that the name of CMU and The Regents of
the University of California not be used in advertising or publicity
pertaining to distribution of the software without specific written
permission.

CMU AND THE REGENTS OF THE UNIVERSITY OF CALIFORNIA DISCLAIM ALL
WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL CMU OR
THE REGENTS OF THE UNIVERSITY OF CALIFORNIA BE LIABLE FOR ANY SPECIAL,
INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
FROM THE LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*********************************************************************/
/* This file is automatically generated by configure.  Do not modify by hand. */
//#include "mibgroup/mibII.h" // no content
//#include "mibgroup/ucd_snmp.h"
//#include "mibgroup/snmpv3mibs.h"
#include "mibgroup/mibII/system_mib.h"  // { mib2 1 }
#include "mibgroup/mibII/sysORTable.h"  // { mib2 1.9.1 } == { system 9.1 }
#include "mibgroup/mibII/interfaces.h"  // { mib2 2 }
//#include "mibgroup/mibII/at.h"        // #3 is deprecated
#include "mibgroup/mibII/ip.h"          // { mib2 4 }
#include "mibgroup/mibII/icmp.h"        // { mib2 5 }
#include "mibgroup/mibII/tcp.h"         // { mib2 6 }
#include "mibgroup/mibII/udp.h"         // { mib2 7 }
#include "mibgroup/mibII/dot3.h"        // { mib2 10.7 } == { transmission 7 }
#include "mibgroup/mibII/snmp_mib.h"    // { mib2 11 }
//#include "mibgroup/mibII/vacm_vars.h"
//#include "mibgroup/ucd-snmp/memory.h"
//#include "mibgroup/ucd-snmp/vmstat.h"
//#include "mibgroup/ucd-snmp/proc.h"
//#include "mibgroup/ucd-snmp/versioninfo.h"
//#include "mibgroup/ucd-snmp/pass.h"
//#include "mibgroup/ucd-snmp/pass_persist.h"
//#include "mibgroup/ucd-snmp/disk.h"
//#include "mibgroup/ucd-snmp/loadave.h"
//#include "mibgroup/ucd-snmp/extensible.h"
//#include "mibgroup/ucd-snmp/errormib.h"
//#include "mibgroup/ucd-snmp/registry.h"
//#include "mibgroup/ucd-snmp/file.h"
//#include "mibgroup/snmpv3/snmpEngine.h"
//#include "mibgroup/snmpv3/snmpMPDStats.h"
//#include "mibgroup/snmpv3/usmStats.h"
//#include "mibgroup/snmpv3/usmUser.h"
//#include "mibgroup/util_funcs.h"
//#include "mibgroup/mibII/var_route.h"
//#include "mibgroup/mibII/route_write.h"
