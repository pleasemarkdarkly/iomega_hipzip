#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/keyboard/Keyboard.h>
#include <util/eventq/EventQueueAPI.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

/* FUNCTIONS */

// keymap
#if __DHARMA == 2
static unsigned int _press_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
static unsigned int _hold_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
static unsigned int _release_map[] = 
{
    1,2,3,4,5,6,9,10,11,12,13,14,7,8,15,16
};
#else /* __DHARMA == 1 */
static unsigned int _press_map[] =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
static unsigned int _hold_map[]  =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
static unsigned int _release_map[]  =
{
    1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
};
#endif

static key_map_t key_map =
{
#if __DHARMA == 2
    num_columns  :  3,
    num_rows     :  6,
    num_buttons  : 16,
#else /* __DHARMA == 1 */
    num_columns  :  8,
    num_rows     :  2,
    num_buttons  : 16,
#endif
    
    repeat_flags : (KEY_REPEAT_ENABLE),

    tick_length  :  2,
    repeat_rate  :  6,
    initial_delay: 12,

    press_map    : (const unsigned int*const)_press_map,
    hold_map     : (const unsigned int*const)_hold_map,
    release_map  : _release_map,
};

static void
_KeyboardTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    CEventQueue * EventQ = CEventQueue::GetInstance();
    DEBUG("EventQueue %screated\n", EventQ == 0 ? "not" : "");
    
    CKeyboard * Kbd = CKeyboard::GetInstance();
    Kbd->SetKeymap( &key_map );
    DEBUG("Keyboard %screated\n", Kbd == 0 ? "not" : "");
    
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

