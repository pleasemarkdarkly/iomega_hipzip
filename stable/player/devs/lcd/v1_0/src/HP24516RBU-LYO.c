// HP24516RBU-LYO.c: driver for the HP24516RBU-LYO lcd module (two Samsung S6B0724 controllers)
// danb@iobjects.com 10/25/01
// (c) Interactive Objects
 
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>
#include <devs/lcd/lcd.h>
#include <util/utils/utils.h>
// uncomment to reverse the display (reverse black and white)
#define REVERSE_DISPLAY

USE_HW_LOCK(PBDR);

/* both systems use nCS4 for chip select */
/* DJ uses PB4 to change between data and address */
#ifdef __DJ 
#define LCD_MA_CONTROL_ADDRESS 0x40000001
#define LCD_MA_DATA_ADDRESS 0x40000001
#define LCD_SL_CONTROL_ADDRESS 0x40000000
#define LCD_SL_DATA_ADDRESS 0x40000000
/* assumes PB4 high = Command select */
#define LCD_ADDRESS_ENABLE *(volatile cyg_uint8 *)PBDR &= ~0x10; Wait(WAIT_1US*10);
#define LCD_DATA_ENABLE  *(volatile cyg_uint8 *)PBDR |= 0x10; Wait(WAIT_1US*10);
#else /* DHARMA */
#define LCD_MA_CONTROL_ADDRESS 0x40000001
#define LCD_MA_DATA_ADDRESS 0x40000003
#define LCD_SL_CONTROL_ADDRESS 0x40000000
#define LCD_SL_DATA_ADDRESS 0x40000002
#define LCD_ADDRESS_ENABLE 
#define LCD_DATA_ENABLE 
#endif /* __DJ */


#define LCD_NUM_PAGES 8
#define LCD_NUM_COLUMNS 240

#define LCD_MA_COLUMNS 132
#define LCD_SL_COLUMNS 108

// 90mhz adjusted timings
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#define WAIT_1MS 6050
#define WAIT_1US 6
#else
#define WAIT_1MS 5000
#define WAIT_1US 5
#endif

/* Local Wait Routine */
static void 
Wait(unsigned int cnt)	
{
    unsigned long i;
    
    i = cnt; 
    while(i--)
        ;
}

static void
LCD_MA_Command1(cyg_uint8 Command)
{
	int timeout = 10000;
  LCD_ADDRESS_ENABLE
 
      while ((*(volatile cyg_uint8 *)LCD_MA_CONTROL_ADDRESS & 0x80) && (timeout > 0))
		  timeout--;    		

    *(volatile cyg_uint8 *)LCD_MA_CONTROL_ADDRESS = Command;
  LCD_DATA_ENABLE


}

static void
LCD_MA_Command2(cyg_uint8 Command1, cyg_uint8 Command2) 
{
		int timeout = 10000;
  LCD_ADDRESS_ENABLE 

      while ((*(volatile cyg_uint8 *)LCD_MA_CONTROL_ADDRESS & 0x80) && (timeout > 0))
		  timeout--;    		

    *(volatile cyg_uint8 *)LCD_MA_CONTROL_ADDRESS = Command1;
    *(volatile cyg_uint8 *)LCD_MA_CONTROL_ADDRESS = Command2;
    LCD_DATA_ENABLE 

}

