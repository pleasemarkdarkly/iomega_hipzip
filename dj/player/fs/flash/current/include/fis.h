#ifndef _FIS_H_
#define _FIS_H_
//==========================================================================
//
//      fis.h
//
//      RedBoot - FLASH image directory layout
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
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas
// Date:         2000-07-28
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

// don't change this for compatibility with redboot
#define MAX_FLASH_NAME 16

struct fis_image_desc {
    unsigned char name[MAX_FLASH_NAME];      // Null terminated name
    unsigned long flash_base;    // Address within FLASH of image
    unsigned long mem_base;      // Address in memory where it executes
    unsigned long size;          // Length of image
    unsigned long entry_point;   // Execution entry point
    unsigned long data_length;   // Length of actual data
    unsigned char _pad[256-(MAX_FLASH_NAME+7*sizeof(unsigned long))];
    unsigned long desc_cksum;    // Checksum over image descriptor
    unsigned long file_cksum;    // Checksum over image data
};

struct fis_image_desc *fis_lookup(char *name);

#endif // _FIS_H_
