#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/battery/battery.h>
/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

/* FUNCTIONS */

static void
_BatteryTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    for (;;) {
	DEBUG("Battery level %d\n", BatteryGetLevel());
	DEBUG("Charge status %d\n", BatteryGetChargeStatus());
    }
    
    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" void cyg_user_start(void);

void cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _BatteryTestThread, (cyg_addrword_t) 0, "BatteryTestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DEBUG("-%s\n", __FUNCTION__);
}

