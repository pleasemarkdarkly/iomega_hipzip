/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// monoscrn.hpp - Linear screen driver for 1-bpp (monochrome) operation.
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
//  This screen driver is intended for use with monochrome systems that support
//  a linear frame buffer. Example controllers include the SMOS 135xx series,
//  the MPC823, SC300, and many others.
//
//  This driver can be tested under Win32 by turning on the definition
//  PEGWIN32. You can safely delete all code bracketed by the PEGWIN32
//  definition when moving to your target.
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _MONOSCREEN_
#define _MONOSCREEN_

#include <devs/lcd/_lcd_hw.h>

/*--------------------------------------------------------------------------*/
// Default resolution and color depth
/*--------------------------------------------------------------------------*/
#define PEG_VIRTUAL_XSIZE LCD_WIDTH
#define PEG_VIRTUAL_YSIZE LCD_HEIGHT
#define ROTSCRATCH_SIZE 10240

#define PlotPointView(x, y, c)\
    {\
    UCHAR *_Put = mpScanPointers[y] + (x >> 3);\
    UCHAR _uShift = ( x & 7);\
    UCHAR _uVal = *_Put & ~(0x80 >> _uShift);\
    _uVal |= c << (7 - _uShift);\
    *_Put = _uVal;\
    }

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
class MonoScreen : public PegScreen
{
    public:
       #ifndef PEGWIN32
        MonoScreen(PegRect &);
       #else
        MonoScreen(HWND hwnd, PegRect &);
        SIGNED mwWinRectXOffset;
        SIGNED mwWinRectYOffset;
       #endif

        virtual ~MonoScreen();

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
        void SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *Get);
        UCHAR *GetPalette(DWORD *pPutSize);
        void ResetPalette(void);
        COLORVAL GetPixelView(SIGNED x, SIGNED y)
        {
            UCHAR *_Get = mpScanPointers[y] + (x >> 3);
            UCHAR _uShift = 7 - (x & 7);
            UCHAR _uVal = *_Get;
            _uVal >>= _uShift;
            _uVal &= 1;
            return ((COLORVAL) _uVal);
        }

        #ifdef PEG_IMAGE_SCALING
        COLORVAL GetBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap)
        {
            WORD wBytesPerLine = (pMap->wWidth + 7) >> 3;
            COLORVAL *pGet = pMap->pStart;
            pGet += wBytesPerLine * y;
            pGet += x >> 3;
            UCHAR _uVal = *pGet;
            UCHAR _uShift = 7 - (x & 7);
            _uVal >>= _uShift;
            _uVal &= 1;
            return ((COLORVAL) _uVal);
        }
        void PutBitmapPixel(SIGNED x, SIGNED y, PegBitmap *pMap, COLORVAL cVal)
        {
            WORD wBytesPerLine = (pMap->wWidth + 7) >> 3;
            COLORVAL *pPut = pMap->pStart;
            pPut += wBytesPerLine * y;
            pPut += x >> 3;
            UCHAR _uShift = 7 - (x & 7);
            UCHAR _uVal = *pPut;
            _uVal &= ~(0x80 >> _uShift);
            _uVal |= cVal << (7 - _uShift);
            *pPut = _uVal;
        }
        #endif

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
        void DrawFastBitmap(const PegPoint Where, const PegBitmap *Getmap,
            const PegRect &View);
        void DrawUnalignedBitmap(const PegPoint Where, const PegBitmap *Getmap,
            const PegRect &View);
        UCHAR  muPalette[2 * 3];
        SIGNED miPitch;

       #ifdef PEGWIN32

        void MemoryToScreen(void);
        HWND mHWnd;
        HPALETTE mhPalette;
        HDC  mHdc;
       #else
        void MemoryToScreen(void);
       #endif
        unsigned char m_rotscratch[ROTSCRATCH_SIZE];
};

#endif



