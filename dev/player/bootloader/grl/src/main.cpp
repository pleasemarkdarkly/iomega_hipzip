/*********************************************************

Gzip Run Loader (GRL)
temancl@iobjects.com
01/05/02

Notes:
Kernel-free application
No threads, No interrupts
Loads, verifies, runs image from flash
Eventual fallback to restore image
Support key combos for different images in flash

**********************************************************/

#include "main.h"
#include "images.h"

#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_diag.h>   // HAL_DELAY_US
#include <cyg/hal/hal_edb7xxx.h> // PBDR
#include <cyg/infra/diag.h>

#include CYGHWR_MEMORY_LAYOUT_H

#include <cyg/hal/hal_tables.h>


#ifdef CYGBLD_HAL_PLATFORM_STUB_H
#include CYGBLD_HAL_PLATFORM_STUB_H
#else
#include <cyg/hal/plf_stub.h>
#endif

#include <fs/flash/FlashManager.h>
#include <cyg/hal/hal_if.h>
#include <new.h>

// emergency restore
#include <fs/fat/sdapi.h>
#include <main/util/update/UpdateApp.h>
#include <devs/lcd/lcd.h>


unsigned char *ram_start, *ram_end;
unsigned char *user_ram_start, *user_ram_end;
unsigned char *workspace_start, *workspace_end;
unsigned long workspace_size;
unsigned long *entry_address;

// path for a bandaid image
#define EMERGENCY_FILE "a:/system/saferestore.img"

// Toss up a splash screen?
#define DISPLAY_SPLASH_SCREEN

typedef void code_fun(void);

extern "C"
{
    void boot_init(void);
    int UnGzip(void * pBase, unsigned long ulSize, void * pTarget, unsigned long * pulWritten);
    bool loadimage(const char* imgname);
    void main_loop(void);
    void abort();
    void cyg_start(void);
    
    int gzip_init(_pipe_t* p);
    int gzip_inflate(_pipe_t* p);
    int gzip_close(_pipe_t* p, int err);
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

#ifdef DISPLAY_SPLASH_SCREEN
    // Toss up a splash screen
    LCDEnable();
    LCDSetBacklight(1);
    LCDClear();
    LCDWriteRaw(ucFullplayBitmap, sizeof(ucFullplayBitmap));
#endif
    
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

    // make sure the workspace is after 8mb, so we dont ungzip shit on top of ourselves
    if( (unsigned int)workspace_start < 0x800000 ) {
        workspace_size -= ((unsigned char*)0x800000 - workspace_start);
        workspace_start = (unsigned char*)0x800000;
    }
    diag_printf("workspace at %p size %d\n", workspace_start, workspace_size);
#else
    workspace_start = (unsigned char *)CYGMEM_REGION_ram;
    workspace_end = (unsigned char *)(CYGMEM_REGION_ram+CYGMEM_REGION_ram_SIZE);
    workspace_size = CYGMEM_REGION_ram_SIZE;
#endif

}



