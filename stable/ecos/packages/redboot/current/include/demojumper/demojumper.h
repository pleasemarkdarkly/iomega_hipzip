//........................................................................................
//........................................................................................
//.. File Name: demojumper.h															..
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
#ifndef DEMOJUMPER_H
#define DEMOJUMPER_H

//#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_edb7xxx.h>
//#include "keyboard/include/kbd_support.h"
#include "demojumper/demos.h"		// enumerated demos to jump into.  defines prompt text, location.
#include "lcd.h"

#define INTERTRUST_SUPPORT
//#undef INTERTRUST_SUPPORT

#define BITS_PER_PIXEL   LCD_BITS_PER_PIXEL
#define PIXEL_WIDTH     LCD_WIDTH
#define PIXEL_HEIGHT    LCD_HEIGHT
#define BYTE_HEIGHT	( PIXEL_HEIGHT/8 )
#define BYTE_WIDTH 	( PIXEL_WIDTH )
#define DISPLAY_BUF_SIZE ( (PIXEL_HEIGHT) * (PIXEL_WIDTH) / (8 / BITS_PER_PIXEL))
#define INVERT_BUF

/* font placement for menu */
#define MENU_ITEM_VSPACE 6				// how much vert space between menu items
#define MENU_ITEM_HEIGHT 17				// how much each menu item takes up vertically
#define MENU_BORDER_WIDTH 10			// how much space to leave for icons around client menu area
#define MENU_ITEM_WIDTH (LCD_WIDTH - 2*MENU_BORDER_WIDTH)
#define MENU_ARROW_XPOS	(MENU_BORDER_WIDTH + MENU_ITEM_WIDTH - 5)
#define MENU_ARROW_BORDER_WIDTH 2		// how much space between arrow tip and screen edge
#define MENU_ITEM_MAX_CHARS 25
#define MENU_ITEM_COUNT ((int) ((LCD_HEIGHT-(2*MENU_BORDER_WIDTH)) / (MENU_ITEM_HEIGHT+MENU_ITEM_VSPACE)) )		// how many menu items per screen

/* border location for menu */
#define MENU_BORDER_EDGE_DIST 3
#define MENU_BORDER_LEFT (MENU_BORDER_EDGE_DIST-1)
#define MENU_BORDER_RIGHT (LCD_WIDTH-MENU_BORDER_EDGE_DIST-1)
#define MENU_BORDER_TOP (MENU_BORDER_EDGE_DIST-1)
#define MENU_BORDER_BOTTOM (LCD_HEIGHT-MENU_BORDER_EDGE_DIST-1)
	
extern cyg_uint8 ucBuf[DISPLAY_BUF_SIZE];

/* firmware module */
typedef struct metadata_s 
{
    unsigned int crc;
    unsigned int len;
    unsigned short version_major;
    unsigned short version_minor;
} metadata_t;

//........................................................................................
//.. factory_test																		..
//.. Purpose: A factory hardware test													..
//.. Expected Parameters: none															..
//.. Return Value(s): none																..
//.. Implemented in file: factory_test.c, Used in: demojumper.c							..
//........................................................................................
void factory_test(void);

//........................................................................................
//.. select_demo																	..
//.. Purpose: Starts GUI used during upgrade selection 									..
//.. Expected Parameters: A pressed key combination, (return value of keys_down()) funct..
//.. Return Value(s): none																..
//.. Implemented in file: GumiScreen.cpp, used in demojumper.c							..
//........................................................................................
void select_demo(void);
void demojumper_init(void);

//........................................................................................
//.. notify_upgrade_progress															..
//.. Purpose: Controls a progress bar used during firmware upgrades						..
//.. Expected Parameters: One of the key values defined below, affects progress display ..
//.. Return Value(s): none																..
//.. Implemented in file: GumiScreen.cpp, used in firmware.c							..
//........................................................................................

/* key values */
#if 0
#define DOWNLOADING_INTO_RAM 1
#define VERIFYING_RAM        2
#define PROGRAMMING_FLASH    3
#define VERIFYING_FLASH      4

void notify_upgrade_progress(int key);

//........................................................................................
//.. kbd_in_demojumper_mode																..
//.. Purpose: Checks to see if user wants to jump into the demojumper 					..
//.. Expected Parameters: A pressed key combination, (return value of keys_down()) funct..
//.. Return Value(s): true/false	true=demojumper button pressed, false=not pressed	..
//.. Implemented in file: demojumper.c, used in GumiScreen.cpp							..
//........................................................................................
bool kbd_in_demojumper_mode(cyg_uint8 keys_down);


//........................................................................................
//.. get_firmware_image_name															..
//.. Purpose: enumerates through images on clik disk 									..
//.. Expected Parameters: none															..
//.. Return Value(s): returns 0 after all images have been found						..
//.. Implemented in file: firmware.c, used in GumiScreen.cpp and ui.c					..
//........................................................................................
char * get_firmware_image_name(void); 

//........................................................................................
//.. get_firmware_image_metadata														..
//.. Purpose: Places firmware metadata in the metadata_t structure						..
//.. Expected Parameters: image name and metatdata_t structure							..
//.. Return Value(s):true/false	true=success, false=failure								..
//.. Implemented in file: firmware.c, used in GumiScreen.cpp							..
//........................................................................................
bool get_firmware_image_metadata(char * image_name, metadata_t * md);

//........................................................................................
//.. get_firmware_metadata																..
//.. Purpose: get metadata about firmware currently on device							..
//.. Expected Parameters: a pointer to metadata_t structure								..
//.. Return Value(s): none																..
//.. Implemented in file: firmware.c, used in demojumper.c, GumiScreen.cpp, firmware.c	..
//........................................................................................
void get_firmware_metadata(metadata_t ** md);

//........................................................................................
//.. firmware_valid																		..
//.. Purpose: Simple firmware validation		 										..
//.. Expected Parameters: none															..
//.. Return Value(s): true/false true=valid, false=invalid								..
//.. Implemented in file: firmware.c, not used											..
//........................................................................................
bool firmware_valid(void);  
#endif

#endif /* DEMOJUMPER_H */
