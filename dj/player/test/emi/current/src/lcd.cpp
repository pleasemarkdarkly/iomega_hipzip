// DJ lcd test
//
//


#include "lcd.h"


cyg_mutex_t mLED,mLCD;
bool s_bLED,s_bLCD;

extern "C"
{
extern int LCDReverse(int Reverse);
};

//! Turn off the LED.
void
LEDOff()
{
	
	if(s_bLED)
		*(volatile cyg_uint8 *)PBDR &= 0xF3;
}

//! Turn the LED red.
void
LEDRed()
{
	cyg_mutex_lock(&mLED);
	if(s_bLED)
	{
		*(volatile cyg_uint8 *)PBDR &= 0xF7;
		*(volatile cyg_uint8 *)PBDR |= 0x04;
	}
	cyg_mutex_unlock(&mLED);
}

//! Turn the LED green.
void
LEDGreen()
{
	cyg_mutex_lock(&mLED);
	if(s_bLED)
	{
		*(volatile cyg_uint8 *)PBDR &= 0xFB;
		*(volatile cyg_uint8 *)PBDR |= 0x08;
	}
	cyg_mutex_unlock(&mLED);
}

//! Turn the LED orange.
void
LEDOrange()
{
	cyg_mutex_lock(&mLED);
	if(s_bLED)
		*(volatile cyg_uint8 *)PBDR |= 0x0C;
	cyg_mutex_unlock(&mLED);
}


/* FUNCTIONS */

// center square at point x of size
int drawbox(int centerx, int size)
{
	int startx, starty,x ,y;

	starty = ((LCD_HEIGHT - size) / 2); // + size;
	startx = centerx - (size/2);
	
	for(x = startx; x < (startx + size); x++)
		for(y = starty; y < (starty + size); y++)
			LCDPutPixel(1,x,y);

}

extern void ATAUIWait();
extern "C"
{
extern void NETUIWait();
};


void ToggleLCD()
{
	cyg_mutex_lock(&mLCD);

	if(s_bLCD)
	{
		diag_printf("LCD Off\n");
		LCDSetBacklight(0);
		LCDClear();
		LCDDisable();
		s_bLCD = false;
	}
	else
	{
		diag_printf("LCD On\n");
		LCDEnable();
		LCDClear();
		LCDSetBacklight(1);
		s_bLCD = true;
	}
	
	cyg_mutex_unlock(&mLCD);
}

void ToggleLED()
{
	cyg_mutex_lock(&mLED);

	if(s_bLED)
	{
		diag_printf("LED Off\n");
		LEDOff();
		s_bLED = false;
	}
	else
	{
		diag_printf("LED On\n");
		s_bLED = true;
	}

	cyg_mutex_unlock(&mLED);
}

// doesn't take any parameters
void lcd_thread(cyg_uint32)
{
	
	cyg_mutex_init(&mLED);
	cyg_mutex_init(&mLCD);

	int x,y;
 
	LCDEnable();
	LCDClear();
	LCDSetBacklight(1);

	s_bLCD = true;
	s_bLED = true;

	while(1)
	{

		cyg_mutex_lock(&mLCD);
		// diagonal pattern
		if(s_bLCD)
		{
			for (x = 0; x < LCD_WIDTH; ++x) {
				for (y = 0; y < LCD_HEIGHT; ++y) {
					if((x + y) % 8 == 0)
					{
						LCDPutPixel(1,x,y);
					}
					else
					{
						LCDPutPixel(0,x,y);
					}

				}
			}
		}
		cyg_mutex_unlock(&mLCD);

		LEDOff();

		// GATE
		ATAUIWait();

		cyg_mutex_lock(&mLCD);
		if(s_bLCD)
		{
			// draw 2 pixel borders
			for( x = 0; x < LCD_WIDTH; ++x)
			{
				LCDPutPixel(1,x,LCD_HEIGHT - 1);
				LCDPutPixel(1,x,LCD_HEIGHT - 2);
			}

			for( x = 0; x < LCD_WIDTH; ++x)
			{
				LCDPutPixel(1,x,0);
				LCDPutPixel(1,x,1);
			}

			for( y = 0; y < LCD_HEIGHT; ++y)
			{
				LCDPutPixel(1,LCD_WIDTH - 1,y);
				LCDPutPixel(1,LCD_WIDTH - 2,y);
			}

			for( y = 0; y < LCD_HEIGHT; ++y)
			{
				LCDPutPixel(1,0,y);
				LCDPutPixel(1,1,y);
			}
		}
		cyg_mutex_unlock(&mLCD);
		
		LEDRed();

		cyg_mutex_lock(&mLCD);
		// 3 magic boxes	
		if(s_bLCD)
		{
			drawbox( LCD_WIDTH * .25, LCD_HEIGHT * .8 );
			drawbox( LCD_WIDTH * .5, LCD_HEIGHT * .6 );
			drawbox( LCD_WIDTH * .75, LCD_HEIGHT * .4 );
		}

		cyg_mutex_unlock(&mLCD);


		cyg_thread_delay(100);
	
		// GATE
		NETUIWait();
		
		cyg_mutex_lock(&mLCD);
		if(s_bLCD)
			LCDReverse(0);
		cyg_mutex_unlock(&mLCD);

		LEDGreen();
		cyg_thread_delay(100);
		
		cyg_mutex_lock(&mLCD);
		if(s_bLCD)
			LCDReverse(1);
		cyg_mutex_unlock(&mLCD);

		LEDOrange();
		cyg_thread_delay(100);

		
	
	}


	LCDSetBacklight(0);
	LCDClear();
	LCDDisable();
	
	

}
