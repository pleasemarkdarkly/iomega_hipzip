#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/ir/IR_UEI.h>
#include <util/eventq/EventQueueAPI.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

const ir_key_map_t ir_press_map[] = 
{

  {0x101,1}, // 1
{0x401,2},
{0x501,3},
{0x1001,4},
{0x1101,5},
{0x1401,6},
{0x1501,7},
{0x4001,8},
{0x4101,9}, // 9
{0x4100,10}, // mute
{0x1,11}, // 0
{0x1500,12}, // enter
{0x500,13}, 
{0x4000,14},
{0x100,15},
{0x1000,16},
{0x4511,17},
{0x400,18},
{0x1505,19}, // guide
{0x5401,20},
{0x1405,21},
{0x1000,22},
{0x5101,23},
{0x500,24},
{0x1400,25},
{0x5501,26},
{0x0,27}, // menu
{0x4500,28},
{0x1005,29},
{0x4401,30},
{0x4501,31},
{0x5100,32},
{0x5001,33},
{0x1105,34}, // vcr +
{0x4411,35},
{0x4111,36},
{0x4011,37},
{0x5541,38},
{0x5441,39},
{0x5141,40},
{0x5041,41},
{0x4400,42},
{0x5000,43},
{0x5500,44},
{0x1100,45}, // skip
{0x5400,46} // power!

};


const ir_map_t ir_map = 
{
    num_buttons: 46,
    repeat_flags: (IR_REPEAT_ENABLE),
    filter_start: 5,  // 0 == unfiltered
    filter_rate:  2,  // 0 == unfiltered
    press_map: ir_press_map,
    hold_map: ir_press_map
};

/* FUNCTIONS */

static void
_KeyboardTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    CEventQueue * EventQ = CEventQueue::GetInstance();
    DEBUG("EventQueue %screated\n", EventQ == 0 ? "not" : "");

    CIR * pIR = CIR::GetInstance();
    DEBUG("IR %screated\n", pIR == 0 ? "not" : "");

    pIR->SetIRMap( &ir_map );
    
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
    
    cyg_thread_create(10, _KeyboardTestThread, (cyg_addrword_t) 0, "KeyboardTestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DEBUG("-%s\n", __FUNCTION__);
}

