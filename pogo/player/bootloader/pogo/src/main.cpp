/*********************************************************

Pogo Bootloader
temancl@iobjects.com
01/05/02

Notes:
Kernel-free application
No threads, No interrupts
Loads, verifies, runs image from flash
Eventual fallback to restore image
Key combos for different images in flash

**********************************************************/

#include <bootloader/main/boot_mem.h>
#include <bootloader/main/boot_types.h>
#include <bootloader/main/boot_flash.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_if.h>
#include <cyg/hal/hal_cache.h>
#include CYGHWR_MEMORY_LAYOUT_H

#include <cyg/hal/hal_tables.h>
#ifdef CYGBLD_HAL_PLATFORM_STUB_H
#include CYGBLD_HAL_PLATFORM_STUB_H
#else
#include <cyg/hal/plf_stub.h>
#endif

// disk and flash managers
CBootFlash* pFlash;
extern void boot_init(void);

extern "C" {

#include "kbd.h"

// linker hack
void abort()
{
    // diag_printf("Abort!\n");
}

bool loadimage(const char *imgname)
{

	// copy default image from flash to hd
	FlashInfo *currflash;	
	typedef void code_fun(void);
    unsigned long entry;   
	unsigned long oldints;
    code_fun *fun;

	currflash = pFlash->GetFirstImage();
  
	while(currflash != NULL) 
	{
		DEBUG_BOOTLOADER("flash entry: %s\n",currflash->szName);
		if(strcmp(currflash->szName,imgname) == 0)		
			break;

		currflash = pFlash->GetNextImage();
	}

	if(currflash != NULL)
	{		
		unsigned long written;
		if(CBootMemory::UnGzip((void *)currflash->ulBase,currflash->ulSize,(void*)0x20000,&written))
		{
			DEBUG_BOOTLOADER("Decompression Error\n");
			return false;
		}

		if(written == 0)
		{
			DEBUG_BOOTLOADER("Decompression Error\n");
			return false;
		}

	}
	else
	{
		DEBUG_BOOTLOADER("Image not Found - uh oh!\n");
		return false;
	
	}
		


	DEBUG_BOOTLOADER("Executing image\n");

	entry = 0x20040;
	fun = (code_fun *)entry;


    HAL_DISABLE_INTERRUPTS(oldints);
    
	#ifdef HAL_ARCH_PROGRAM_NEW_STACK
		HAL_ARCH_PROGRAM_NEW_STACK(fun);
	#else
		(*fun)();
	#endif
	
}


void cyg_start(void)
{ 

  // unsigned long ulStartTime,ulOldSize,ulNewSize,ulTestSize,ulFinalSize;
  // void *pOldBase,*pNewBase,*pTestBase,*pFinalBase;
  unsigned long ulSize;
  

  boot_init();

  
  DEBUG_BOOTLOADER("PoGo Bootloader v0.2 - (c)fullplay\n\n\n");	
  
    
  // disk and flash managers
  pFlash = new CBootFlash();

  kbd_scan();
  // check kbd, force restore

  // jog button, held during startup, forces restore
  if(__kbd_col_state[0] & 0x2)
  {
	  DEBUG_BOOTLOADER("Restore Placeholder Goes Here\n");  
  }

  if(!loadimage("app"))
  {
	  DEBUG_BOOTLOADER("Error loading image, Load Failsafe here\n");  
  }


}
};
