//==========================================================================
//
//      config.c
//
//      RedBoot - Configuration data support
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
// Date:         2000-07-28
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>

#ifdef CYGSEM_REDBOOT_CONFIG
RedBoot_cmd("config",
            "Manage configuration kept in persistent memory",
            "[-l]",
            do_config
    );
#endif

#ifdef CYGSEM_REDBOOT_CONFIG
#include <config.h>

static struct _config config;
static bool config_ok;

#define CONFIG_KEY1    0x0BADFACE
#define CONFIG_KEY2    0xDEADDEAD

#define CONFIG_DONE    0
#define CONFIG_ABORT  -1
#define CONFIG_CHANGED 1
#define CONFIG_OK      2
#define CONFIG_BACK    3
#define CONFIG_BAD     4

// Note: the following options are related.  If 'bootp' is false, then
// the other values are used in the configuration.  Because of the way
// that configuration tables are generated, they should have names which
// are related.  The configuration options will show up lexicographically
// ordered, thus the peculiar naming.
RedBoot_config_option("Run script at boot",
                      boot_script,
                      ALWAYS_ENABLED, true,
                      CONFIG_BOOL,
                      false
    );
RedBoot_config_option("Boot script",
                      boot_script_data,
                      "boot_script", true,
                      CONFIG_SCRIPT,
                      ""
    );
// Some preprocessor magic for building the [constant] prompt string
#define __cat(s1,c2,s3) s1 ## #c2 ## s3
#define _cat(s1,c2,s3) __cat(s1,c2,s3)
RedBoot_config_option(_cat("Boot script timeout (",
                           CYGNUM_REDBOOT_SCRIPT_TIMEOUT_RESOLUTION,
                           "ms resolution)"),
                      boot_script_timeout,
                      "boot_script", true,
                      CONFIG_INT,
                      0
    );
#undef __cat
#undef _cat

CYG_HAL_TABLE_BEGIN( __CONFIG_options_TAB__, RedBoot_config_options);
CYG_HAL_TABLE_END( __CONFIG_options_TAB_END__, RedBoot_config_options);

extern struct config_option __CONFIG_options_TAB__[], __CONFIG_options_TAB_END__[];

// 
// Layout of config data
// Each data item is variable length, with the name, type and dependencies
// encoded into the object.
//  offset   contents
//       0   data type
//       1   length of name (N)
//       2   enable sense
//       3   length of enable key (M)
//       4   key name
//     N+4   enable key
//   M+N+4   data value
//

#define CONFIG_OBJECT_TYPE(dp)          (dp)[0]
#define CONFIG_OBJECT_KEYLEN(dp)        (dp)[1]
#define CONFIG_OBJECT_ENABLE_SENSE(dp)  (dp)[2]
#define CONFIG_OBJECT_ENABLE_KEYLEN(dp) (dp)[3]
#define CONFIG_OBJECT_KEY(dp)           ((dp)+4)
#define CONFIG_OBJECT_ENABLE_KEY(dp)    ((dp)+4+CONFIG_OBJECT_KEYLEN(dp))
#define CONFIG_OBJECT_VALUE(dp)         ((dp)+4+CONFIG_OBJECT_KEYLEN(dp)+CONFIG_OBJECT_ENABLE_KEYLEN(dp))

