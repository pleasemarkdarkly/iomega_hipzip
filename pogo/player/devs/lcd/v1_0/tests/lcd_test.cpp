#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <stdio.h>

#include <cyg/hal/hal_edb7xxx.h>

#include <devs/lcd/lcd.h>
/* DEFINES */

#define DEBUG(s...) diag_printf(##s)

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

/* FUNCTIONS */

#define TEST_PRESCALE
//#define TEST_PUT_PIXEL
//#define TEST_PIXEL_DRAW
//#define TEST_SCREEN_DRAW

static void
_LCDTestThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    LCDEnable();
    LCDClear();

    volatile unsigned char * FrameBuffer = (unsigned char * )LCD_FRAME_BUFFER_ADDRESS;

#if defined(TEST_PRESCALE)
    LCDDisable();
    int interval = LCD_WIDTH / 16;
    for (int prescale = 0; prescale < 64; ++prescale) {
	unsigned int ctl_word = *(volatile cyg_uint32 *)LCDCON;
	ctl_word &= ~(0x3f<<25);
	ctl_word |= (prescale<<25);
	*(volatile cyg_uint32 *)LCDCON = ctl_word;
	DEBUG("Prescale %d\n", prescale);
	  
	LCDClear();
	for (int y = 0; y < LCD_HEIGHT; ++y) {
	    int color = 0;
	    for (int x = 0; x < LCD_WIDTH; x += interval) {
		for (int i = 0; i < interval; ++i) {
		    LCDPutPixel(color, x + i, y);
		}
		++color;
	    }
	}
	
	cyg_thread_delay(100);
	LCDDisable();
    }
#endif
    
#if defined(TEST_PUT_PIXEL)
    for( int i = 0; i < 100; i++ ) {
        LCDPutPixel( 0x0f, i, 0 );
    }
#endif
    
#if defined(TEST_PIXEL_DRAW)
    unsigned char color = 0x00;
    for( int i = 0, size = (LCD_HEIGHT * LCD_WIDTH * LCD_BITS_PER_PIXEL)/8;
         i < size;
         i++ ) {
        unsigned char pixel = (color << 4) | color;

        FrameBuffer[i] = pixel;

        if( i != 0 && (i % 4) == 0 ) {
            color++;
            if( color == 16 ) color = 0;
        }
        for( int z = 0; z < 100; z++ ) ;
    }
#endif

#if defined(TEST_SCREEN_DRAW)
    for (unsigned char Color = 0; Color < 16; ++Color) {
	cyg_thread_delay(200);
	DEBUG("Color %d\n", Color);
	unsigned char ColorTwoPixels = (Color << 4) | (Color);
	memset((void*)FrameBuffer, ColorTwoPixels, (LCD_HEIGHT * LCD_WIDTH * LCD_BITS_PER_PIXEL) / 8);
    }
#endif
    
#if defined(LCD_BACKLIGHT)
    for (int i = 0; i < 5; ++i) {
	DEBUG("Backlight on\n");
	LCDSetBacklight(LCD_BACKLIGHT_ON);
	cyg_thread_delay(100);
	DEBUG("Backlight off\n");
	LCDSetBacklight(LCD_BACKLIGHT_OFF);
	cyg_thread_delay(100);
    }
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


