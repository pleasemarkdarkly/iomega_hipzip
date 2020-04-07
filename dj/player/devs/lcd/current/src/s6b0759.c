#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>
#include <devs/lcd/lcd.h>

#define LCD_BASE_ADDRESS 0x40000000
//#define LCD_NUM_PAGES 8//10
//#define LCD_NUM_COLUMNS 98//128
#define LCD_NUM_PAGES 10
#define LCD_NUM_COLUMNS 128

#define ENTER_LCD_CONTROL_MODE() *(volatile cyg_uint8 *)PEDR &= ~0x02;
#define ENTER_LCD_DATA_MODE() *(volatile cyg_uint8 *)PEDR |= 0x02;

void
LCDCommand1(cyg_uint8 Command)
{
    while (*(volatile cyg_uint8 *)0x40000000 & 0x80)
	;
    *(volatile cyg_uint8 *)0x40000000 = Command;
}

void
LCDCommand2(cyg_uint8 Command1, cyg_uint8 Command2) 
{
    while (*(volatile cyg_uint8 *)0x40000000 & 0x80)
	;
    *(volatile cyg_uint8 *)0x40000000 = Command1;
    *(volatile cyg_uint8 *)0x40000000 = Command2;
}

int
LCDEnable(void)
{
    /* Set the bus access to the LCD */
    *(volatile cyg_uint32 *)MEMCFG2 &= ~0x000000ff;
    *(volatile cyg_uint32 *)MEMCFG2 |= 0x00000002 | ((4) << 2); /* 8 bit bus access, 4 wait states */
    
    /* Set up the address lead */
    *(volatile cyg_uint8 *)PEDDR |= 0x02;

    ENTER_LCD_CONTROL_MODE();
    
    /* Application setup */
    
    /* Display duty select */
    /* ADC select */
    /* SHL select */
    /* COM0 register select */

    /* LCD power setup */

    /* Oscillator on */
    LCDCommand1(0xab);
    /* DC-DC step up register select, 5 times boosting circuit */
    LCDCommand1(0x66);
    /* Regulator resistor select, this value was found through experimentation */
    LCDCommand1(0x24);
    /* Electronic volume register select, this value was found through experimentation */
    LCDCommand2(0x81, 0x20);
    /* LCD bias register select, 1/8 */
    LCDCommand1(0x54);
    /* Power control */
    LCDCommand1(0x2f);

    /* Enable display */
    LCDCommand1(0xaf);
    
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
    /* No backlight on this display */
    return -1;
}

int
LCDClear(void) 
{
    cyg_uint8 PageAddress = 0;
    cyg_uint8 ColumnAddress = 0;

    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {

	ENTER_LCD_CONTROL_MODE();
    
	/* Set page address */
	/* Valid page addresses for dot-matrix display area are 0 - 9
	   Valid page address for displaying icons is 10
	   Addresses 11 - 15 are invalid */
	LCDCommand1((0xb0 | PageAddress));
    
	/* Set column address */
	/* Valid column addresses are 0 - 127 */
	ColumnAddress = 0;
	LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
	
	ENTER_LCD_DATA_MODE();
	
	for (ColumnAddress = 0; ColumnAddress < 127; ++ColumnAddress) {
	    
	    /* Write data */
	    *(volatile cyg_uint8 *)LCD_BASE_ADDRESS = 0x00;
	    
	    /* Column address is incremented */
	    /* If write not done then write again */
	}
	
	/* Optional status read */
    }
    
    return 0;
}

int
LCDPutPixel(unsigned char Color, int X, int Y)
{
    cyg_uint8 PageAddress;
    cyg_uint8 PageOffset;
    cyg_uint8 ColumnAddress;
    cyg_uint8 Data;
    
    /* Check boundaries on X */
    if ((X < 0) || (127 < X)) {
	return -1;
    }

    /* Check boundaries on Y */
    if ((Y < 0) || (79 < Y)) {
	return -1;
    }

    /* Translate X and Y into page and column numbers */
    PageAddress = Y / 8;
    PageOffset = Y % 8;
    ColumnAddress = X;

    ENTER_LCD_CONTROL_MODE();

    /* Read data */
    LCDCommand1((0xb0 | PageAddress));

    /* Set column address */
    LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

    ENTER_LCD_DATA_MODE();
    
    /* Dummy read */
    Data = *(volatile cyg_uint8 *)LCD_BASE_ADDRESS;

    /* Actual read */
    Data = *(volatile cyg_uint8 *)LCD_BASE_ADDRESS;

    ENTER_LCD_CONTROL_MODE();
    
    /* Set page address */
    LCDCommand1((0xb0 | PageAddress));
    
    /* Set column address */
    LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

    ENTER_LCD_DATA_MODE();

    /* Write data */
    if (Color == 0) {
	*(volatile cyg_uint8 *)LCD_BASE_ADDRESS = Data & ~(1<<PageOffset);
    }
    else {
	*(volatile cyg_uint8 *)LCD_BASE_ADDRESS = Data | (1<<PageOffset);
    }
    
    return 0;
}

/* The bytes get written in the following format:
   0 10 20
   1 11 21
   2 12 ..
   3 ..
   4 ..
   5 ..
   6 ..
   7 ..
   8 ..
   9 ..
   This is the same as the HipZip LCD */
int
LCDWriteRaw(const unsigned char * Buffer, int Size) 
{
    cyg_uint8 PageAddress;
    cyg_uint8 ColumnAddress;
    int i;
    
    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {

	ENTER_LCD_CONTROL_MODE();
	
	/* Set page address */
	LCDCommand1((0xb0 | PageAddress));
	
	/* Set column address */
	ColumnAddress = 0;
	LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

	ENTER_LCD_DATA_MODE();
	
	for (i = 0; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
	    
	    /* Write data */
	    *(volatile cyg_uint8 *)LCD_BASE_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1)- PageAddress)];
	}
    }

    return 0;
}

int
LCDWriteInverted(const unsigned char * Buffer, int Size)
{
    cyg_uint8 PageAddress;
    cyg_uint8 ColumnAddress;
    int i;
    
    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {

	ENTER_LCD_CONTROL_MODE();
	
	/* Set page address */
	LCDCommand1((0xb0 | PageAddress));
	
	/* Set column address */
	ColumnAddress = 0;
	LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

	ENTER_LCD_DATA_MODE();
	
	for (i = 0; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
	    
	    /* Write data */
	    *(volatile cyg_uint8 *)LCD_BASE_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1)- PageAddress)] ^ 0xff;
	}
    }

    return 0;
}