static int
get_config(unsigned char *dp, char *title, bool list_only)
{
    char line[256], *sp, *lp;
    int ret;
    bool hold_bool_val, new_bool_val, enable;
    unsigned long hold_int_val, new_int_val;
#ifdef CYGPKG_REDBOOT_NETWORKING
    in_addr_t hold_ip_val, new_ip_val;
    enet_addr_t hold_esa_val;
    int esa_ptr;
    char *esp;
#endif
    void *val_ptr;
    int type;

    if (CONFIG_OBJECT_ENABLE_KEYLEN(dp)) {
        config_get(CONFIG_OBJECT_ENABLE_KEY(dp), &enable, CONFIG_BOOL);
        if (((bool)CONFIG_OBJECT_ENABLE_SENSE(dp) && !enable) ||
            (!(bool)CONFIG_OBJECT_ENABLE_SENSE(dp) && enable)) {
            return CONFIG_OK;  // Disabled field
        }
    }
    val_ptr = (void *)CONFIG_OBJECT_VALUE(dp);
    if (title != (char *)NULL) {
        printf("%s: ", title);
    } else {
        printf("%s: ", CONFIG_OBJECT_KEY(dp));
    }
    switch (type = CONFIG_OBJECT_TYPE(dp)) {
    case CONFIG_BOOL:
        memcpy(&hold_bool_val, val_ptr, sizeof(bool));
        printf("%s ", hold_bool_val ? "true" : "false");
        break;
    case CONFIG_INT:
        memcpy(&hold_int_val, val_ptr, sizeof(unsigned long));
        printf("%ld ", hold_int_val);
        break;
#ifdef CYGPKG_REDBOOT_NETWORKING
    case CONFIG_IP:
        printf("%s ", inet_ntoa((in_addr_t *)val_ptr));
        break;
    case CONFIG_ESA:
        for (esa_ptr = 0;  esa_ptr < sizeof(enet_addr_t);  esa_ptr++) {
            printf("0x%02X", ((unsigned char *)val_ptr)[esa_ptr]);
            if (esa_ptr < (sizeof(enet_addr_t)-1)) printf(":");
        }
        printf(" ");
        break;
#endif
    case CONFIG_STRING:
        printf("??");
        return CONFIG_OK;  // FIXME - skip for now
    case CONFIG_SCRIPT:
        printf("\n");
        sp = lp = (unsigned char *)val_ptr;
        while (*sp) {
            while (*lp != '\n') lp++;
            *lp = '\0';
            printf(".. %s\n", sp);
            *lp++ = '\n';
            sp = lp;
        }
        break;
    }
    if (list_only) {
        printf("\n");
        return CONFIG_OK;
    }
    if (type != CONFIG_SCRIPT) {
        ret = gets(line, sizeof(line), 0);    
        if (ret < 0) return CONFIG_ABORT;
        if (strlen(line) == 0) return CONFIG_OK;  // Just a CR - leave value untouched
        if (line[0] == '.') return CONFIG_DONE;
        if (line[0] == '^') return CONFIG_BACK;
    }
    switch (type) {
    case CONFIG_BOOL:
        memcpy(&hold_bool_val, val_ptr, sizeof(bool));
        if (!parse_bool(line, &new_bool_val)) {
            return CONFIG_BAD;
        }
        if (hold_bool_val != new_bool_val) {
            memcpy(val_ptr, &new_bool_val, sizeof(bool));
            return CONFIG_CHANGED;
        } else {
            return CONFIG_OK;
        }
        break;
    case CONFIG_INT:
        memcpy(&hold_int_val, val_ptr, sizeof(unsigned long));
        if (!parse_num(line, &new_int_val, 0, 0)) {
            return CONFIG_BAD;
        }
        if (hold_int_val != new_int_val) {
            memcpy(val_ptr, &new_int_val, sizeof(unsigned long));
            return CONFIG_CHANGED;
        } else {
            return CONFIG_OK;
        }
        break;
#ifdef CYGPKG_REDBOOT_NETWORKING
    case CONFIG_IP:
        memcpy(&hold_ip_val.s_addr, &((in_addr_t *)val_ptr)->s_addr, sizeof(in_addr_t));
        if (!inet_aton(line, &new_ip_val)) {
            return CONFIG_BAD;
        }
        if (hold_ip_val.s_addr != new_ip_val.s_addr) {
            memcpy(val_ptr, &new_ip_val, sizeof(in_addr_t));
            return CONFIG_CHANGED;
        } else {
            return CONFIG_OK;
        }
        break;
    case CONFIG_ESA:
        memcpy(&hold_esa_val, val_ptr, sizeof(enet_addr_t));
        esp = line;
        for (esa_ptr = 0;  esa_ptr < sizeof(enet_addr_t);  esa_ptr++) {
            unsigned long esa_byte;
            if (!parse_num(esp, &esa_byte, &esp, ":")) {
                memcpy(val_ptr, &hold_esa_val, sizeof(enet_addr_t));
                return CONFIG_BAD;
            }
            ((unsigned char *)val_ptr)[esa_ptr] = esa_byte;
        }
        return CONFIG_CHANGED;
        break;
#endif
    case CONFIG_SCRIPT:
        // Assume it always changes
        sp = (unsigned char *)val_ptr;
        printf("Enter script, terminate with empty line\n");
        while (true) {
            *sp = '\0';
            printf(">> ");
            ret = gets(line, sizeof(line), 0);
            if (ret < 0) return CONFIG_ABORT;
            if (strlen(line) == 0) break;
            lp = line;
            while (*lp) {
                *sp++ = *lp++;
            }
            *sp++ = '\n';
        }
        break;
    case CONFIG_STRING:
        printf("??");
    }
    return CONFIG_CHANGED;
}

