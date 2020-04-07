//........................................................................................
//........................................................................................
//.. File Name: demojumper.c															..
//.. Date: 7/17/2000																	..
//.. Author(s): Todd Malsbary															..
//.. Description of content: Iomega demojumper declarations								..
//.. Usage: The demojumper manages firmware upgrades and the player, and is separate	..
//..		from the player. It is a mini-player so to speak.					 		..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/14/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>
//#include <cyg/io/io.h>
#include "demojumper/ddo_system.h"
#include "demojumper/demojumper.h"
#include "demojumper/debug.h"
#include "demojumper/kbd_server.h"
#include <redboot.h>
#define DISABLE_KBD 0
#define DO_FACTORY_TEST 1
#define SELECT_FIRMWARE 1
#define LOAD_FIRMWARE 1
#define RESET_DEVICE 1
#define PRINT_VSR_TABLE 0

#define KBD_TOP_BUTTONS_PRESSED  0x0f
#define KBD_SIDE_BUTTONS_PRESSED 0xe0
#define KBD_DEMOJUMPER_BUTTONS_PRESSED 0x03 /* play and stop at the same time */

// warning: this fn has to be tailored to match info in demos.h!
char			demo_names[DEMO_COUNT][DEMO_DESC_LEN];
char			demo_aliases[DEMO_COUNT][DEMO_ALIAS_LEN];
unsigned long	demo_flash_bases[DEMO_COUNT];
unsigned long	demo_lens[DEMO_COUNT];
void setup_demos() {
	// localize demo-count sensitive code so elsewhere we get to loop
	strcpy (demo_names[0],DEMO0_TITLE);
	strcpy (demo_aliases[0],DEMO0_ALIAS);
	demo_flash_bases[0] = DEMO0_FLASH;
	demo_lens[0] = DEMO0_LEN;

	strcpy (demo_names[1],DEMO1_TITLE);
	strcpy (demo_aliases[1],DEMO1_ALIAS);
	demo_flash_bases[1] = DEMO1_FLASH;
	demo_lens[1] = DEMO1_LEN;

	strcpy (demo_names[2],DEMO2_TITLE);
	strcpy (demo_aliases[2],DEMO2_ALIAS);
	demo_flash_bases[2] = DEMO2_FLASH;
	demo_lens[2] = DEMO2_LEN;
	
#if DEMO_COUNT > 3

	strcpy (demo_names[3],DEMO3_TITLE);
	strcpy (demo_aliases[3],DEMO3_ALIAS);
	demo_flash_bases[3] = DEMO3_FLASH;
	demo_lens[3] = DEMO3_LEN;
	
	strcpy (demo_names[4],DEMO4_TITLE);
	strcpy (demo_aliases[4],DEMO4_ALIAS);
	demo_flash_bases[4] = DEMO4_FLASH;
	demo_lens[4] = DEMO4_LEN;

	strcpy (demo_names[5],DEMO5_TITLE);
	strcpy (demo_aliases[5],DEMO5_ALIAS);
	demo_flash_bases[5] = DEMO5_FLASH;
	demo_lens[5] = DEMO5_LEN;

	strcpy (demo_names[6],DEMO5_TITLE);
	strcpy (demo_aliases[6],DEMO5_ALIAS);
	demo_flash_bases[6] = DEMO5_FLASH;
	demo_lens[6] = DEMO5_LEN;
#endif
}

void
demojumper_init(void)
{
    char * build_date = __DATE__;
    char * build_time = __TIME__;
    //metadata_t * md;
//    cyg_uint8 keys_down;
  
    /* display identification info */
//    get_firmware_metadata(&md);

    SHUNT_PRINT("DADIO demojumper : %s, %s\n", build_time, build_date);
//    SHUNT_PRINT("DADIO %d.%d\n", md->version_major, md->version_minor);
    SHUNT_PRINT("Authors : Todd Malsbary <toddm@iobjects.com>\n");
    SHUNT_PRINT("          Dan Bolstad <danb@iobjects.com>\n");
    SHUNT_PRINT("          Eric Gibbs <ericg@iobjects.com>\n");
    SHUNT_PRINT("\n");
  
	select_demo();
}

#if 0
static cyg_uint8
read_kbd(void)
{
    cyg_uint8 port_data;
  
    /* scan keyboard */
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) | SYSCON1_KBD_LOW;
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) | SYSCON1_KBD_COL(0);
    kbd_delay();
    port_data = *(volatile cyg_uint8 *)PADR;
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) | SYSCON1_KBD_TRISTATE;
    kbd_delay();
    *(volatile cyg_uint32 *)SYSCON1 = (*(volatile cyg_uint32 *)SYSCON1 & ~SYSCON1_KBD_CTL) | SYSCON1_KBD_HIGH;

    return port_data;
}


static bool
kbd_in_factory_mode(cyg_uint8 keys_down)
{
    /* check buttons pressed for factory combination */
    if ((keys_down & KBD_TOP_BUTTONS_PRESSED) == KBD_TOP_BUTTONS_PRESSED ||
	(keys_down & KBD_SIDE_BUTTONS_PRESSED) == KBD_SIDE_BUTTONS_PRESSED) {
	return true;
    }
    else {
	return false;
    }
}

bool
kbd_in_demojumper_mode(cyg_uint8 keys_down)
{
    /* check buttons pressed for demojumper combination */
    if ((keys_down & KBD_DEMOJUMPER_BUTTONS_PRESSED) == KBD_DEMOJUMPER_BUTTONS_PRESSED) {
	return true;
    }
    else {
	return false;
    }
}
#endif

#if 0
static void
kbd_delay(void)
{
    volatile int i;
  
    for (i = 0;  i < 250;  i++)
	;
}
#endif

#if 0
static void
usb_callback(int key)
{
    SHUNT_PRINT("usb callback %d\n", key);
}

void
load_demo(unsigned int demo_index)
{
    cyg_uint32 * exception_handler = (cyg_uint32 *)demo_start + 8;	/* VSR table starts at 0x20 */
    cyg_uint32 * new_reset_vsr = (cyg_uint32 *)demo_start + 16;     /* reset_vector */
    int vsr_num;

#if PRINT_VSR_TABLE /* this is only for debugging, enabling this will break the demojumper */
    /* print out current VSR table */
    SHUNT_PRINT("Current VSR table:\n");
    for (vsr_num = CYGNUM_HAL_VSR_MIN; vsr_num < CYGNUM_HAL_VSR_COUNT; ++vsr_num) {
	SHUNT_PRINT("VSR (0x%x): 0x%x\n", vsr_num, hal_vsr_table[vsr_num]);
    }
#endif

    /* install new VSR table */
    HAL_VSR_SET(CYGNUM_HAL_VECTOR_RESET, new_reset_vsr, NULL);
    for (vsr_num = CYGNUM_HAL_VSR_MIN + 1; vsr_num < CYGNUM_HAL_VSR_COUNT; ++vsr_num) {
	HAL_VSR_SET(vsr_num, exception_handler[vsr_num], NULL);
    }

#if PRINT_VSR_TABLE /* this is only for debugging, enabling this will break the demojumper */
    SHUNT_PRINT("New VSR table:\n");
    for (vsr_num = CYGNUM_HAL_VSR_MIN; vsr_num < CYGNUM_HAL_VSR_COUNT; ++vsr_num) {
	SHUNT_PRINT("VSR (0x%x): 0x%x\n", vsr_num, hal_vsr_table[vsr_num]);
    }
#endif

}
#endif // if 0 cutting usb_callback & load_demo

RedBoot_init(demojumper_init, RedBoot_INIT_PRIO(11));
