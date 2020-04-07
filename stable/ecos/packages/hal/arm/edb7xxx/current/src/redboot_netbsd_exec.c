//==========================================================================
//
//      redboot_netbsd_boot.c
//
//      RedBoot command to boot NetBSD on edb7xxx
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
// Date:         2001-02-20
// Purpose:      
// Description:  
//              
// This code is part of RedBoot (tm).
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <redboot.h>

#include <cyg/hal/hal_platform_setup.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>

#define HAL_PLATFORM_MACHINE_TYPE 0x70DD

// Exported CLI function(s)
static void do_exec(int argc, char *argv[]);
RedBoot_cmd("exec", 
            "Execute an image - with MMU off", 
            "[-w timeout] [-b <load addr> [-l <length>]]\n"
            "        [-r <ramdisk addr> [-s <ramdisk length>]]\n"
            "        [-c \"kernel command line\"] [<entry_point>]",
            do_exec
    );

typedef void code_fun(void);
#define UNMAPPED_ADDR(x) (((unsigned long)x & 0x0FFFFFFF) + DRAM_PA)

//
// Parameter info for NetBSD kernel
//
// TODO

static void 
do_exec(int argc, char *argv[])
{
    unsigned long entry;
    unsigned long oldints;
    code_fun *fun, *prg;
    bool wait_time_set;
    int  wait_time, res, i;
    bool base_addr_set, length_set, cmd_line_set;
    bool ramdisk_addr_set, ramdisk_size_set;
    unsigned long base_addr, length;
    unsigned long ramdisk_addr, ramdisk_size;
    struct option_info opts[6];
    char line[8];
    unsigned long *_prg, *ip;
    char *cmd_line;

    entry = (unsigned long)UNMAPPED_ADDR(0xC0100000);  // NetBSD execute address
    base_addr = 0x8000;
    ramdisk_size = 4096*1024;
    init_opts(&opts[0], 'w', true, OPTION_ARG_TYPE_NUM, 
              (void **)&wait_time, (bool *)&wait_time_set, "wait timeout");
    init_opts(&opts[1], 'b', true, OPTION_ARG_TYPE_NUM, 
              (void **)&base_addr, (bool *)&base_addr_set, "base address");
    init_opts(&opts[2], 'l', true, OPTION_ARG_TYPE_NUM, 
              (void **)&length, (bool *)&length_set, "length");
    init_opts(&opts[3], 'c', true, OPTION_ARG_TYPE_STR, 
              (void **)&cmd_line, (bool *)&cmd_line_set, "kernel command line");
    init_opts(&opts[4], 'r', true, OPTION_ARG_TYPE_NUM, 
              (void **)&ramdisk_addr, (bool *)&ramdisk_addr_set, "ramdisk_addr");
    init_opts(&opts[5], 's', true, OPTION_ARG_TYPE_NUM, 
              (void **)&ramdisk_size, (bool *)&ramdisk_size_set, "ramdisk_size");
    if (!scan_opts(argc, argv, 1, opts, 6, (void *)&entry, OPTION_ARG_TYPE_NUM, "[physical] starting address"))
    {
        return;
    }

    // Set up parameters to pass to kernel
    // TODO

    if (wait_time_set) {
        int script_timeout_ms = wait_time * 1000;
        unsigned char *hold_script = script;
        printf("About to start execution at %p - abort with ^C within %d seconds\n",
               (void *)entry, wait_time);
        script = (unsigned char *)0;
        while (script_timeout_ms >= CYGNUM_REDBOOT_CLI_IDLE_TIMEOUT) {
            res = gets(line, sizeof(line), CYGNUM_REDBOOT_CLI_IDLE_TIMEOUT);
            if (res == _GETS_CTRLC) {
                script = hold_script;  // Re-enable script
                return;
            }
            script_timeout_ms -= CYGNUM_REDBOOT_CLI_IDLE_TIMEOUT;
        }
    }
    if (base_addr_set && !length_set) {
      printf("Length required for non-standard base address\n");
      return;
    }
    base_addr = UNMAPPED_ADDR(base_addr);
    
    HAL_DISABLE_INTERRUPTS(oldints);
    HAL_DCACHE_SYNC();
    HAL_ICACHE_DISABLE();
    HAL_DCACHE_DISABLE();
    HAL_DCACHE_SYNC();
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_DCACHE_INVALIDATE_ALL();
    
    asm volatile ("mov r2,%0;"
		  "mov r3,%1;"
		  "ldr r6,=0xC0100000;" // Unmapped address NetBSD wants to load at
		  "mov r8,r6;"

		  "ldr	r5,=00f;"
		  "ldr	r7,=0xE0000000;" // ROM0_LA_START
		  "sub	r5,r5,r7;"
		  "ldr	r4,=0x070;"
		  "mcr	p15,0,r4,c1,c0;" // Turn off the MMU, caches
		  "mov	pc,r5;"    // Change address spaces
		  "nop;"
		  "nop;"
		  "nop;"
		  "00:;"

		  "cmp r2,r6;"        // Default kernel load address
                  "beq 10f;"
		  "05:;"
		  "ldr r4,[r2],#4;"
		  "str r4,[r6],#4;"
		  "sub r3,r3,#4;"
		  "cmp r3,#0;"
		  "bne 05b;"
		  "10:;"

		  "mov pc,r8;" : :
		  "r"(base_addr),"r"(length) :
		  "r7","r6","r5","r4","r3","r2");
}

// EOF redboot_netbsd_exec.c
