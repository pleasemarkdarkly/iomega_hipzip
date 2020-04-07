/************************************************

Bootloader test application
temancl@iobjects.com
10/16/01

Notes:
Links against a redboot tree

************************************************/

// #include <bootloader/main/bootloader.h>
#include <bootloader/main/boot_mem.h>
#include <bootloader/main/boot_types.h>
#include <bootloader/main/boot_ide.h>
#include <bootloader/main/boot_flash.h>


#include "InstallerScreen.h"

extern unsigned long get_ms_ticks();
extern void boot_init(void);
extern void cyg_thread_delay(unsigned int);
static CShow g_show;

extern "C" { 

  void abort()
  {
    diag_printf("Abort!\n");
  }

void initui(void)
{

  CInstallerScreen is(&g_show);
  // set up ui

  cyg_thread_delay(1000);
  diag_printf("add sequence\n");
  is.ShowScreen(START_SEQUENCE,0,"");
  is.Draw();
  cyg_thread_delay(3000);
  is.ShowScreen(START_SEQUENCE,1,"");
  cyg_thread_delay(3000);
  is.ShowScreen(START_SEQUENCE,2,"");
  // FIXME: testing
  
  


  while(1);

}


void cyg_start(void)
{ 

  // unsigned long ulStartTime,ulOldSize,ulNewSize,ulTestSize,ulFinalSize;
  // void *pOldBase,*pNewBase,*pTestBase,*pFinalBase;
  unsigned long ulSize;

  boot_init();
  DEBUG_BOOTLOADER("Dadio Bootloader v0.1 - Interactive Objects\n\n\n");

  initui();
 
  CBootMemory mem((void*)0x1000000,(void*)0x100000);
  CBootDisk disk;
  CBootFlash flash;

  DSTAT* curr;
  FlashInfo *currflash;

  curr = disk.GetFirstImage();

  DEBUG_BOOTLOADER("Valid image list\n");
  while(curr != NULL) {
    DEBUG_BOOTLOADER("%s.%s\n",curr->fname,curr->fext);
    curr = disk.GetNextImage();
  }

  currflash = flash.GetFirstImage();
  
  while(currflash != NULL) {
    DEBUG_BOOTLOADER("flash entry: %s\n",currflash->szName);
    currflash = flash.GetNextImage();
  }
  
  // decompress from disk

  disk.ReadFile("A:\\TEST.DSZ",mem.FreeBlock(),&ulSize);
  mem.NotifyImage(ulSize);

  DEBUG_BOOTLOADER("Verifying file");
  mem.Verify();

  DEBUG_BOOTLOADER("uncompressing file\n");
  mem.Decompress();

  disk.DeleteFile("A:\\TESTGZ.BIN");
  disk.WriteFile("A:\\TESTGZ.BIN",mem.GetBlock(mem.BlockCount()-1),mem.GetSize(mem.BlockCount()-1));
  
  disk.ReadFile("A:\\TEST.BIN",mem.FreeBlock(),&ulSize);
  mem.NotifyImage(ulSize);

  // compare the previous 2 images
  if(memcmp(mem.GetBlock(mem.BlockCount()-1),
	    mem.GetBlock(mem.BlockCount()-2),
	    mem.GetSize(mem.BlockCount()-1)) == 0) {
    DEBUG_BOOTLOADER("compare succeeded\n");
  }
  else {
    DEBUG_BOOTLOADER("COMPARE FAILED\n");

  }
  
  
#if 0

  // get SFV'd gzip'd file
  if(!boot_ide_read("A:\\TEST.DSZ",pOldBase,&ulOldSize)) {
    DEBUG_BOOTLOADER("Read of test.dgz failed");
    while(1);
  }
  
  DEBUG_BOOTLOADER("test.dsz is %u bytes\nDecrypting Image..\n",ulOldSize);

  pNewBase = (void*)((unsigned long)pOldBase + ulOldSize);

  // pad for alignment
  pNewBase = (void*)((unsigned long)pNewBase + ((unsigned long)pNewBase % 4));
  
  boot_mem_unsfv(pOldBase, ulOldSize, pNewBase, &ulNewSize);

  DEBUG_BOOTLOADER("decrypt complete\nstart gunzip",(get_ms_ticks() - ulStartTime));

  pFinalBase = (void*)((unsigned long)pNewBase + ulNewSize);

  // pad for alignment
  pFinalBase = (void*)((unsigned long)pFinalBase + ((unsigned long)pFinalBase % 4));

  boot_mem_ungz(pNewBase,ulNewSize,pFinalBase,&ulFinalSize);

  DEBUG_BOOTLOADER("gunzip complete\n");
 
  boot_ide_delete("A:\\TESTGZ.BIN");  
  boot_ide_write("A:\\TESTGZ.BIN",pFinalBase,ulFinalSize);

  // get sfv'd file

  pTestBase = (void *)((unsigned long)pFinalBase + ulFinalSize); 
  pTestBase = (void *)((unsigned long)pTestBase + ((unsigned long)pTestBase % 4));

  boot_ide_read("A:\\TEST.BIN",pTestBase,&ulTestSize);

  if(ulTestSize != ulFinalSize)
    {
      DEBUG_BOOTLOADER("sizes don't match - %u != %u\n",ulTestSize,ulFinalSize);
    }
  
  if(memcmp(pFinalBase,pTestBase,ulTestSize) == 0)
    {
      DEBUG_BOOTLOADER("compare succeeds\n");
    }
#endif


}

};
