// ks0713.c: lcd driver for the Samsung KS0713 controller on the
//           H-Tech G128064-52 LCD Module.  
// danb@iobjects.com 10/09/01
// (c) Interactive Objects


#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/infra/diag.h>
#include <devs/lcd/lcd.h>

// uncomment to reverse the display (reverse black and white)
#define REVERSE_DISPLAY


#ifdef __POGO
#define LCD_CONTROL_ADDRESS 0x30000000
#define LCD_DATA_ADDRESS 0x30000000
// PB6 defines DATA/COMMAND mode - RS high is DATA mode
#define LCD_DATA_ENABLE *(volatile cyg_uint8 *)PBDR |= 0x40;
#define LCD_COMMAND_ENABLE *(volatile cyg_uint8 *)PBDR &= ~0x40;
#else /* DHARMA TEST LCD */
#define LCD_CONTROL_ADDRESS 0x40000000
#define LCD_DATA_ADDRESS 0x40000001
// no-ops on dharma, done with address lines
#define LCD_DATA_ENABLE 
#define LCD_COMMAND_ENABLE
#endif

#define LCD_NUM_PAGES 8
#define LCD_NUM_COLUMNS 128

void
LCDCommand1(cyg_uint8 Command)
{
  LCD_COMMAND_ENABLE
 
  while (*(volatile cyg_uint8 *)LCD_CONTROL_ADDRESS & 0x80);

    *(volatile cyg_uint8 *)LCD_CONTROL_ADDRESS = Command;
  
  LCD_DATA_ENABLE

}

void
LCDCommand2(cyg_uint8 Command1, cyg_uint8 Command2) 
{
  LCD_COMMAND_ENABLE
 

   while (*(volatile cyg_uint8 *)LCD_CONTROL_ADDRESS & 0x80);
   *(volatile cyg_uint8 *)LCD_CONTROL_ADDRESS = Command1;
    *(volatile cyg_uint8 *)LCD_CONTROL_ADDRESS = Command2;

  LCD_DATA_ENABLE
}

