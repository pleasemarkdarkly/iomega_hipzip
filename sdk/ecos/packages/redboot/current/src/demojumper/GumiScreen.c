//........................................................................................
//........................................................................................
//.. File Name: GumiScreen.cpp															..
//.. Date: 7/21/2000																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: CGumiScreen Class, Demojumper interface functions				 				..
//.. Usage: These functions control the screens displayed in the demojumper				..
//.. Last Modified By: Donni Reitz-Pesek	donni@iobjects.com							..	
//.. Modification date: 10/26/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <redboot.h>
#include <stdlib.h>
//#include <string.h>

//#include <cyg/kernel/kapi.h>
#include "demojumper/lcd.h"
#include "demojumper/GumiScreen.h"
#include "demojumper/GumiFonts.h"
#include "demojumper/bitmaps.h"
//#include <cyg/infra/diag.h>
#include "demojumper/debug.h"
#include "demojumper/gui_state.h"
#include "demojumper/demojumper.h"

#define SPLASH_SCREEN_TIME_ON   200

UCHAR GrayPalette[16 * 3] = {
0, 0, 0,
38, 38, 38,
75, 75, 75,
113, 113, 113,
14, 14, 14,
52, 52, 52,
89, 89, 89,
192, 192, 192,
128, 128, 128,
76, 76, 76,
150, 150, 150,
226, 226, 226,
28, 28, 28,
104, 104, 104,
178, 178, 178,
255, 255, 255
};

const UCHAR CF_FILL =    0x01;
UCHAR ucBuf[ DISPLAY_BUF_SIZE ];

void select_demo(void)
{
	GumiScreen_Construct();
	GumiScreen_Go();
}

void
GumiScreen_Go(void)
{
	
	// initialize the count of valid files
	m_ucNumFFiles = DEMO_COUNT;
	
	// set up dynamic arrays to allow flexible iterating henceforth (else no way to cleanly iterate through #define names)
	setup_demos();

//#define DUMP_DEMONAMES
#ifdef DUMP_DEMONAMES
	{
		int i;
		SHUNT_PRINT("Available demos:\n");
		for (i = 0; i < DEMO_COUNT; ++i)
		{
			SHUNT_PRINT("%s ('%s')\n", demo_names[i],demo_aliases[i]);
		}
	}
#endif

	// draw first menu image:
	GumiScreen_DrawMenu();
}


