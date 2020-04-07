//........................................................................................
//........................................................................................
//.. File Name: GumiScreen.h															..
//.. Date: 7/21/2000																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: class CGumiScreen and some lcd settings					..
//.. Usage: Control functions for demojumper GUI										..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef GUMISCRN_H_
#define GUMISCRN_H_

#include "demojumper/lcd.h"
#include "demojumper/gumi.h"
#include <cyg/infra/cyg_type.h>

#define GRAYSCALE
#define PROFILE_MODE        // always leave this turned on

/*--------------------------------------------------------------------------*/
// Default resolution and color depth
/*--------------------------------------------------------------------------*/

#define VIRTUAL_XSIZE LCD_WIDTH
#define VIRTUAL_YSIZE LCD_HEIGHT

#define PlotPointView(x, y, c)\
{\
    UCHAR *_Put = mpScanPointers[y] + (x >> 1);\
    UCHAR _uVal = *_Put;\
    if (x & 1)\
    {\
        _uVal &= 0xf0;\
        _uVal |= c;\
    }\
    else\
    {\
        _uVal &= 0x0f;\
        _uVal |= c << 4;\
    }\
    *_Put = _uVal;\
}

//........................................................................................
//.. Go																					..
//.. Purpose: Starts the firmware upgrade menu											..
//.. Expected Parameters: none															..
//.. Return Value(s): none																..
//........................................................................................
		void GumiScreen_Go(void);

//........................................................................................
//.. DrawMenu																				..
//.. Purpose: Loads the menu and preps for a choice										..
//.. Expected Parameters: none															..
//.. Return Value(s): none																..
//........................................................................................
        void GumiScreen_DrawMenu(void);

//........................................................................................
//.. DoDadioScreen																		..
//.. Purpose: Display dadio screen														..
//.. Expected Parameters: none															..
//.. Return Value(s): none																..
//........................................................................................
//        void GumiScreen_DoDadioScreen(void);
		
//........................................................................................
//.. DoVersionNumber																	..
//.. Purpose: Display the version number on screen										..
//.. Expected Parameters: none															..
//.. Return Value(s):  none																..
//........................................................................................
//        void GumiScreen_DoVersionNumber(void);

//........................................................................................
//.. DoBootloaderNumber																	..
//.. Purpose: Display the demojumper version number on screen							..
//.. Expected Parameters: none															..
//.. Return Value(s):  none																..
//........................................................................................
//        void GumiScreen_DoBootloaderNumber(void);

//........................................................................................
//.. DoLoadingText																		..
//.. Purpose: Displays series of text messages before loading image						..
//.. Expected Parameters: none															..
//.. Return Value(s): none																..
//........................................................................................
//        void GumiScreen_DoLoadingText(void);

//........................................................................................
//.. DoWarningScreenUpdates																..
//.. Purpose:  Displays series of text messages while loading image						..
//.. Expected Parameters: one of four key values declared in demojumper.h				..
//.. Return Value(s): none																..
//........................................................................................
//        void GumiScreen_DoWarningScreenUpdates(int key);
        
//        SIGNED miPitch;
        UCHAR m_ucNumFFiles;
    
        UCHAR *GumiScreen_GetVideoAddress(void);
        void GumiScreen_ConfigureController(void);
        void GumiScreen_ClearBuffer(void);
        void GumiScreen_DrawArrow(bool bUpper, UCHAR color);
        void GumiScreen_FillRect(GumiRect Rect, UCHAR color);
		void GumiScreen_SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet);

        void GumiScreen_DrawTextView(GumiPoint Where, const char *Text, GumiColor Color,
            GumiFont *Font, SIGNED iCount, GumiRect Rect);
        void GumiScreen_VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos);
        void GumiScreen_HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos,
            UCHAR Color, SIGNED wWidth);
        void GumiScreen_VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos,
            UCHAR Color, SIGNED wWidth);

		UCHAR  muPalette[16 * 3];
		UCHAR* mpScanPointers[LCD_HEIGHT];
        SIGNED mwHRes;
        SIGNED mwVRes;
        bool m_bSoftwareAlreadyPresent;
		bool m_bDemoSelected;
        void GumiScreen_Construct(void);  

#endif // GUMISCRN_H_



