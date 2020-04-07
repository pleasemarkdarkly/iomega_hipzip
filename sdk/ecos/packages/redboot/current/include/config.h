//==========================================================================
//
//      config.h
//
//      Configuration data tables for RedBoot
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
// Contributors: gthomas, toddm
// Date:         2000-08-21
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_SCRIPT_LENGTH CYGNUM_REDBOOT_SCRIPT_SIZE
#define MAX_CONFIG_DATA   CYGNUM_REDBOOT_CONFIG_SIZE

#define CONFIG_EMPTY   0
#define CONFIG_BOOL    1
#define CONFIG_INT     2
#define CONFIG_STRING  3
#define CONFIG_SCRIPT  4
#ifdef CYGPKG_REDBOOT_NETWORKING
#define CONFIG_IP      5
#define CONFIG_ESA     6
#endif

struct config_option {
    char *key;
    char *title;
    char *enable;
    bool  enable_sense;
    int   type;
    unsigned long dflt;
} CYG_HAL_TABLE_TYPE;

#define ALWAYS_ENABLED (char *)0

#define RedBoot_config_option(_t_,_n_,_e_,_ie_,_type_,_dflt_)        \
struct config_option _config_option_##_n_                               \
CYG_HAL_TABLE_QUALIFIED_ENTRY(RedBoot_config_options,_n_) =             \
   {#_n_,_t_,_e_,_ie_,_type_,(unsigned long)_dflt_};

// Load the configuration data from persistent storage.  Will be done
// automatically via RedBoot_init.
void load_config(void);
// Cause the in-memory configuration data to be written to persistent storage
void config_write(void);
// Fetch a data item from persistent storage, returns 'false' if not found
bool config_get(char *key, void *val, int type);
// Add a new data item to configuration data base.  Returns 'false'
// if no space is available.
bool config_add(struct config_option *opt);

// Configuration data, saved in the config store, used to set/update RedBoot
// normal "configuration" data items.
struct _config {
    unsigned long len;
    unsigned long key1;
    unsigned char config_data[MAX_CONFIG_DATA-(4*4)];
    unsigned long key2;
    unsigned long cksum;
};

// Config store interface functions
extern bool cs_init(void);
extern void cs_write_config(struct _config *config);
extern void cs_load_config(struct _config *config);

#endif // _CONFIG_H_
