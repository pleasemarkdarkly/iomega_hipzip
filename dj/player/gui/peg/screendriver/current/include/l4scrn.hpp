/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// l4scrn.hpp - Linear screen driver for 4-bpp (16 color) operation.
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-2000 Swell Software 
//              All Rights Reserved.
//
// Unauthorized redistribution of this source code, in whole or part,
// without the express written permission of Swell Software
// is strictly prohibited.
//
// Notes:
//
//  This screen driver is intended for use with 16-color systems that support
//  a linear frame buffer. Example controllers include the SMOS 135xx series
//  and the MPC823.
//
//  This driver can be tested under Win32 by turning on the definition
//  PEGWIN32. You can safely delete all code bracketed by the PEGWIN32
//  definition when moving to your target.
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _L4SCREEN_
#define _L4SCREEN_

#include <devs/lcd/_lcd_hw.h>

/*--------------------------------------------------------------------------*/
// Default resolution and color depth
/*--------------------------------------------------------------------------*/
#define PEG_VIRTUAL_XSIZE LCD_WIDTH
#define PEG_VIRTUAL_YSIZE LCD_HEIGHT

#define GRAYSCALE

/*--------------------------------------------------------------------------*/
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


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
class L4Screen : public PegScreen
{
    public:
       #ifndef PEGWIN32
        L4Screen(PegRect &);
       #else
        L4Screen(HWND hwnd, PegRect &);
        SIGNED mwWinRectXOffset;
        SIGNED mwWinRectYOffset;
       #endif

        virtual ~L4Screen();

        //---------------------------------------------------------
        // Public pure virtual functions in PegScreen:
        //---------------------------------------------------------

        void BeginDraw(PegThing *);
        void BeginDraw(PegThing *, PegBitmap *);
        void EndDraw(void);
        void EndDraw(PegBitmap *);
        void SetPointerType(UCHAR bType);
        void HidePointer(void);
        void SetPointer(PegPoint);
        void Capture(PegCapture *Info, PegRect &Rect);
        COLORVAL GetPixelView(SIGNED x, SIGNED y)
        {
            UCHAR *_Get = mpScanPointers[y] + (x >> 1);
            UCHAR _uVal = *_Get;
            if (x & 1)
            {
                _uVal &= 0x0f;
            }
            else
            {
                _uVal >>= 4;
            }
            return ((COLORVAL) _uVal);

        }

        #ifdef PEG_IMAGE_SCALING

        COLORVAL GetBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap)
        {
            WORD wBytesPerLine = (pMap->wWidth + 1) >> 1;
            COLORVAL *pGet = pMap->pStart;
            pGet += wBytesPerLine * y;
            pGet += x >> 1;
            UCHAR uVal = *pGet;
            if (x & 1)
            {
                uVal &= 0x0f;
            }
            else
            {
                uVal >>= 4;
            }
            return (COLORVAL) uVal;
        }
        void PutBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap, COLORVAL cVal)
        {
            WORD wBytesPerLine = (pMap->wWidth + 1) >> 1;
            COLORVAL *pPut = pMap->pStart;
            pPut += wBytesPerLine * y;
            pPut += x >> 1;

            UCHAR uVal = *pPut;
            if (x & 1)
            {
                uVal &= 0xf0;
                uVal |= (UCHAR) cVal;
            }
            else
            {
                uVal &= 0x0f;
                uVal |= (UCHAR) cVal << 4;
            }
            *pPut = (COLORVAL) uVal;
        }
        #endif

        void SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *Get);
        UCHAR *GetPalette(DWORD *pPutSize);
        void ResetPalette(void);

        //---------------------------------------------------------
        // End public pure virtual functions.
        //---------------------------------------------------------

        virtual PegBitmap *CreateBitmap(SIGNED wWidth, SIGNED wHeight);

    protected:
        
        BOOL mbPointerHidden;
        PegCapture mCapture;
        PegPoint  mLastPointerPos;
    
        UCHAR *GetVideoAddress(void);
        void ConfigureController(void);

    private:

        //---------------------------------------------------------
        // Protected pure virtual functions in PegScreen:
        //---------------------------------------------------------

        void DrawTextView(PegPoint Where, const TCHAR *Text, PegColor &Color,
            PegFont *Font, SIGNED iCount, PegRect &Rect);
        void LineView(SIGNED xStart, SIGNED yStart, SIGNED xEnd, SIGNED yEnd,
            PegRect &Rect, PegColor Color, SIGNED wWidth);
        void BitmapView(const PegPoint Where, const PegBitmap *Getmap,
            const PegRect &View);
        void RectMoveView(PegThing *Caller, const PegRect &View,
            const SIGNED xMove, const SIGNED yMove);

        void DrawRleBitmap(const PegPoint Where, const PegRect View,
            const PegBitmap *Getmap);
        void HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos,
            COLORVAL Color, SIGNED wWidth);
        void VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos,
            COLORVAL Color, SIGNED wWidth);
        void HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos);
        void VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos);

        //---------------------------------------------------------
        // End protected pure virtual functions.
        //---------------------------------------------------------

        // functions for drawing to off-screen bitmaps, using linear addressing:
        void Draw16ColorBitmap(const PegPoint Where, const PegBitmap *Getmap,
            const PegRect &View);
        void DrawUnaligned16ColorBitmap(const PegPoint Where, const PegBitmap *Getmap,
            const PegRect &View);


       #ifdef PEGWIN32

        void MemoryToScreen(void);
        UCHAR  muPalette[16 * 3];
        HWND mHWnd;
        HPALETTE mhPalette;
        HDC  mHdc;
       #else
		void MemoryToScreen(void);
		UCHAR  muPalette[16 * 3];
       #endif
};

#endif