int
LCDEnable(void)
{

 
#ifdef __POGO
  /* Set the bus access to the LCD */
  *(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
  *(volatile cyg_uint32 *)MEMCFG1 |= 0x13000000; //  8 bit, 4 ws (with 16-bit boot flash)
//  *(volatile cyg_uint32 *)MEMCFG1 |= 0x03000000; //  8 bit, 8 ws (with 16-bit boot flash)
  
  // PB6 is an output for data/command select on the lcd
  *(volatile cyg_uint8 *)PBDDR |= 0x40;
#else
  *(volatile cyg_uint32 *)MEMCFG2 &= ~0x000000ff;
  *(volatile cyg_uint32 *)MEMCFG2 |=  0x00000002 | ((4) << 2); /* 8 bit , 4ws*/
  
  /* Set up PE1 for the Backlight */
  *(volatile cyg_uint8 *)PEDDR |= 0x02;
#endif

  /* Come Out of Sleep Mode */
  /* Entire Display OFF (this is the normal mode) */
  LCDCommand1(0xa4);
  /* Set Static Indicator and Register */
  LCDCommand2(0xad, 0x00);
  /* Display ON */
  LCDCommand1(0xaf);
  
  /* Reset */
  LCDCommand1(0xe2);
  

  
  /* Application setup */
      /* ADC/SHL setup */
   /* ADC select */
  LCDCommand1(0xa0);  /* Normal (0xa0), Reverse (0xa1) */
    /* SHL select */
  LCDCommand1(0xc0);  /* Normal (0xc0), Reverse (0xc8) */
	/* LCD Bias Select */
  LCDCommand1(0xa3);  /* 1/9 */

    /* LCD power setup */
     /* Power control */
  LCDCommand1(0x2f);
    /* Regulator resistor select, this value was found through experimentation */
  LCDCommand1(0x26);
	/* Reference Voltage Select:  A two byte instruction */
  LCDCommand2(0x81, 0xB);
  
#ifdef REVERSE_DISPLAY
  /* Reverse Display */
  LCDCommand1(0xa7);
#endif

    /* Enable display */
    LCDCommand1(0xaf);

    
    return 0;
}


int
LCDDisable(void) 
{

#if 0
	/* Turn off the Backlight */
	LCDSetBacklight(0);
	/* Release PE1 from the Backlight*/
	*(volatile cyg_uint8 *)PEDDR &= ~0x02;
#endif 

	/* Power Save (Compound Instruction) */
    /* Display OFF */
    LCDCommand1(0xae);
	/* Entire Display ON */
	LCDCommand1(0xa5);

    return 0;
}

int
LCDReverse(int Reverse) 
{
	if (Reverse == 1)
		/* Reverse Display */
		LCDCommand1(0xa7);
	else
		LCDCommand1(0xa6);

    return 0;
}

int
LCDSetBacklight(int Value) 
{

#if 0 // backlight control disabled for testing
	if (Value == 1)
		*(volatile cyg_uint8 *)PEDR |= 0x02;
	else
		*(volatile cyg_uint8 *)PEDR &= ~0x02;
#endif

    return 0;
}

int
LCDClear(void) 
{
    cyg_uint8 PageAddress = 0;
    cyg_uint8 ColumnAddress = 0;

    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {

		/* Set page address */
		/* Valid page addresses for dot-matrix display area are 0 - 9
		   Valid page address for displaying icons is 10
		   Addresses 11 - 15 are invalid */
		LCDCommand1((0xb0 | PageAddress));
    
		/* Set column address */
		/* Valid column addresses are 0 - 127 */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		for (ColumnAddress = 0; ColumnAddress < LCD_NUM_COLUMNS; ++ColumnAddress) {
			
			/* Write data */
#ifdef REVERSE_DISPLAY
			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = 0xff;
#else

			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = 0x00;
#endif
	    
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
	
    /* Read data */
    LCDCommand1((0xb0 | PageAddress));
	
    /* Set column address */
    LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
	
//	hal_delay_us( 250 );    
	
	/* Dummy read */
    Data = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;
	
    /* Actual read */
    Data = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;
	
    /* Set page address */
    LCDCommand1((0xb0 | PageAddress));
    
    /* Set column address */
    LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
	
    /* Write data */
#ifdef REVERSE_DISPLAY
    if (Color == 1) {
#else
    if (Color == 0) {
#endif
		*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = Data & ~(1<<PageOffset);
    }
    else {
		*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = Data | (1<<PageOffset);
    }
    
    return 0;
}

/* The bytes get written (vertically) in the following format:
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
    int i;
    
    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) {
		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		for (i = 0; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1)- PageAddress)];
			//cyg_thread_delay(1); // a debug delay to show the drawing slowly
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
		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		for (i = 0; i < (LCD_NUM_COLUMNS * LCD_NUM_PAGES); i += LCD_NUM_PAGES) {
			
			/* Write data */
			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = Buffer[i + ((LCD_NUM_PAGES - 1)- PageAddress)] ^ 0xff;
		}
    }
	
    return 0;
}

int
LCDMemoryTest()
{

	// read/write test on LCD framebuffer memory
	cyg_uint8 PageAddress;
    cyg_uint8 ColumnAddress;
    int i;
	cyg_uint8 data = 0xFF;
	cyg_uint8 test;

	// write a page at a time
    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) 
	{
		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		// write sample data
		for(i = 0; i < LCD_NUM_COLUMNS; i++)
		{		
			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = data;
		}

		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

		// dummy read
		test = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;

		// read and compare sample data for this page
		for(i = 0; i < LCD_NUM_COLUMNS; i++)
		{			

			test = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;

			if(test != data)
			{
				diag_printf("Memory test failed on data %x != test %x, page %d, column %d\n",data,test,PageAddress,i);
				return 1;
			}
		}

	}


	data = 0x00;


	// write a page at a time
    for (PageAddress = 0; PageAddress < LCD_NUM_PAGES; ++PageAddress) 
	{
		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));
		
		// write sample data
		for(i = 0; i < LCD_NUM_COLUMNS; i++)
		{		
			*(volatile cyg_uint8 *)LCD_DATA_ADDRESS = data;
		}

		
		/* Set page address */
		LCDCommand1((0xb0 | PageAddress));
		
		/* Set column address */
		ColumnAddress = 0;
		LCDCommand2((0x10 | (ColumnAddress >> 4)), (ColumnAddress & 0x0f));

		// dummy read
		test = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;

		// read and compare sample data for this page
		for(i = 0; i < LCD_NUM_COLUMNS; i++)
		{			

			test = *(volatile cyg_uint8 *)LCD_DATA_ADDRESS;

			if(test != data)
			{
				diag_printf("Memory test failed on data %x != test %x, page %d, column %d\n",data,test,PageAddress,i);
				return 1;
			}
		}

	}

	diag_printf("LCD mem test worked\n");

	return 0;


}