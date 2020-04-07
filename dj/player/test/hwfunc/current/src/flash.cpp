#include <cyg/kernel/kapi.h>
#include <cyg/fileio/fileio.h>
#ifndef INTERNAL
#include <datastream/isofile/IsoFileInputStream.h>
#endif
#include <cyg/infra/diag.h>
#include <cyg/io/flash.h>
#include <stdlib.h>

	
unsigned long workspace_size = FLASH_MIN_WORKSPACE;
unsigned char workspace[FLASH_MIN_WORKSPACE]  __attribute__((aligned(4)));;

static int flash_print( const char* fmt, ... ) 
{    
    return 0;
}

extern "C"
{
int test_flash(char param_strs[][32],int* param_nums)
{
	int read;
	int stat;
	unsigned char* buff;
	unsigned long ulFlashStart,ulFlashEnd;
	void* err_addr;
	int err;
	int timeout = 10;

#ifndef INTERNAL

	diag_printf("workspace at 0x%x, size = %d\n",workspace,FLASH_MIN_WORKSPACE);


	
	diag_printf("flash init\n");
	if((stat = flash_init(workspace,FLASH_MIN_WORKSPACE,flash_print)) != 0)
	{
        diag_printf("FLASH: driver init failed!, status: 0x%x\n", stat);
		free(buff);
        return -1;
	}
		
	diag_printf("flash limits\n");
	flash_get_limits(NULL, (void **)&ulFlashStart, (void **)&ulFlashEnd);
	
	 while((err = mount("/dev/cda/", "/", "cd9660")) < 0 && timeout > 0)
	 {
		 diag_printf("Error mounting CD, try inserting a disc\n");
		 timeout--;
	 }


	 if(timeout <= 0)
	 {
		 diag_printf("cd mount failed\n");
		 return -1;
	 }

	// open CD
	CIsoFileInputStream cdis;

	cdis.Open("/dj-90_full_image.bin");

	// try and read flash bin
	
	buff = (unsigned char*)malloc(sizeof(unsigned char)*cdis.Length());
	if(buff == NULL)
		return -1;
	
	read = cdis.Read(buff,cdis.Length());
	
	// we should read 2 megs
	if(read != 2*1024*1024)
	{
		diag_printf("read error\n");
		free(buff);
		return -1;
	}


	diag_printf("erase - from %x to %x\n",ulFlashStart,ulFlashEnd);
	// erase all flash
	if ((flash_erase((void *)ulFlashStart, read, (void **)&err_addr)) != 0) 
    {
        diag_printf("Error erasing flash \n");
		free(buff);
		return -1;
    }

	diag_printf("program\n");
   	// write bin
    if ((flash_program((void *)ulFlashStart, (void *)buff, read, (void **)&err_addr)) != 0) 
    {
        diag_printf("Error programming flash\n");	
		free(buff);
		return -1;
    }

	free(buff);
	// flash complete
#endif

	return 0;

}

};