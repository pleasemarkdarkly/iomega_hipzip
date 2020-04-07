#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>
#include <cyg/io/io.h>

#include <string.h>

#include <io/storage/blk_dev.h>
#define CMD_IDENTIFY 0xec

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

unsigned short IdentifyData[256];
unsigned char SectorData[512 * 64];
unsigned char SenseData[14];

/* FUNCTIONS */
extern "C" {
  void _ATATestThread( CYG_ADDRESS Data );
  void cyg_user_start(void);
};


void
_ATATestThread(CYG_ADDRESS Data)
{
    DEBUG("+%s\n", __FUNCTION__);

    cyg_uint32 Length;
    
    cyg_io_handle_t CFHandle;
    cyg_io_lookup("/dev/hdb/", &CFHandle);

    Length = 1;
    cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_POWER_UP, 0, &Length);
    
    Length = sizeof(SectorData);
    cyg_io_bread(CFHandle, SectorData, &Length, 0);

    diag_printf("waiting\n");
    cyg_thread_delay(1000);

    Length = sizeof(SectorData);
    cyg_io_bread(CFHandle, SectorData, &Length, 0);
    
    Length = 1;
    cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_SLEEP, 0, &Length);

    diag_printf("asleep\n");
    
    Length = 1;
    cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_WAKEUP, 0, &Length);

    diag_printf("awake\n");
    
    Length = 1;
    cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_POWER_DOWN, 0, &Length);

    diag_printf("powered down\n");
    
    Length = 1;
    cyg_io_set_config(CFHandle, IO_BLK_SET_CONFIG_POWER_UP, 0, &Length);

    diag_printf("powered up\n");
    
    Length = sizeof(SectorData);
    cyg_io_bread(CFHandle, SectorData, &Length, 0);
#if 0
    cyg_io_handle_t ATAHandle;
    cyg_io_lookup("/dev/ide/0", &ATAHandle);
    
    Length = 512;
    cyg_io_bread(ATAHandle, SectorData, &Length, 0);

    Length = 1024;
    cyg_io_bread(ATAHandle, SectorData, &Length, 0);

    cyg_io_handle_t ATAPIHandle;
    cyg_io_lookup("/dev/ide/1", &ATAPIHandle);
#endif
    DEBUG("-%s\n", __FUNCTION__);
}

void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _ATATestThread, (cyg_addrword_t) 0, "ATATestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}
