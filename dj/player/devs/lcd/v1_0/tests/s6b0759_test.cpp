#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <devs/lcd/lcd.h>
/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static cyg_uint8 uc_Dadio_Bitmap[784] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc1,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0x18,0x7f,0xff,0xff,0xff,0xff,0xff,0xfc,0x7e,0x1f,0xff,0xff,
0xff,0xff,0xff,0xf8,0xff,0x87,0xff,0xff,0xff,0xff,0xff,0xf1,0xe0,0x01,0xff,0xff,
0xff,0xff,0xff,0xe3,0xc0,0x00,0x3f,0xff,0xff,0xff,0xff,0xe7,0xe0,0x00,0x3f,0xff,
0xff,0xff,0xff,0xc7,0xf8,0x00,0x1f,0xff,0xff,0xff,0xff,0x87,0xe7,0x20,0x0f,0xff,
0xff,0xff,0xff,0x83,0xdd,0xb8,0x03,0xff,0xff,0xff,0xff,0x03,0xbc,0xb8,0x03,0xff,
0xff,0xff,0xfe,0x03,0xb1,0x2f,0x0a,0x7f,0xff,0xff,0xfe,0x03,0x9f,0x85,0x04,0x7f,
0xff,0xff,0xfc,0x08,0x3f,0xa9,0x85,0xff,0xff,0xff,0xfc,0x18,0x7f,0xbe,0xc0,0xff,
0xff,0xff,0xf8,0x10,0xff,0xbe,0x40,0xff,0xff,0xff,0xf8,0x01,0xff,0x1b,0x60,0xff,
0xff,0xff,0xf0,0x03,0xde,0x0d,0x60,0xff,0xff,0xff,0xf0,0x03,0xec,0x25,0x20,0xff,
0xff,0xff,0xf0,0x0b,0x9c,0x15,0xa0,0xff,0xff,0xff,0xe0,0x1d,0x5c,0x05,0x20,0xff,
0xff,0xff,0xe0,0x2d,0x54,0x05,0x60,0xff,0xff,0xff,0xe0,0x15,0x56,0x0d,0x20,0x7f,
0xff,0xff,0xe0,0x09,0x5f,0x9b,0x61,0x7f,0xff,0xff,0xe0,0x03,0x97,0x7b,0x21,0x7f,
0xff,0xff,0xe0,0x01,0xf0,0x1f,0x20,0x7f,0xff,0xff,0xe0,0x00,0xfe,0x0f,0x70,0x7f,
0xff,0xff,0xe0,0x00,0x7c,0x26,0x7c,0x7f,0xff,0xff,0xf0,0x07,0xbc,0x14,0x19,0x7f,
0xff,0xff,0xf0,0x07,0xc4,0x03,0xc2,0x7f,0xff,0xff,0xf0,0x07,0xf8,0x07,0xfe,0xff,
0xff,0xff,0xf8,0x07,0xfe,0x0f,0xfc,0xff,0xff,0xff,0xf8,0x0f,0xff,0x1f,0xf9,0xff,
0xff,0xff,0xfc,0x0f,0xff,0xff,0xf1,0xff,0xff,0xff,0xfc,0x1f,0xf8,0x1f,0xe3,0xff,
0xff,0xff,0xfe,0x7f,0xe7,0xe7,0xc7,0xff,0xff,0xff,0xff,0x3f,0x8f,0xf3,0x8f,0xff,
0xff,0xff,0xff,0x9e,0x1f,0xf8,0x1f,0xff,0xff,0xff,0xff,0xc0,0x7f,0xfc,0x3f,0xff,
0xff,0xff,0xff,0xe1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xf8,0x3e,0x00,0x00,0x00,0x7f,0xff,0xff,0xfb,0xbe,0x00,0x00,0x00,0x7f,
0xff,0xff,0xf8,0x0e,0x00,0x00,0x00,0x7f,0xff,0xff,0xff,0xfe,0x00,0x00,0x00,0x7f,
0xff,0xff,0xf8,0x2e,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xfe,0x0f,0xff,0xe0,0xff,
0xff,0xff,0xe8,0x3e,0x1f,0xff,0xe0,0xff,0xff,0xff,0xeb,0xbe,0x1f,0xff,0xe1,0xff,
0xff,0xff,0xe0,0x3f,0x0f,0xff,0xe1,0xff,0xff,0xff,0xff,0xff,0x81,0xff,0x83,0xff,
0xff,0xff,0xf8,0x2f,0xe0,0x00,0x0f,0xff,0xff,0xff,0xff,0xff,0xfe,0x00,0x7f,0xff,
0xff,0xff,0xff,0xbf,0xff,0xff,0xff,0xff,0xff,0xff,0xf8,0x0f,0x80,0x3f,0xff,0xff,
0xff,0xff,0xff,0xbf,0x00,0x0f,0x1f,0xff,0xff,0xff,0xff,0xff,0x1f,0xc7,0x0f,0xff,
0xff,0xff,0xf8,0xbf,0x3f,0xe7,0x8f,0xff,0xff,0xff,0xfa,0xbf,0x3f,0xe7,0x8f,0xff,
0xff,0xff,0xf8,0x3e,0x00,0x00,0x0f,0xff,0xff,0xff,0xff,0xfe,0x00,0x00,0x0f,0xff,
0xff,0xff,0xf8,0x0e,0x00,0x00,0x1f,0xff,0xff,0xff,0xff,0xfe,0x7f,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xc0,0x0f,0xff,0xff,
0xff,0xff,0xf8,0xbf,0x80,0x07,0xff,0xff,0xff,0xff,0xfa,0xbf,0x87,0xe3,0xff,0xff,
0xff,0xff,0xf8,0x3f,0x8f,0xf3,0xff,0xff,0xff,0xff,0xff,0xff,0x8f,0xf3,0xff,0xff,
0xff,0xff,0xf8,0x3f,0x80,0x03,0xff,0xff,0xff,0xff,0xfb,0xff,0x00,0x00,0x01,0xff,
0xff,0xff,0xf8,0x3f,0x00,0x00,0x01,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x01,0xff,
0xff,0xff,0xf8,0x3f,0x1f,0xff,0xff,0xff,0xff,0xff,0xfb,0xbf,0xff,0xfe,0x1f,0xff,
0xff,0xff,0xf8,0x0c,0x00,0x0c,0x1f,0xff,0xff,0xff,0xff,0xfc,0x00,0x0c,0x1f,0xff,
0xff,0xff,0xf8,0x2c,0x00,0x0f,0xff,0xff,0xff,0xff,0xff,0xfc,0x7f,0xff,0xff,0xff,
0xff,0xff,0xf8,0x3f,0xf8,0x01,0xff,0xff,0xff,0xff,0xfb,0xbf,0x80,0x00,0x3f,0xff,
0xff,0xff,0xf8,0x3f,0x00,0x00,0x1f,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x0f,0xff,
0xff,0xff,0xff,0xfe,0x00,0x00,0x07,0xff,0xff,0xff,0xff,0xfe,0x00,0x7c,0x03,0xff,
0xff,0xff,0xf8,0x0e,0x00,0xff,0x03,0xff,0xff,0xff,0xfb,0xee,0x00,0xff,0x03,0xff,
0xff,0xff,0xfb,0xee,0x00,0xff,0x03,0xff,0xff,0xff,0xf8,0x0f,0x00,0x7e,0x03,0xff,
0xff,0xff,0xff,0xff,0x00,0x3c,0x07,0xff,0xff,0xff,0xfb,0x0f,0x80,0x00,0x0f,0xff,
0xff,0xff,0xfb,0x6f,0xc0,0x00,0x1f,0xff,0xff,0xff,0xf8,0x6f,0xf0,0x00,0x7f,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,};

/* FUNCTIONS */

static void
_LCDTestThread(CYG_ADDRESS Data)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    LCDEnable();
    LCDClear();

#if 1
    for (int i = 0; i < 10; ++i) {
	for (int Y = 0; Y < 80; ++Y) {
	    for (int X = 0; X < 128; ++X) {
		LCDPutPixel(1, X, Y);
	    }
	}

	cyg_thread_delay(100);

	for (int Y = 79; Y >= 0; --Y) {
	    for (int X = 127; X >= 0; --X) {
		LCDPutPixel(0, X, Y);
	    }
	}
    }
#endif

#if 0
    /* This will jumble the screen unless you set the page and column to the same as iomega (8 and 98) */
    LCDWriteInverted(uc_Dadio_Bitmap, sizeof(uc_Dadio_Bitmap));
#endif
    
    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" void cyg_user_start(void);

void cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _LCDTestThread, (cyg_addrword_t) 0, "LCDTestThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DEBUG("-%s\n", __FUNCTION__);
}

