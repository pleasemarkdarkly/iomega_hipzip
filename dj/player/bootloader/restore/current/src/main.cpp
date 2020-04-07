// main.cpp: restore emergency image from hard drive
// temancl@fullplaymedia.com
// (c) Fullplay Media
//
// notes:
// uses main/util/update and fs/flash for code-reuse and consistency with the app
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/hal_intr.h>
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/hal_tables.h>
#include <cyg/hal/hal_if.h>
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_cache.h>


#include <fs/fat/sdapi.h>
#include <fs/flash/flashmanager.h>

#include <datastream/fatfile/FatFile.h>

#define IMAGE_NAME "app"
#define FACTORY_PATH "A:\\SYSTEM\\FACTORY.ORG"

// thread/ecos magic
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

extern "C" {
    void cyg_user_start( void );
    void thread_entry( cyg_uint32 data );
};



///////////////////////////////////////////////////////////////////
// LED Control functions
/////////////////////////////////////////////////////////////////

void
LEDOrange()
{
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR |= 0x0C;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED orange\n");
#endif  // __DJ
}


void
LEDGreen()
{
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR &= 0xFB;
    *(volatile cyg_uint8 *)PBDR |= 0x08;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED green\n");
#endif  // __DJ
}

void
LEDRed()
{
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR &= 0xF7;
    *(volatile cyg_uint8 *)PBDR |= 0x04;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED red\n");
#endif  // __DJ
}

///////////////////////////////////////////////////////////////////
// Display functions, substitute screen display / better ui here
/////////////////////////////////////////////////////////////////
void display_error()
{

	diag_printf("UI: Fatal Error: factory restore failed\n");

	LEDRed();
	// halt on fatal error
	while(1);

}


void display_working()
{
	LEDOrange();
	diag_printf("UI: Factory Restore in Progress, please wait\n");

}

void display_done()
{
	LEDGreen();
	diag_printf("UI: Factory Restore Complete, Restarting system\n");
}

////////////////////////////////////////////
// main function, attempt image restore
////////////////////////////////////////////

void thread_entry( cyg_uint32 data )
{

	diag_printf("Emergency Factory Restore\n");
	diag_printf("(c) Fullplay Media 2002\n");
	display_working();

	// init disk
	int status;
	status = pc_system_init(0);

	if(!status)
	{
		diag_printf("drive 0 initialization failure\n");
		display_error();
	}

	// search for factory image on hd
	
	
	char* buffer;
	CFatFile file;
	unsigned long size = 0;
	
	CFlashManager *pfm = CFlashManager::GetInstance();

	DSTAT ds;
	
	if(pc_gfirst(&ds,FACTORY_PATH))
	{
		size = ds.fsize;
	}

	pc_gdone(&ds);

	if(size == 0)
	{
		diag_printf("factory image size = 0\n");
		display_error();
	}

	if(!file.Open(FACTORY_PATH))
	{
		diag_printf("error reading factory image\n");
		display_error();
	}

	buffer = (char *)malloc(sizeof(char)*size);

	if(!file.Read(buffer,size))
	{
		diag_printf("error reading factory image\n");
		display_error();
	}
	
	file.Close();	
	pc_system_close(0);

	int oldints;
	HAL_DISABLE_INTERRUPTS(oldints);
	
	// rewrite that image
	diag_printf("writing factory image\n");
	pfm->UpdateImage(IMAGE_NAME,buffer,size);
	free((void*)buffer);

	// all done, reset	

	display_done();

	// disable interrupts, flush cache
	HAL_UCACHE_DISABLE();
	HAL_UCACHE_INVALIDATE_ALL();

	// get reset vector
	void (*f)() = (void(*)())hal_vsr_table[0];

	// jump
	f();
	
}


void cyg_user_start( void ) 
{
   
    cyg_thread_create( 9, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
    
}