//
// Manage configuration information
//

static int
config_length(int type)
{
    switch (type) {
    case CONFIG_BOOL:
        return sizeof(bool);
    case CONFIG_INT:
        return sizeof(unsigned long);
#ifdef CYGPKG_REDBOOT_NETWORKING
    case CONFIG_IP:
        return sizeof(in_addr_t);
    case CONFIG_ESA:
        // Would like this to be sizeof(enet_addr_t), but that causes much
        // pain since it fouls the alignment of data which follows.
        return 8;
#endif
    case CONFIG_STRING:
        return 0;
    case CONFIG_SCRIPT:
        return MAX_SCRIPT_LENGTH;
    }
}

void
do_config(int argc, char *argv[])
{
    bool need_update = false;
    struct config_option *optend = __CONFIG_options_TAB_END__;
    struct config_option *opt = __CONFIG_options_TAB__;
    struct _config hold_config;
    struct option_info opts[1];
    bool list_only;
    unsigned char *dp;
    int len, ret;
    char *title;

    if (!cs_init()) return;
    memcpy(&hold_config, &config, sizeof(config));
    script = (unsigned char *)0;

    init_opts(&opts[0], 'l', false, OPTION_ARG_TYPE_FLG, 
              (void **)&list_only, (bool *)0, "list configuration only");
    if (!scan_opts(argc, argv, 1, opts, 1, 0, 0, ""))
    {
        return;
    }

    dp = &config.config_data[0];
    while (dp < &config.config_data[sizeof(config.config_data)]) {
        if (CONFIG_OBJECT_TYPE(dp) == CONFIG_EMPTY) {
            break;
        }
        len = 4 + CONFIG_OBJECT_KEYLEN(dp) + CONFIG_OBJECT_ENABLE_KEYLEN(dp) + 
            config_length(CONFIG_OBJECT_TYPE(dp));
        // Provide a title for well known [i.e. builtin] objects
        title = (char *)NULL;
        opt = __CONFIG_options_TAB__;
        while (opt != optend) {
            if (strcmp(opt->key, CONFIG_OBJECT_KEY(dp)) == 0) {
                title = opt->title;
                break;
            }
            opt++;
        }
        ret = get_config(dp, title, list_only);
        switch (ret) {
        case CONFIG_DONE:
            goto done;
        case CONFIG_ABORT:
            memcpy(&config, &hold_config, sizeof(config));
            return;
        case CONFIG_CHANGED:
            need_update = true;
        case CONFIG_OK:
            dp += len;
            break;
        case CONFIG_BACK:
            dp = &config.config_data[0];
            continue;
        case CONFIG_BAD:
            // Nothing - make him do it again
            printf ("** invalid entry\n");
        }
    }

 done:
    if (!need_update) return;
    config_write();
}

//
// Write the in-memory copy of the configuration data to the config store.
//
void
config_write(void)
{
    config.len = sizeof(config);
    config.key1 = CONFIG_KEY1;  
    config.key2 = CONFIG_KEY2;
    config.cksum = crc32((unsigned char *)&config, sizeof(config)-sizeof(config.cksum));
    if (verify_action("Update RedBoot non-volatile configuration")) {
	cs_write_config(&config);
    }
}

//
// Retrieve a data object from the data base (in memory copy)
//
bool
config_get(char *key, void *val, int type)
{
    unsigned char *dp;
    void *val_ptr;
    int len;

    if (!config_ok) return false;

    dp = &config.config_data[0];
    while (dp < &config.config_data[sizeof(config.config_data)]) {
        len = 4 + CONFIG_OBJECT_KEYLEN(dp) + CONFIG_OBJECT_ENABLE_KEYLEN(dp) +
            config_length(CONFIG_OBJECT_TYPE(dp));
        val_ptr = (void *)CONFIG_OBJECT_VALUE(dp);
        if (strcmp(key, CONFIG_OBJECT_KEY(dp)) == 0) {
            if (CONFIG_OBJECT_TYPE(dp) == type) {
                switch (type) {
                    // Note: the data may be unaligned in the configuration data
                case CONFIG_BOOL:
                    memcpy(val, val_ptr, sizeof(bool));
                    break;
                case CONFIG_INT:
                    memcpy(val, val_ptr, sizeof(unsigned long));
                    break;
#ifdef CYGPKG_REDBOOT_NETWORKING
                case CONFIG_IP:
                    memcpy(val, val_ptr, sizeof(in_addr_t));
                    break;
                case CONFIG_ESA:
                    memcpy(val, val_ptr, sizeof(enet_addr_t));
                    break;
#endif
                case CONFIG_STRING:
                    break;
                case CONFIG_SCRIPT:
                    // Just return a pointer to the script
                    *(unsigned char **)val = (unsigned char *)val_ptr;
                    break;
                }
            } else {
                printf("Request for config value '%s' - wrong type\n", key);
            }
            return true;
        }
        dp += len;
    }
    printf("Can't find config data for '%s'\n", key);
    return false;
}

