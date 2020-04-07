#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_arch.h>
#include <cyg/hal/drv_api.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

cyg_handle_t _ThreadH[NTHREADS];
cyg_thread _Thread[NTHREADS];
char _ThreadStack[NTHREADS][STACKSIZE];

static cyg_interrupt ms_interrupt;
static cyg_handle_t ms_interrupt_handle;

static cyg_interrupt tc1_interrupt;
static cyg_handle_t tc1_interrupt_handle;

static cyg_uint32 _tick = 0;

static cyg_uint32
tc1_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_TC1OI);
    ++_tick;
    return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}

static void
tc1_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
}

static cyg_uint32 _period;

static void
tc1_init(cyg_uint32 period)
{
    volatile cyg_uint32 *syscon1 = (volatile cyg_uint32 *)SYSCON1;
    volatile cyg_uint32 *tc1d = (volatile cyg_uint32 *)TC1D;
    // Set timer to 512KHz, prescale mode
    *syscon1 = (*syscon1 & ~(SYSCON1_TC1M|SYSCON1_TC1S)) | SYSCON1_TC1S | SYSCON1_TC1M;
    // Initialize counter
    *tc1d = period;
    _period = period;

    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_TC1OI,
			     99,
			     (cyg_addrword_t)0,
			     tc1_ISR,
			     tc1_DSR,
			     &tc1_interrupt_handle,
			     &tc1_interrupt);
    cyg_drv_interrupt_attach(tc1_interrupt_handle);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_TC1OI);
}

cyg_uint32 _start = 0;
cyg_uint32 _current = 0;

int mstab[2500];
int mstabi = 0;

int _ms;

static cyg_uint32
ms_ISR(cyg_vector_t vector, cyg_addrword_t data)
{
    int ms;
    
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_UMSINT);

    _ms = (*(volatile cyg_uint32 *)SYSFLG1) & SYSFLG1_CTS;
    
    //mstab[mstabi++] = ms;
    
    _current = _tick;
    
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_UMSINT);
    return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}

int _state = 0;

static int pwtab[2500];
static int pwtabi = 0;

static cyg_uint8 Btab[2500];
static int Btabi = 0;

static void
ms_DSR(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    cyg_uint32 pulse_width;

    static cyg_uint8 byte = 0;
    static int bit = 0;
    static int n_byte = 0;

#if 0
    while (_tick < _current + 3)
	;
    
    if (_ms != ((*(volatile cyg_uint32 *)SYSFLG1) & SYSFLG1_CTS)) {
	cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
	return;
    }
#endif
    
    pulse_width = _current - _start;
    _start = _current;

    pwtab[pwtabi++] = pulse_width;

    if (pwtabi == 2500) {
	int i;
	for (i = 0; i < pwtabi; ++i) {
	    diag_printf("%d\n", pwtab[i]);
	}
	for (;;) {
	    cyg_thread_delay(10);
	}
    }
    
    switch (_state) {
	case 0: 
	{
	    /* transition */
	    /* 9ms */
	    if ((70 <= pulse_width) && (pulse_width <= 74)) {
		_state = 1;
	    }
	    break;
	}

	case 1: 
	{
	    /* transition */
	    /* 4.5ms */
	    if ((33 <= pulse_width) && (pulse_width <= 36)) {
		_state = 2;
	    }
	    /* 2.25ms */
	    else if ((16 <= pulse_width) && (pulse_width <= 20)) {
		_state = 3;
	    }
	    break;    
	}

	case 2: 
	{
	    /* transition */
	    /* .56mS */
	    if ((4 <= pulse_width) && (pulse_width <= 6)) {
		_state = 4;
	    }
	    break;
	}


	case 3: 
	{
	    /* transition */
	    /* .56mS */
	    if ((4 <= pulse_width) && (pulse_width <= 6)) {
		_state = 0;
	    }
	    
	    Btab[Btabi++] = 0xdd; // repeat
	    
	    break;
	}

	case 4: 
	{
	    /* action */
	    if ((3 <= pulse_width) && (pulse_width <= 4)) {
		byte |= (0 << bit);
	    }
	    else if ((12 <= pulse_width) && (pulse_width <= 14)) {
		byte |= (1 << bit);
	    }
	    bit++;

	    /* transition */
	    if (bit < 8) {
		_state = 2;
	    }
	    else {

		Btab[Btabi++] = byte;
		
		byte = 0;
		bit = 0;
		++n_byte;
		if (n_byte < 4) {
		    _state = 2;
		}
		else {
		    n_byte = 0;
		    _state = 0;
		}
	    }
	    break;
	}
    }
    
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
}

static bool
ms_init(void)
{
    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_UMSINT,
			     99,
			     (cyg_addrword_t)0,
			     ms_ISR,
			     ms_DSR,
			     &ms_interrupt_handle,
			     &ms_interrupt);
    cyg_drv_interrupt_attach(ms_interrupt_handle);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_UMSINT);
    return true;
}

static void
_TestThread(CYG_ADDRESS Data)
{
    diag_printf("+%s\n", __FUNCTION__);
    
    tc1_init(64);		// .125ms period
    ms_init();
}

extern "C" {
void
cyg_user_start(void)
{
    diag_printf("+%s\n", __FUNCTION__);

    cyg_thread_create(10, _TestThread, (cyg_addrword_t) 0, "TestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}
};
