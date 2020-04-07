#include <redboot.h>
#if 0
#include <cyg/kernel/kapi.h>
#endif
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include "demojumper/lcd.h"
//#include "debug.h"

#define TOTAL_PIXELS (LCD_WIDTH * LCD_HEIGHT)
#define TOTAL_BITS (TOTAL_PIXELS * LCD_BITS_PER_PIXEL)

int
LCDEnable(void)
{
    cyg_uint32 ControlWord;

    /* Make sure LCD is disabled first */
    *(volatile cyg_uint32 *)SYSCON1 &= ~SYSCON1_LCDEN;

    /* Map greyscale levels */
    switch (LCD_BITS_PER_PIXEL) {
	case 4:
	{
	    ControlWord = LCDCON_GSEN | LCDCON_GSMD;
	    *(volatile cyg_uint32 *)PALMSW = 0xfedcba98;
	    *(volatile cyg_uint32 *)PALLSW = 0x76543210;
	    break;
	}
	case 2:
	{
	    ControlWord = LCDCON_GSEN;
	    *(volatile cyg_uint32 *)PALMSW = 0xfa50fa50;
	    *(volatile cyg_uint32 *)PALLSW = 0xfa50fa50;
	    break;
	}
	case 1:
	{
	    ControlWord = 0;
	    *(volatile cyg_uint32 *)PALMSW = 0xf0f0f0f0;
	    *(volatile cyg_uint32 *)PALLSW = 0xf0f0f0f0;
	}
    };

    /* Video buffer size */
    ControlWord |= ((LCD_WIDTH * LCD_HEIGHT * LCD_BITS_PER_PIXEL) / 128 - 1) << LCDCON_BUFSIZ_S;
    /* Line length */
    ControlWord |= ((LCD_WIDTH / 16) - 1) << LCDCON_LINE_LENGTH_S;
    /* Pixel prescale */
    ControlWord |= ((526628 / (LCD_HEIGHT * LCD_WIDTH)) - 1) << LCDCON_PIX_PRESCALE_S;
    /* AC frequency bias */
    /* TODO This is wrong for the large lcd, cycle through the greyscales to see the problem */
    ControlWord |= 13 << LCDCON_AC_PRESCALE_S;

    *(volatile cyg_uint32*)LCDCON = ControlWord;
   
    /* Power up LCD driver */
    *(volatile cyg_uint8 *)PADDR |= 0x80;
    *(volatile cyg_uint8 *)PADR &= ~0x80;

    /* Wait at least 20mS between power up and enable */
#if 0	// should be one if cyg_threads are available
    cyg_thread_delay(3);
#else
    HAL_DELAY_US(20 * 1000);
#endif
    
    /* Enable display */
    *(volatile cyg_uint8 *)PBDDR |= 0x08;
    *(volatile cyg_uint8 *)PBDR |= 0x08;
    
    *(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_LCDEN;

    return 0;
}

int
LCDDisable(void)
{
    /* TODO */
    return 0;
}

int
LCDSetBacklight(int Value)
{
    if (Value == LCD_BACKLIGHT_OFF) {

	/* Power down LCD backlight */
	*(volatile cyg_uint8 *)PADDR |= 0x40;
	*(volatile cyg_uint8 *)PADR &= ~0x40;
    }
    else if (Value == LCD_BACKLIGHT_ON){

	/* Power up LCD backlight */
	*(volatile cyg_uint8 *)PADDR |= 0x40;
	*(volatile cyg_uint8 *)PADR |= 0x40;
    }
}

int
LCDClear(void)
{
    cyg_uint8 * FrameBuffer = (cyg_uint8 *)LCD_FRAME_BUFFER_ADDRESS;
    int Position;
    
    for (Position = 0; Position < (LCD_WIDTH * LCD_HEIGHT * LCD_BITS_PER_PIXEL) / 8; ++Position) {
	FrameBuffer[Position] = 0x00;
    }
    
    return 0;
}

int
LCDWriteRaw(const cyg_uint8 * Buffer, int Size) 
{
    cyg_uint8 * FrameBuffer = (cyg_uint8 *)LCD_FRAME_BUFFER_ADDRESS;
    int i;
  
#if (LCD_BITS_PER_PIXEL == 4)
    for (i = 0; i < Size; i += 2) {
	FrameBuffer[i] = (Buffer[i + 1] ^ 0xff);
	FrameBuffer[i + 1] = (Buffer[i] ^ 0xff);
    }
#else
    for (i = 0; i < Size; i++) {
	FrameBuffer[i] = (Buffer[i] << 4) | (Buffer[i] >> 4);
    }
#endif

    return 0;
}

int
LCDPutPixel(unsigned char Color, int X, int Y) 
{
    cyg_uint8 * FrameBuffer = (cyg_uint8 *)LCD_FRAME_BUFFER_ADDRESS;
    int IsLowPixel;

    IsLowPixel = (X % 2);
    if (IsLowPixel) {
	Color <<= 4;
    }

    /* TODO This only works if you clear first */
    FrameBuffer[(Y * LCD_WIDTH) + (X / 2)] |= Color;

    return 0;
}
