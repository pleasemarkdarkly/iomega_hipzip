#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/jogdial/JogDial.h>
#include <util/eventq/EventQueueAPI.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
_JogDialTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    CEventQueue * EventQ = CEventQueue::GetInstance();
    DEBUG("EventQueue %screated\n", EventQ == 0 ? "not" : "");
    
    CJogDial * Jog = CJogDial::GetInstance();
    Jog->SetKeymap( 0 , 1 );
    DEBUG("JogDial %screated\n", Jog == 0 ? "not" : "");
    
    for (;;) {
	unsigned int Key;
	void * Data;
	EventQ->GetEvent( &Key, &Data );
	DEBUG("Key: %x Data: %x\n", Key, Data);
    }

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" void cyg_user_start(void);

void cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _JogDialTestThread, (cyg_addrword_t) 0, "JogDialTestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DEBUG("-%s\n", __FUNCTION__);
}