//
// Add a new option to the database
//
bool
config_add(struct config_option *opt)
{
    unsigned char *dp, *kp;
    int len, elen, size;

    dp = &config.config_data[0];
    size = 0;
    while (size < sizeof(config.config_data)) {
        if (CONFIG_OBJECT_TYPE(dp) == CONFIG_EMPTY) {
            kp = opt->key;
            len = strlen(kp) + 1;
            size += len + 2 + 2 + config_length(opt->type);
            if (opt->enable) {
                elen = strlen(opt->enable) + 1;
                size += elen;
            } else {
                elen = 0;
            }
            if (size > sizeof(config.config_data)) {
                break;
            }
            CONFIG_OBJECT_TYPE(dp) = opt->type; 
            CONFIG_OBJECT_KEYLEN(dp) = len;
            CONFIG_OBJECT_ENABLE_SENSE(dp) = opt->enable_sense;
            CONFIG_OBJECT_ENABLE_KEYLEN(dp) = elen;
            dp = CONFIG_OBJECT_KEY(dp);
            while (*kp) *dp++ += *kp++;
            *dp++ = '\0';    
            if (elen) {
                kp = opt->enable;
                while (*kp) *dp++ += *kp++;
                *dp++ = '\0';    
            }
            switch (opt->type) {
                // Note: the data may be unaligned in the configuration data
            case CONFIG_BOOL:
                memcpy(dp, (void *)&opt->dflt, sizeof(bool));
                break;
            case CONFIG_INT:
                memcpy(dp, (void *)&opt->dflt, sizeof(unsigned long));
                break;
#ifdef CYGPKG_REDBOOT_NETWORKING
            case CONFIG_IP:
                memcpy(dp, (void *)&opt->dflt, sizeof(in_addr_t));
                break;
            case CONFIG_ESA:
                memcpy(dp, (void *)&opt->dflt, sizeof(enet_addr_t));
                break;
#endif
            case CONFIG_STRING:
            case CONFIG_SCRIPT:
                break;
            }
            dp += config_length(opt->type);
            return true;
        } else {
            len = 4 + CONFIG_OBJECT_KEYLEN(dp) + CONFIG_OBJECT_ENABLE_KEYLEN(dp) +
                config_length(CONFIG_OBJECT_TYPE(dp));
            dp += len;
            size += len;
        }
    }
    printf("No space to add '%s'\n", opt->key);
    return false;
}

//
// Reset/initialize configuration data - used only when starting from scratch
//
static void
config_init(void)
{
    // Well known option strings
    struct config_option *optend = __CONFIG_options_TAB_END__;
    struct config_option *opt = __CONFIG_options_TAB__;

    memset(&config, 0, sizeof(config));
    while (opt != optend) {
        if (!config_add(opt)) {
            return;
        }
        opt++;
    }
    config_ok = true;
}

//
// Attempt to get configuration information from the config store.
// If available (i.e. good checksum, etc), initialize "known"
// values for later use.
//
void
load_config(void)
{
    bool use_boot_script;

    config_ok = false;
    script = (unsigned char *)0;
    if (!cs_init()) return;
    cs_load_config(&config);
    if ((crc32((unsigned char *)&config, sizeof(config)-sizeof(config.cksum)) != config.cksum) ||
        (config.key1 != CONFIG_KEY1)|| (config.key2 != CONFIG_KEY2)) {
        printf("Configuration checksum error or invalid key\n");
        config_init();
        return;
    }        
    config_ok = true;
    config_get("boot_script", &use_boot_script, CONFIG_BOOL);
    if (use_boot_script) {
        config_get("boot_script_data", &script, CONFIG_SCRIPT);
        config_get("boot_script_timeout", &script_timeout, CONFIG_INT);
    }
}

RedBoot_init(load_config, RedBoot_INIT_FIRST);

#endif