void
GumiScreen_DrawMenu(void)
{
	int i;
	static bool bFirst = true;
	// the space inside the firmware menu where the options are displayed
	static GumiRect grWindowSpace;
	static GumiRect grMenuText[MENU_ITEM_COUNT];
	static GumiPoint gpMenu[MENU_ITEM_COUNT];
	static GumiColor GC;
	static GumiRect GR;			// the status text area, this gr is (was) used to erase that region before drawing another

	if (bFirst)
	{
		grWindowSpace.wTop = MENU_BORDER_WIDTH-1;
		grWindowSpace.wBottom = LCD_HEIGHT - MENU_BORDER_WIDTH - 1;
		grWindowSpace.wLeft = MENU_BORDER_WIDTH - 1;
		grWindowSpace.wRight = LCD_WIDTH - MENU_BORDER_WIDTH - 1;
		
		for (i = 0; i < MENU_ITEM_COUNT; ++i)
		{
			grMenuText[i].wTop = grWindowSpace.wTop + i * ( MENU_ITEM_HEIGHT + MENU_ITEM_VSPACE );
			grMenuText[i].wBottom = grMenuText[i].wTop + MENU_ITEM_HEIGHT;
			grMenuText[i].wLeft = grWindowSpace.wLeft;
			grMenuText[i].wRight = grWindowSpace.wRight;

			gpMenu[i].x = grMenuText[i].wLeft;
			gpMenu[i].y = grMenuText[i].wTop;
		}			

		GC.uBackground = 0x00;
		GC.uForeground = 0xFF;
		GC.uFlags = 0;

		// todo: if we use this, fix the #'s
		GR.wTop = 34;
		GR.wBottom = 60;
		GR.wLeft = 7;
		GR.wRight = 90;

		for (i = 0; i < MENU_ITEM_COUNT; ++i)
		{
			if (i < DEMO_COUNT)
				g_psMenu[i] = demo_names[i];
		}

		if(m_ucNumFFiles)
			g_ucBottom = (m_ucNumFFiles > MENU_ITEM_COUNT) ? MENU_ITEM_COUNT-1 : m_ucNumFFiles - 1;

		// first draw the background
		LCDWriteRaw( ucDharmaBitmap , DISPLAY_BUF_SIZE );
		HAL_DELAY_US(100 * 1000);

		bFirst = false;
	}

	
	GumiScreen_ClearBuffer();

	// draw the two vertical side lines
	GumiScreen_VerticalLine(MENU_BORDER_TOP,MENU_BORDER_BOTTOM,MENU_BORDER_LEFT,GC.uForeground,1);
	GumiScreen_VerticalLine(MENU_BORDER_TOP,MENU_BORDER_BOTTOM,MENU_BORDER_RIGHT,GC.uForeground,1);

	// turn the arrows on and off
	if(g_ucTop > 0){
		// draw the arrow
		GumiScreen_DrawArrow(true, GC.uForeground);
		// (don't draw the top line)
		;
		// draw the tops of the two vertical side lines
		GumiScreen_VerticalLine(0, MENU_BORDER_TOP, MENU_BORDER_LEFT, GC.uForeground, 1);
		GumiScreen_VerticalLine(0, MENU_BORDER_TOP, MENU_BORDER_RIGHT, GC.uForeground, 1);
	}
	else{
		// (don't draw the arrow)
		;
		// draw the top line
		GumiScreen_HorizontalLine(MENU_BORDER_LEFT, MENU_BORDER_RIGHT, MENU_BORDER_TOP, GC.uForeground, 1);
		// (don't draw the tops of the two vertical side lines)
		;
	}
	
	if(g_ucBottom < (m_ucNumFFiles - 1)){
		// draw the bottom arrow
		GumiScreen_DrawArrow(false, GC.uForeground);
		// (don't draw the bottom line)
		;
	}
	else{
		// (don't draw the bottom arrow)
		;
		// draw the bottom line
		GumiScreen_HorizontalLine(MENU_BORDER_LEFT, MENU_BORDER_RIGHT, MENU_BORDER_BOTTOM, GC.uForeground, 1);
	}
	
	// blank out the client area

	GumiScreen_FillRect(grWindowSpace, GC.uBackground);
		
	// draw menu strings
	for (i = 0; i < MENU_ITEM_COUNT; ++i)
	{
		if(g_psMenu[i] != NULL)
			GumiScreen_DrawTextView(gpMenu[i], g_psMenu[i], GC, &VerdanaFont12, MENU_ITEM_MAX_CHARS, grMenuText[i]);
	}


	// highlight the currently selected menu option
	for(i = grWindowSpace.wLeft; i <= grWindowSpace.wRight; i++)
		GumiScreen_VerticalLineXOR(grWindowSpace.wTop+((MENU_ITEM_HEIGHT+MENU_ITEM_VSPACE)*g_ucHighlighted),
						grWindowSpace.wTop+((MENU_ITEM_HEIGHT+MENU_ITEM_VSPACE)*g_ucHighlighted) + MENU_ITEM_HEIGHT,
						i);
		
	// update the screen
	LCDWriteRaw(ucBuf, DISPLAY_BUF_SIZE);
}


/*--------------------------------------------------------------------------*/
// Constructor- initialize video memory addresses
/*--------------------------------------------------------------------------*/
void GumiScreen_Construct(void) //GumiRect &Rect)
{
	SIGNED iLoop;
	int i;
	UCHAR *CurrentPtr = GumiScreen_GetVideoAddress();
	mwHRes = LCD_WIDTH;
	mwVRes = LCD_HEIGHT;
//	miPitch = (mwVRes + 7) >> 3;
	m_bDemoSelected = false;
	
	for (iLoop = 0; iLoop < mwVRes; iLoop++)
	{
		mpScanPointers[iLoop] = CurrentPtr;
        CurrentPtr += mwHRes >> 1;
	}
	
	GumiScreen_ConfigureController();        // set up controller registers
	
#ifdef GRAYSCALE
    GumiScreen_SetPalette(0, 16, GrayPalette);
#else
    GumiScreen_SetPalette(0, 16, DefPalette256);
#endif

	for (i = 0; i < MENU_ITEM_COUNT; ++i)
	{
		g_psMenu[i] = NULL;
	}
}


