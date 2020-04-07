// DJ lcd test
//
//


#include <devs/lcd/lcd.h>
#include "cmds.h"
#include "parser.h"

extern int LCDReverse(int Reverse);

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

// doesn't take any parameters
int test_lcd(char param_strs[][MAX_STRING_LEN],int* param_nums)
{
	int bOn = 1, bOff = 1;
	int x,y;
	if(param_strs[0][0])
	{
		if(strncmpci(param_strs[0],"ON",MAX_STRING_LEN) == 0)
		{	
			bOff = 0;
		}
		else if(strncmpci(param_strs[0],"OFF",MAX_STRING_LEN) == 0)
		{
			bOn = 0;		
		}
	}

			

	
#ifdef __DJ
	

	if(bOn)
	{
 		DEBUG2("LCD Test\n");

		LCDEnable();
		DEBUG3("LCD Enabled\n");

		LCDClear();
		DEBUG3("LCD Cleared\n");

		DEBUG3("LCDSetBacklight On\n");
		LCDSetBacklight(1);

	#if 0
		// diagonal pattern
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
	#endif


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
		

		// 3 magic boxes

		
		drawbox( LCD_WIDTH * .25, LCD_HEIGHT * .8 );
		drawbox( LCD_WIDTH * .5, LCD_HEIGHT * .6 );
		drawbox( LCD_WIDTH * .75, LCD_HEIGHT * .4 );

	}


	if(bOn && bOff)
	{
		cyg_thread_delay(100);
			
		DEBUG3("LCDReverse On\n");
		LCDReverse(0);
		cyg_thread_delay(100);

		DEBUG3("LCDReverse Off\n");
		LCDReverse(1);
		cyg_thread_delay(100);


		DEBUG3("LCDSetBacklight Off\n");
		LCDSetBacklight(0);
	}


	if(bOff)
	{

		LCDClear();
		LCDDisable();
		DEBUG3("LCD Disabled\n");

	}

	#else
		DEBUG2("LCD Test Disabled\n")
	#endif

	
	return TEST_OK_PASS;
}
