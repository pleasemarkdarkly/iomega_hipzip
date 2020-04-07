//
// freebsd fat32 chkdsk utility
// temancl@fullplaymedia.com
//



#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <devs/storage/ata/atadrv.h>
#include <cyg/fileio/fileio.h>

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <fs/utils/chkdsk_fat32/chkdsk.h>

/* DEFINES */

#define NTHREADS 1
#define STACKSIZE ( (8 * 4096) )


/* STATICS */

static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char _stack[NTHREADS][STACKSIZE];

extern "C" 
{
void atapi_thread(unsigned int);
  void cyg_user_start(void);
};

void
cyg_user_start(void)
{
  cyg_thread_create(10, atapi_thread, (cyg_addrword_t) 0, "atapi_thread",
		    (void *)_stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
  cyg_thread_resume(thread[0]);
}


const char fname[] = "/dev/hda/";


const char szphases[][32] = {"Reading FATs","Checking Cluster Chains","Checking Directories","Checking for Lost Files","Updating FATs",NULL};



bool callback(int pass, int phase, int prog, int total)
{
	static int lastpass = -1;
	static int lastphase = -1;
	
	if(pass != lastpass)
	{
		diag_printf("\nCB: Starting pass %d\n",pass);
		lastpass = pass;
	}

	if(phase != lastphase)
	{
		diag_printf("\nCB: Starting phase %d - %s\n",phase+1,szphases[phase]);
		lastphase = phase;
	}

	diag_printf("CB: %s - %d of %d\r",szphases[phase],prog,total);

	// return if chkdsk should continue (true) or cancel (false)
	return true;

}


void
atapi_thread(unsigned int ignored)
{	
	
	diag_printf("chkdsk fat32 test\n");
	chkdsk_fat32(fname,callback);
	diag_printf("chkdsk fat32 test complete\n");
	while(1);
}                                     