void GumiScreen_SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet)
{
	WORD loop;

	UCHAR *pPut = muPalette;

	for (loop = 0; loop < iNum; loop++)
	{
		*pPut++ = *pGet++;
		*pPut++ = *pGet++;
		*pPut++ = *pGet++;
	}
}


/*--------------------------------------------------------------------------*/
// *** This function must be filled in by the developer ***
/*--------------------------------------------------------------------------*/
UCHAR *GumiScreen_GetVideoAddress(void)
{
	return ucBuf;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void GumiScreen_VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
{
    UCHAR uVal = 0xf0;

    if (wXPos & 1)
    {
        uVal >>= 4;
    }
    while (wYStart <= wYEnd)
    {
        UCHAR *Put = mpScanPointers[wYStart] + (wXPos >> 1);
        *Put ^= uVal;
        wYStart += 1;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void GumiScreen_DrawTextView(GumiPoint Where, const char *Text, GumiColor Color,
    GumiFont *Font, SIGNED iCount, GumiRect Rect)
{
   #ifdef PEG_UNICODE
    char *pCurrentChar = (char *) Text;
   #else
    UCHAR *pCurrentChar = (UCHAR *) Text;
   #endif
    UCHAR *pGetData;
    UCHAR *pGetDataBase;
    WORD  wBitOffset;
    SIGNED  wXpos = Where.x;
    WORD cVal = *pCurrentChar++;

    while(cVal && wXpos <= Rect.wRight)
    {
		SIGNED ScanRow;
		SIGNED  wCharWidth;
		WORD wOffset;
        WORD ByteOffset;
		if (iCount == 0)
        {
            return;
        }
        iCount--;
        wOffset = cVal - (WORD) Font->wFirstChar;
        wBitOffset = Font->pOffsets[wOffset];
        wCharWidth = Font->pOffsets[wOffset+1] - wBitOffset;

		if (wCharWidth < 0)
			wCharWidth = 10;
        if (wXpos + wCharWidth > Rect.wRight)
        {
            wCharWidth = Rect.wRight - wXpos + 1;
        }

        ByteOffset = Font->pOffsets[wOffset] / 8;
        pGetDataBase = Font->pData + ByteOffset;
        pGetDataBase += (Rect.wTop - Where.y) * Font->wBytesPerLine;

        for (ScanRow = Rect.wTop; ScanRow <= Rect.wBottom; ScanRow++)
        {
			UCHAR InMask;
			WORD wBitsOutput;
			UCHAR cData;
            pGetData = pGetDataBase;
            InMask = 0x80 >> (wBitOffset & 7);
            wBitsOutput = 0;
            cData = *pGetData++;

            while(wBitsOutput < wCharWidth)
            {
                if (!InMask)
                {
                    InMask = 0x80;
                    // read a byte:
                    cData = *pGetData++;
                }

                if (wXpos >= Rect.wLeft)
                {
                    if (cData & InMask)        // is this bit a 1?
                    {
#ifndef INVERT_BUF
                        PlotPointView(wXpos, ScanRow, Color.uForeground);
#else	// INVERT_BUF
                        PlotPointView(wXpos, ScanRow, Color.uBackground);
#endif	// INVERT_BUF
                    }
                    else
                    {
                        if (Color.uFlags & CF_FILL)
                        {
                            PlotPointView(wXpos, ScanRow, Color.uBackground);
                        }
                    }
                }
                InMask >>= 1;
                wXpos++;
                wBitsOutput++;
                if (wXpos > Rect.wRight)
                {
                    break;
                }
            }
            pGetDataBase += Font->wBytesPerLine;
			wXpos -= wCharWidth;
        }
        wXpos += wCharWidth;
        cVal = *pCurrentChar++;
    }
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void GumiScreen_HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos, UCHAR Color, SIGNED wWidth)
{
    UCHAR *Put;
    UCHAR uVal;
    UCHAR uFill = (UCHAR) (Color | (Color << 4));
#ifdef INVERT_BUF
	uFill = ~uFill;
#endif	// INVERT_BUF

    while(wWidth-- > 0)
    {
        SIGNED iLen = wXEnd - wXStart + 1;
        Put = mpScanPointers[wYPos] + (wXStart >> 1);

        // most compilers seem to do a good job of optimizing 
        // memset to do 32-bit data writes. If your compiler doesn't
        // make the most of your CPU, you might want to re-write this
        // in assembly.

        if (wXStart & 1)
        {
            uVal = *Put;
            uVal &= 0xf0;
#ifndef INVERT_BUF
            uVal |= Color;
#else
            uVal |= (0x0f & ~Color);
#endif	// INVERT_BUF
            *Put++ = uVal;
            iLen--;
        }
        if (iLen > 0)
        {
            memset(Put, uFill, iLen >> 1);

            if (!(wXEnd & 1))
            {
                uVal = *(Put + (iLen >> 1));
                uVal &= 0x0f;
                uVal |= uFill & 0xf0;
                *(Put + (iLen >> 1)) = uVal;
            }
        }
        wYPos++;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void GumiScreen_VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos, UCHAR Color, SIGNED wWidth)
{
    UCHAR uFill = (UCHAR) (Color | (Color << 4));
#ifdef INVERT_BUF
	uFill = ~uFill;
#endif	// INVERT_BUF

    while(wYStart <= wYEnd)
    {
        UCHAR *Put = mpScanPointers[wYStart] + (wXPos >> 1);
        SIGNED iLen = wWidth;

        if (wXPos & 1)
        {
            UCHAR uVal = *Put;
            uVal &= 0xf0;
#ifndef INVERT_BUF
            uVal |= Color;
#else
            uVal |= (0x0f & ~Color);
#endif	// INVERT_BUF
            *Put++ = uVal;
            iLen--;
        }

        if (iLen > 0)
        {
            memset(Put, uFill, iLen >> 1);

            if ((wXPos ^ iLen) & 1)
            {
                UCHAR uVal = *(Put + (iLen >> 1));
                uVal &= 0x0f;
                uVal |= uFill & 0xf0;
                *(Put + (iLen >> 1)) = uVal;
            }
        }
        wYStart++;
    }
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void GumiScreen_ConfigureController(void)
{
#ifdef PEGWIN32
#else
	LCDEnable();
	LCDClear();

	LCDSetBacklight(LCD_BACKLIGHT_ON);
#ifdef INVERT_BUF
	memset(ucBuf, 0xFF, DISPLAY_BUF_SIZE);
#else
	memset(ucBuf, 0x00, DISPLAY_BUF_SIZE);
#endif

#endif
}

void
GumiScreen_ClearBuffer(void)
{
	int i;
#ifndef INVERT_BUF
	UCHAR blank = 0x00;
#else	// INVERT_BUF
	UCHAR blank = 0xff;
#endif	// INVERT_BUF
	
	for(i = 0; i < DISPLAY_BUF_SIZE; i++)
		ucBuf[i] = blank;
}


void
GumiScreen_DrawArrow(bool bUpper, UCHAR color)
{
	SIGNED cX1 = MENU_ARROW_XPOS;		// x-from paramater, initially x-center of arrow, where the tip will be placed horizontally
	SIGNED cX2 = MENU_ARROW_XPOS;		// x-to paramater, initially x-center of arrow, where the tip will be placed horizontally
	SIGNED cY, cI;			// initial y-position for lines, to be initted to the y-pos of the arrow tip; increment parameter, depends on upper/lower arrow
	SIGNED i;
	if(bUpper){
		cY = MENU_ARROW_BORDER_WIDTH - 1;				// y-start of arrow tip (upper), one pixel from top of lcd
		cI = 1;				// incremental value for y, for the upper arrow will add one to the y-val each iteration
	}
	else{
		cY = LCD_HEIGHT - MENU_ARROW_BORDER_WIDTH - 1;	// initial y-val, one pixel from the bottommost row (bottommost == lcd_height-1)
		cI = -1;			// incremental value for y, for the upper arrow will add one to the y-val each iteration
	}
	for(i = 0; i <= 5; i++){						// iterate through 5 rows of arrow graphic
		GumiScreen_HorizontalLine(cX1-i, cX2+i, cY, color, 1);		// draw this line's row
		cY += cI;										// make the arrow-wedge wider and bump the y-value
	}
}


void
GumiScreen_FillRect(GumiRect Rect, UCHAR color)
{
	SIGNED i;
	for(i = Rect.wTop; i <= Rect.wBottom; i++)
		GumiScreen_HorizontalLine(Rect.wLeft, Rect.wRight, i, color, 1);
}