int UnGzip(void * pBase, unsigned long ulSize, void * pTarget, unsigned long * pulWritten)
{

    int err;

    _pipe_t pipe;
 
    // setup pipe
    pipe.in_buf = (unsigned char *) pBase;
    pipe.in_avail = ulSize;
    pipe.out_buf = (unsigned char *) pTarget;
    pipe.out_size = 0;
    pipe.msg = 0;

    // init gzip
    err = gzip_init(&pipe);

    diag_printf("ungzip:base = %x, size = %d, target = %x\n",pBase,ulSize,pTarget);
 
    // decompress file to target location and close gzip
    if( err ) {
        diag_printf("init    - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);
    }

    err = gzip_inflate(&pipe);

    if( err ) {
        diag_printf("inflate - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);
    }

    *pulWritten = pipe.out_size;

    err = gzip_close(&pipe,err);

    if( err ) {
        diag_printf("close   - err = %d %s, size = %d\n",err,pipe.msg,pipe.out_size);
    }

    return err;

}


static CFlashManager* flash;
static CUpdateApp* update;

static char flash_pool[sizeof(CFlashManager)] __attribute__((aligned(4)));
static char update_pool[sizeof(CUpdateApp)]   __attribute__((aligned(4)));

bool loadimage(const char *imgname)
{
	struct fis_image_desc* img;
	unsigned long written;
	code_fun* fun;

	CFlashManager* pflash = flash;

	
	diag_printf("looking for image %s\n", imgname);
	// try and find image
	if((img = pflash->FindImage(imgname)) != NULL)
	{
		diag_printf("image found\n");

		if(UnGzip((void *)img->flash_base,img->data_length,(void*)img->mem_base,&written))
		{
			diag_printf("Decompression Error\n");
			return false;
		}
		
		if(written == 0)
		{
			diag_printf("Decompression Error\n");
			return false;
		}

	
	}
	else
	{
		diag_printf("Image not Found\n");
		return false;
	
	}
		
	diag_printf("Executing image at 0x%x\n",img->entry_point);
	fun = (code_fun *)img->entry_point;

	// disable interrupts, flush cache
	unsigned long oldints;
	HAL_DISABLE_INTERRUPTS(oldints);
	HAL_UCACHE_DISABLE();
	HAL_UCACHE_INVALIDATE_ALL();
    
#ifdef HAL_ARCH_PROGRAM_NEW_STACK
    HAL_ARCH_PROGRAM_NEW_STACK(fun);
#else
    (*fun)();
#endif

    // never executes, but makes the compiler happy
    return true;
}

void main_loop(void)
{
    // unsigned long ulStartTime,ulOldSize,ulNewSize,ulTestSize,ulFinalSize;
    // void *pOldBase,*pNewBase,*pTestBase,*pFinalBase;
    unsigned long ulSize;
  
    boot_init();
    update = new(update_pool) CUpdateApp;
    flash = new(flash_pool) CFlashManager;

    diag_printf("GRL Bootloader v0.2 - (c)fullplay\n\n\n");	
      
    if(!loadimage("app"))
    {
        CUpdateApp* pUpdate = update;

        // app wasn't available, so now things are messy. try and find a .img file on the hard drive
        // first get some UI going on here so the user understands why we're going to take a bit to start
        LCDEnable();
        LCDSetBacklight(1);
        LCDClear();
        LCDWriteRaw(ucFirmwareCorruptBitmap, sizeof(ucFirmwareCorruptBitmap));
        
        diag_printf("failed to load app, trying to load emergency image %s\n", EMERGENCY_FILE);
        pc_system_init(0);
        PCFD fd = po_open(EMERGENCY_FILE, PO_RDONLY, PS_IREAD);
        if( fd < 0 ) {
            diag_printf("emergency file not found, halting\n");
            abort();
        }

        // determine file length
        short err;
        int len = po_lseek( fd, 0, PSEEK_END, &err );
        po_lseek( fd, 0, PSEEK_SET, &err );

        // load the image into an arbitrary point in ram - say 6mb out
        unsigned char* buf = (unsigned char*) 0x600000;

        if( (unsigned)len != po_read( fd, buf, len ) ) {
            diag_printf("failed to read emergency file\n");
            abort();
        }
        po_close( fd );

        diag_printf("loaded image, verifying\n");
        
        if( !pUpdate->VerifyImage( (const char*)buf, len ) ) {
            diag_printf("failed to verify emergency file\n");
            abort();
        }

        diag_printf("verified image, burning\n");
        
        // burn the image
        if( !pUpdate->UpdateImage( (const char*)buf, len, 0 ) ) {
            diag_printf("failed to update image\n");
            abort();
        }

        diag_printf("burned, attempting to load\n");
        
        // close down the disk
        pc_system_close(0);

        // the flash has been reburned at this point - sketchy since we run from flash
        // rescan the table and see if app is there now
        if( !loadimage("app") ) {
            // at this point, we're toast
            diag_printf("failed to load freshly burned app\n");
            abort();
        }
    }
}


void abort()
{
    // We end up here if something went horribly wrong.
    // At this point the device probably needs to be shipped back
    bool bOrange = true;
    *(volatile cyg_uint8*)PBDDR = 0xff;
    
    LCDWriteRaw(ucReinstallFailedBitmap, sizeof(ucReinstallFailedBitmap));
    
    while(1) {
        // pause 500ms
        HAL_DELAY_US( 500 * 1000 );
        // print something on serial
        diag_printf("Failed to boot\n");
        // oscillate the LED between orange and red
        if( bOrange ) {
            *(volatile cyg_uint8*)PBDR &= 0xF7;
            *(volatile cyg_uint8*)PBDR |= 0x04;
        }
        else {
            *(volatile cyg_uint8*)PBDR |= 0x0C;
        }
        bOrange = !bOrange;
    }
}

void cyg_start(void)
{
	main_loop();
}