static void
LCD_SL_Command1(cyg_uint8 Command)
{
		int timeout = 10000;
  LCD_ADDRESS_ENABLE
      while ((*(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS & 0x80) && (timeout > 0))
		  timeout--;    		

    *(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS = Command;
  LCD_DATA_ENABLE

}

static void
LCD_SL_Command2(cyg_uint8 Command1, cyg_uint8 Command2) 
{
		int timeout = 10000;
  LCD_ADDRESS_ENABLE

        while ((*(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS & 0x80) && (timeout > 0))
		  timeout--; 

    *(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS = Command1;
    *(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS = Command2;
    LCD_DATA_ENABLE

}

void LCDPrintStatus(void) {

  LCD_ADDRESS_ENABLE
    //  Wait(WAIT_1MS*2);
  diag_printf("\nLCD: Status = %x\n",*(volatile cyg_uint8 *)LCD_SL_CONTROL_ADDRESS);
  //  Wait(WAIT_1MS*2);
  LCD_DATA_ENABLE

}
 
int
LCDEnable(void)
{
  

	HW_LOCK(PBDR);

    /* Data always enabled, Address explicitly enables/disables*/
    LCD_DATA_ENABLE  
            
    /* Come Out of Sleep Mode */
    /* Entire Display OFF (this is the normal mode) */
    LCD_MA_Command1(0xa4);
    /* Set Static Indicator and Register */
    LCD_MA_Command2(0xad, 0x00);
    
    /* Reset */
    LCD_MA_Command1(0xe2);
    LCD_SL_Command1(0xe2);
    
    
    /* Application setup */
    
    /* ADC select */
    LCD_MA_Command1(0xa0);  /* Normal */
    LCD_SL_Command1(0xa0);  /* Normal */
    /* SHL select */
    LCD_MA_Command1(0xc8);  /* Normal */
    LCD_SL_Command1(0xc8);  /* Normal */
    /* LCD Bias Select */
    LCD_MA_Command1(0xa3);  /* 1/9 */
    
    /* LCD power setup */
    
    /* Voltage Converter ON */
    LCD_MA_Command1(0x2c);
    /* Wait 2 ms */
    Wait(WAIT_1MS*2);
    /* Voltage Regulator ON */
    LCD_MA_Command1(0x2e);
    /* Wait 2 ms */
    Wait(WAIT_1MS*2);
    /* Voltage Voltage Follower ON */
    LCD_MA_Command1(0x2f);
    /* Regulator Resistor Select, this value was found through experimentation */
    LCD_MA_Command1(0x24);  /* 0x24 is the default, higher number is a darker black*/
    /* Reference Voltage Select:  A two byte instruction */
    LCD_MA_Command2(0x81, 0x22);  /* 0x81, 0x20 is the default */
    
#ifdef REVERSE_DISPLAY
    /* Reverse Display */
    LCD_MA_Command1(0xa7);
    LCD_SL_Command1(0xa7);
#endif
    
    /* Enable display */
    LCD_MA_Command1(0xaf);

  
	HW_UNLOCK(PBDR);
  return 0;
}


int
LCDDisable(void) 
{

	/* Turn off the Backlight */
	LCDClear();
	LCDSetBacklight(0);


	HW_LOCK(PBDR);

	/* Power Save (Compound Instruction) */
    /* Display OFF */
    LCD_MA_Command1(0xae);
	/* Entire Display ON */
	LCD_MA_Command1(0xa5);

	HW_UNLOCK(PBDR);
    return 0;
}

int
LCDReverse(int Reverse) 
{
	HW_LOCK(PBDR);
	if (Reverse == 1)
	{
		/* Reverse Display */
		LCD_MA_Command1(0xa7);
		LCD_SL_Command1(0xa7);
	}
	else
	{
		LCD_MA_Command1(0xa6);
		LCD_SL_Command1(0xa6);
	}
	HW_UNLOCK(PBDR);
    return 0;
}

int
LCDSetBacklight(int Value) 
{
	

	// default behavior
#ifdef __DJ
	
	HW_LOCK(PBDR);

	if (Value == 1)
		*(volatile cyg_uint8 *)PBDR |= 0x02;
	else
		*(volatile cyg_uint8 *)PBDR &= ~0x02;

	HW_UNLOCK(PBDR);

	return 0;
#else
	if (Value == 1)
		*(volatile cyg_uint8 *)PEDR |= 0x02;
	else
		*(volatile cyg_uint8 *)PEDR &= ~0x02;
	return 0;
#endif /* __DJ */

}

int
LCDClear(void) 
{
    cyg_uint8 PageAddress = 0;
    cyg_uint8 ColumnAddress = 0;
#ifdef REVERSE_DISPLAY
	cyg_uint8 ClearColor = 0xff;
#else
	cyg_uint8 ClearColor = 0x00;
#endif

	HW_LOCK(PBDR);

    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {
		
		/* Set page address */
		/* Valid page addresses for dot-matrix display area are 0 - 7 */
		LCD_MA_Command1((0xb0 | PageAddress));
		LCD_SL_Command1((0xb0 | PageAddress));
		
		/* Set column address */
		/* Valid column addresses are 0 - 132 */
		ColumnAddress = 0;
		LCD_MA_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		LCD_SL_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		// write to the master controller
		for (ColumnAddress = 0; ColumnAddress < LCD_MA_COLUMNS; ++ColumnAddress) {
			*(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS = ClearColor;
		}
		
		// write to the slave controller
		for (ColumnAddress = 0; ColumnAddress < LCD_SL_COLUMNS; ++ColumnAddress) {
			*(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS = ClearColor;
		}
		
		/* Optional status read */
    }
    HW_UNLOCK(PBDR);
    return 0;
}

int
LCDPutPixel(unsigned char Color, int X, int Y)
{
    cyg_uint8 PageAddress;
    cyg_uint8 PageOffset;
    cyg_uint8 ColumnAddress;
    cyg_uint8 Data;
    
	HW_LOCK(PBDR);
    /* Check boundaries on X */
    if ((X < 0) || (X >= LCD_NUM_COLUMNS)) {
		return -1;
    }
	
    /* Check boundaries on Y */
    if ((Y < 0) || (Y >= (LCD_NUM_PAGES*8))) {
		return -1;
    }
	
    /* Translate X and Y into page and column numbers */
    PageAddress = Y / 8;
    PageOffset = Y % 8;
    ColumnAddress = X;
	
	if (ColumnAddress < LCD_MA_COLUMNS)
	{
		/* Read data */
		LCD_MA_Command1((0xb0 | PageAddress));
		
		/* Set column address */
		LCD_MA_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		/* Dummy read */
		Data = *(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS;
		
		/* Actual read */
		Data = *(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS;
		
		/* Set page address */
		LCD_MA_Command1((0xb0 | PageAddress));
    
		/* Set column address */
		LCD_MA_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		/* Write data */
#ifdef REVERSE_DISPLAY
		if (Color == 1) {
#else
		if (Color == 0) {
#endif
			*(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS = Data & ~(1<<PageOffset);
		}
		else {
			*(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS = Data | (1<<PageOffset);
		}
	}
	else if (ColumnAddress < LCD_NUM_COLUMNS)
	{
		/* Adjust the Column address since we're talking to the second controller */
		ColumnAddress -= LCD_MA_COLUMNS;

		/* Read data */
		LCD_SL_Command1((0xb0 | PageAddress));
		
		/* Set column address */
		LCD_SL_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		/* Dummy read */
		Data = *(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS;
		
		/* Actual read */
		Data = *(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS;
		
		/* Set page address */
		LCD_SL_Command1((0xb0 | PageAddress));
    
		/* Set column address */
		LCD_SL_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		/* Write data */
#ifdef REVERSE_DISPLAY
		if (Color == 1) {
#else
		if (Color == 0) {
#endif
			*(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS = Data & ~(1<<PageOffset);
		}
		else {
			*(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS = Data | (1<<PageOffset);
		}
	}
	HW_UNLOCK(PBDR);
	return 0;
}

/* The bytes get written (vertically) in the following order:
     0   1   2   3   4   5 ... ...
   128 129 130 131 132 ... ... ...
   256 257 258 259 ... ... ... ...
   384 385 386 ... ... ... ... ...
   512 ... ... ... ... ... ... ...
   640 ... ... ... ... ... ... ...
   768 ... ... ... ... ... ... ...
   896 ... ... ... ... ... ... ...
*/
int
LCDWriteRaw(const unsigned char * Buffer, int Size) 
{

    cyg_uint8 PageAddress;
    cyg_uint8 ColumnAddress;
	int SlaveBufferOffset;
    int i;
    	
	HW_LOCK(PBDR);

	SlaveBufferOffset = LCD_MA_COLUMNS * LCD_NUM_PAGES;

    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {
		
		/* Set page address */
		LCD_MA_Command1((0xb0 | PageAddress));
		LCD_SL_Command1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCD_MA_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		LCD_SL_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		// write to the first (master) controller
		for (i = 0; i < (LCD_MA_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1) - PageAddress)];
			//cyg_thread_delay(1); // a debug delay to show the drawing slowly
		}

		// write to the second (slave) conroller
		for (i = SlaveBufferOffset; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1) - PageAddress)];
			//cyg_thread_delay(1); // a debug delay to show the drawing slowly
		}
    }
	HW_UNLOCK(PBDR);
    return 0;
}

int
LCDWriteInverted(const unsigned char * Buffer, int Size)
{
	
    cyg_uint8 PageAddress;
    cyg_uint8 ColumnAddress;
	int SlaveBufferOffset;
    int i;
    
	HW_LOCK(PBDR);
	SlaveBufferOffset = LCD_MA_COLUMNS * LCD_NUM_PAGES;

    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {
		
		/* Set page address */
		LCD_MA_Command1((0xb0 | PageAddress));
		LCD_SL_Command1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCD_MA_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		LCD_SL_Command2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		// write to the first (master) controller
		for (i = 0; i < (LCD_MA_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_MA_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1) - PageAddress)] ^ 0xff;
			//cyg_thread_delay(1); // a debug delay to show the drawing slowly
		}

		// write to the second (slave) conroller
		for (i = SlaveBufferOffset; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_SL_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1) - PageAddress)] ^ 0xff;
			//cyg_thread_delay(1); // a debug delay to show the drawing slowly
		}
    }
	HW_UNLOCK(PBDR);
    return 0;
}
