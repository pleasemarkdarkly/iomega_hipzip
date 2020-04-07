/////////////////////////////////////////////////////////////////
// File Name: bootloader.h
// Date: 10/17/01
// Author(s): Teman Clark-Lindh <temancl@iobjects.com>
// Description: Contains generic/universal definitions for Dadio boot loader
// Usage: Call boot_init() to initialize the serial console. 
//
// Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
// All rights reserved. This code may not be redistributed in source or linkable
// object form without the express written consent of Interactive Objects.      
//
// Contact Information: www.iobjects.com
//////////////////////////////////////////////////////////////////

#include <bootloader/main/bootloader.h>

// Global variables
#define EXTERN extern

unsigned char *ram_start, *ram_end;
unsigned char *user_ram_start, *user_ram_end;
unsigned char *workspace_start, *workspace_end;
unsigned long workspace_size;
unsigned long *entry_address;

char RedBoot_version[] CYGBLD_ATTRIB_WEAK = 
  "\nRedBoot(tm) bootstrap and debug environment - built " __TIME__ ", " __DATE__ "\n\n";

// Override default GDB stubs 'info'
// Note: this can still be a "weak" symbol since it will occur in the .o
// file explicitly mentioned in the link command.  User programs will 
// still be allowed to override it.
char GDB_stubs_version[] CYGBLD_ATTRIB_WEAK = 
    "eCos GDB stubs [via RedBoot] - built " __DATE__ " / " __TIME__;

// linking hacka hacka hacka - these were not really implemented anyway before
extern "C"
{
 int printf(const char *str,...)
{
  DEBUG_BOOTLOADER("printf: %s",str);
}


int get_ms_ticks()
{
  return 0;
}

};


void boot_init(void)
{
    int res = 0;
    bool prompt = true;
    int cur;
    struct init_tab_entry *init_entry;

    // Make sure the channels are properly initialized.
    hal_if_diag_init();

    // Force console to output raw text - but remember the old setting
    // so it can be restored if interaction with a debugger is
    // required.
    cur = CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_CALL_IF_SET_COMM_ID_QUERY_CURRENT);
    CYGACC_CALL_IF_SET_CONSOLE_COMM(CYGNUM_HAL_VIRTUAL_VECTOR_DEBUG_CHANNEL);
    CYGACC_CALL_IF_DELAY_US(2*100000);

    ram_start = (unsigned char *)CYGMEM_REGION_ram;
    ram_end = (unsigned char *)(CYGMEM_REGION_ram+CYGMEM_REGION_ram_SIZE);
#ifdef HAL_MEM_REAL_REGION_TOP
    {
        unsigned char *ram_end_tmp = ram_end;
        ram_end = HAL_MEM_REAL_REGION_TOP( ram_end_tmp );
    }
#endif
#ifdef CYGMEM_SECTION_heap1
    workspace_start = (unsigned char *)CYGMEM_SECTION_heap1;
    workspace_end = (unsigned char *)(CYGMEM_SECTION_heap1+CYGMEM_SECTION_heap1_SIZE);
    workspace_size = CYGMEM_SECTION_heap1_SIZE;
#else
    workspace_start = (unsigned char *)CYGMEM_REGION_ram;
    workspace_end = (unsigned char *)(CYGMEM_REGION_ram+CYGMEM_REGION_ram_SIZE);
    workspace_size = CYGMEM_REGION_ram_SIZE;
#endif

}

void boot_exec(unsigned long ulJumpAddr)
{

  DEBUG_BOOTLOADER("not yet..\n");

}
