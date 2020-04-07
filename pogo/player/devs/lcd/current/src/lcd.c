#ifndef NOKERNEL
#include <cyg/kernel/kapi.h>
#else
#include <cyg/hal/hal_if.h>
#endif

#include <cyg/hal/hal_edb7xxx.h>
#include <devs/lcd/lcd.h>
#if __DHARMA == 2
#include <cyg/hal/mmgpio.h>
#endif

#define TOTAL_PIXELS (LCD_WIDTH * LCD_HEIGHT)
#define TOTAL_BITS  (TOTAL_PIXELS * LCD_BITS_PER_PIXEL)

int
LCDEnable(void)
{
    cyg_uint32 ctl_word;
    cyg_uint32 i;

    /* Make sure LCD is disabled first */
    *(volatile cyg_uint32 *)SYSCON1 &= ~SYSCON1_LCDEN;
    
    // Define the Palette.
    switch( LCD_BITS_PER_PIXEL ) {
        case 4:
        {
            ctl_word = LCDCON_GSEN | LCDCON_GSMD;
#if 1  // these are inverses of eachother.  change to '1' to invert the screen
            *(volatile cyg_uint32 *)PALMSW = 0xfedcba98;
            *(volatile cyg_uint32 *)PALLSW = 0x76543210;
#else
            *(volatile cyg_uint32 *)PALMSW = 0x01234567;
            *(volatile cyg_uint32 *)PALLSW = 0x89abcdef;
#endif
            break;
        }
        case 2:
        {
            ctl_word = LCDCON_GSEN;
            *(volatile cyg_uint32 *)PALLSW = 0xfa50fa50;
            *(volatile cyg_uint32 *)PALMSW = 0xfa50fa50;
            break;
        }
        case 1:
        {
            ctl_word = 0;
            *(volatile cyg_uint32 *)PALLSW = 0xf0f0f0f0;
            *(volatile cyg_uint32 *)PALMSW = 0xf0f0f0f0;
        }
    };

    // video buffer size
    ctl_word |= (( LCD_WIDTH * LCD_HEIGHT * LCD_BITS_PER_PIXEL ) / 128 - 1) << LCDCON_BUFSIZ_S;
    // line length
    ctl_word |= (( LCD_WIDTH / 16 ) - 1) << LCDCON_LINE_LENGTH_S;
    // pixel prescale
    /* See the cirrus documentation for LCDCON regarding 526628.  It depends on the clock frequency
       of the main bus: 36.864E6 for most cases.  This probably needs to be adjusted for 90Mhz operation. */
    ctl_word |= (( 526628 / (LCD_HEIGHT * LCD_WIDTH)) - 1) << LCDCON_PIX_PRESCALE_S;
   
    // AC frequency bias
    /* From the datasheets it looks like this value should be tDM, unfortunately that value
       is not specified. */
#if LCD_WIDTH == 320
    /* 0 was the previous value, however the colors are not very even, 33 gets a lot closer to having
       a smooth greyscale. -tm */
    ctl_word |= 33 << LCDCON_AC_PRESCALE_S;
#else
    ctl_word |= 13 << LCDCON_AC_PRESCALE_S;
#endif

    /* Configure the LCD controller */
    *(volatile cyg_uint32*) LCDCON = ctl_word;
   
    /* Power up LCD driver */
#if __DHARMA == 2
    SetMMGPO(0x00, MMGPO_LCD_POWER_ON);
#else /* __DHARMA == 1 */
    *(volatile cyg_uint8 *)PADDR |= 0x80;
    *(volatile cyg_uint8 *)PADR &= ~0x80;
#endif

    /* Wait between power up and enable */
#ifndef NOKERNEL
    cyg_thread_delay( 3 );
#else
    i = 1000;
    while(i > 0)
      i--;
#endif
    
    /* Enable display */
#if __DHARMA == 2
    SetMMGPO(MMGPO_LCD_ENABLE, 0x00);
#else /* __DHARMA == 1 */
    *(volatile cyg_uint8 *)PBDDR |= 0x08;
    *(volatile cyg_uint8 *)PBDR |= 0x08;
#endif
    
    *(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_LCDEN;

    return 0;
}

int
LCDDisable(void)
{
    *(volatile cyg_uint32 *)SYSCON1 &= ~SYSCON1_LCDEN;

    /* Disable display */
#if __DHARMA == 2
    SetMMGPO(0, MMGPO_LCD_ENABLE);
#else /* __DHARMA == 1 */
    *(volatile cyg_uint8 *)PBDDR |= 0x08;
    *(volatile cyg_uint8 *)PBDR &= ~0x08;
#endif

    /* Power down LCD driver */
#if __DHARMA == 2
    SetMMGPO(MMGPO_LCD_POWER_ON, 0);
#else /* __DHARMA == 1 */
    *(volatile cyg_uint8 *)PADDR |= 0x80;
    *(volatile cyg_uint8 *)PADR |= 0x80;
#endif    
    
    return 0;
}

#if defined(LCD_BACKLIGHT)
int
LCDSetBacklight(int Value)
{
    return 0;
}
#endif

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
LCDWriteRaw(const unsigned char* buf, int len ) 
{
    cyg_uint8* fb = (cyg_uint8*)LCD_FRAME_BUFFER_ADDRESS;
    int i;
  
#if (LCD_BITS_PER_PIXEL == 4)
    for( i = 0; i < len - 1; i += 2 ) {
        fb[i] = buf[i + 1];
        fb[i + 1] = buf[i];
    }
#else
    memcpy( (void*)LCD_FRAME_BUFFER_ADDRESS, buf, len );
#endif // if 1

    return 0;
}

// convert the 1 bpp buffer to 4 bpp, and invert the display
int 
LCDWriteInverted1to4(const unsigned char* buf, int len)
{
    cyg_uint8* fb = (cyg_uint8*)LCD_FRAME_BUFFER_ADDRESS;
    int i;

    unsigned char inmask = 0x80;
    unsigned char* indata = buf;
    unsigned char out1, out2;

    for( i = 0; i < len*4 - 1; i += 2 ) {
        if (!inmask)
        {
            inmask = 0x80;
            ++indata;
        }
        if (*indata & inmask)
            out2 = 0x0f;
        else out2 = 0xff;
        inmask >>= 1;
        if (*indata & inmask)
            out2 &= 0xf0;
        inmask >>= 1;
        
        if (*indata & inmask)
            out1 = 0x0f;
        else out1 = 0xff;
        inmask >>= 1;
        if (*indata & inmask)
            out1 &= 0xf0;
        inmask >>= 1;
        
        fb[i] = out1;
        fb[i + 1] = out2;
    }
    return 0;
}

int
LCDWriteInverted(const unsigned char* buf, int len ) 
{
    cyg_uint8* fb = (cyg_uint8*)LCD_FRAME_BUFFER_ADDRESS;
    int i;
  
    for( i = 0; i < len - 1; i += 2 ) {
	fb[i] = ~buf[i + 1];
	fb[i + 1] = ~buf[i];
    }
  
    return 0;
}

int
LCDPutPixel(unsigned char Color, int X, int Y) 
{

    volatile cyg_uint8 * FrameBuffer = (cyg_uint8 *)LCD_FRAME_BUFFER_ADDRESS;
    int Byte = (Y * LCD_WIDTH * LCD_BITS_PER_PIXEL) / 8;
    cyg_uint8 val;
    
    Byte += ((X * LCD_BITS_PER_PIXEL) / 8);

#if 1

    if( Byte & 0x01 ) Byte &= ~(0x01);
    else Byte |= 0x01;
    
    val = FrameBuffer[ Byte ];
    
    if( X & 0x01 ) {
        val &= 0xf0;
        val |= (Color & 0x0f);
    }
    else {
        val &= 0x0f;
        val |= (Color << 4);
    }
    FrameBuffer[ Byte ] = val;
    
#else
    int IsLowPixel;
    IsLowPixel = (X % 2);
    if (IsLowPixel) {
	Color <<= 4;
    }

    /* TODO This only works if you clear first */
    FrameBuffer[(Y * LCD_WIDTH) + (X / 2)] |= Color;
#endif
    
    return 0;
}
