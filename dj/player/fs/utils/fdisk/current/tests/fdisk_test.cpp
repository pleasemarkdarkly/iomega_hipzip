#include <stdlib.h>   /* rand, RAND_MAX*/
#include <string.h>   /* memset */

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>

#include <fs/fdisk/fdisk.h>

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

void
atapi_thread(unsigned int ignored)
{
	
	while(1)
	{
		fdisk_drive("/dev/hda/",0);
	}
}                                     


