//===========================================================================
//
//      abs.cxx
//
//      ISO C standard abs()/labs() utility functions 
//
//===========================================================================
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
//===========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    jlarmour
// Contributors: 
// Date:         2000-04-28
// Purpose:     
// Description:  Real alternative for inline implementation of the ISO
//               standard abs()/labs() utility functions defined in
//               section 7.10.6.1 of the standard
// Usage:       
//
//####DESCRIPTIONEND####
//
//===========================================================================

// We don't want the inline versions of abs/labs defined here

#define CYGPRI_LIBC_STDLIB_ABS_INLINE

/* This means that including abs.inl will make the outline functions */

#include <cyg/libc/stdlib/abs.inl>

// EOF abs.cxx