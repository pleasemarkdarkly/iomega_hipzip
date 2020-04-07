#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/io.h>

#include <io/storage/blk_dev.h>

/* DEFINES */

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

/* FUNCTIONS */
extern "C" {
  void _ATATestThread( CYG_ADDRESS Data );
  void cyg_user_start(void);
};

void
_ATATestThread(CYG_ADDRESS Data)
{
    diag_printf("+%s\n", __FUNCTION__);

    cyg_uint32 Length;
    Cyg_ErrNo ErrNo;
    
    cyg_io_handle_t CFHandle;
    ErrNo = cyg_io_lookup("/dev/hdb/", &CFHandle);
    if (ErrNo != ENOERR) {
	diag_printf("Could not get handle to /dev/hdb/ %d", ErrNo);
	return;
    }
    
    Length = 1;
    ErrNo = cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_POWER_UP, 0, &Length);
    if (ErrNo != ENOERR) {
	diag_printf("Could not power up drive %d\n", ErrNo);
    }
    else {
	diag_printf("Powered up drive\n");
    }

    for (;;) {
	diag_printf("Reading\n");

	Length = 512;
	char Buffer[512];
	ErrNo = cyg_io_bread(CFHandle, Buffer, &Length, 0);
	if (ErrNo != ENOERR) {
	    diag_printf("Could not read sector 0 %d\n", ErrNo);
	}
	else {
	    diag_printf("Read sector 0\n");
	}
    }
    
    diag_printf("-%s\n", __FUNCTION__);
}

void
cyg_user_start(void)
{
    diag_printf("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _ATATestThread, (cyg_addrword_t) 0, "ATATestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}
