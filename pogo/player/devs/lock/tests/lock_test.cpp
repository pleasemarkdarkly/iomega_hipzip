#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/lock/LockSwitch.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
_LockSwitchTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);
    
    CLockSwitch * Lock = CLockSwitch::GetInstance();
    DEBUG("CLockSwitch %screated\n", Lock == 0 ? "not" : "");
    
    for (;;) {
		
		DEBUG("Lock %s\n", Lock->IsLocked() ? "Enabled" : "Disabled");
		cyg_thread_delay(100);
    }

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" void cyg_user_start(void);

void cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _LockSwitchTestThread, (cyg_addrword_t) 0, "LockSwitchTest",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DEBUG("-%s\n", __FUNCTION__);
}

