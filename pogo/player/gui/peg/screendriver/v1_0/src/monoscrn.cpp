/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// monoscrn.cpp - PegScreen driver template for 1 bit-per-pixel
//      linear frame buffer access. This driver can be configured
//      to work with any screen resolution, simply by passing in the correct
//      rectangle to the constructor. 
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
// There are conditional sections of code delineated by #ifdef PEGWIN32. This
// is used ONLY for testing the driver, or for development under WIN32 using
// this driver before the target hardware is ready. These sections of code
// can be removed to improve readability without affecting your target system.
//
// This driver is intended for systems which have direct, linear (i.e. not
// paged) access to the video frame buffer. This driver does not implement
// double buffering, although that could be added.
//
// All available configuration flags are found in the monoscrn.hpp header file.
//
//      *****************  IMPORTANT  ********************
//
// In order to use this driver in your system, you need to fill in the 
// following functions:
//
// 1) The ConfigureController() function, at the end of this file. This
//    function should initialize the video controller registers to properly
//    driver the display screen, and to properly access the video buffer.
//
// 2) The GetVideoAddress() function, to return a pointer
//    to the video frame buffer. This is target system dependant.
//
// 3) The SetPalette() function, to program the palette registers (if any).
//    The initial pallet programmed is BLACK, DARKGRAY, LIGHTGRAY, WHITE.
//
// Known Limitations:
// 
// This driver implements off-screen drawing using the native 1-bpp format.
// This means that transparency information is lost if a bitmap is drawn
// to the off-screen buffer. The tradeoff here is that off-screen drawing
// uses less memory, while supporting transparency in off-screen drawing
// would quadruple the memory requirements for the off-screen bitmap.
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include "stdlib.h"
#include "string.h"
#include <gui/peg/peg.hpp>
#include <devs/lcd/lcd.h>

UCHAR DefPalette[2*3] = {
0, 0, 0,        // black
255, 255, 255   // white
};

/*--------------------------------------------------------------------------*/
#define DISPLAY_BUF_SIZE	LCD_HEIGHT * LCD_WIDTH / 8
static UCHAR s_ucBuf[ DISPLAY_BUF_SIZE ];


#ifdef PEG_RUNTIME_COLOR_CHECK

// Extra bitmaps for monochrome mode. These bitmaps are only installed when
// run-time color depth determination is used, and the screen driver is
// monochrome. The screen driver must replace the above bitmaps with
// those shown below.


ROMDATA UCHAR ucMonoSysButtonBitmap[] = {
0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x0f,
0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
};


ROMDATA UCHAR ucMonoMixPatternBitmap[] = {
0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00,
0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f,
0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00,
0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f, 0x00, 0x0f,
};

ROMDATA UCHAR ucMonoHandBitmap[] = {
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x00,0x00,0xff,
0x00,0x00,0x00,0xff,0xff,0xff,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,
0x00,0x0f,0x00,0x00,0xff,0xff,0x00,0x0f,0x0f,0x00,0x0f,0x0f,0x00,0x0f,0x00,0x00,0x0f,0x00,0x00,
0xff,0x00,0x0f,0x00,0x00,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x0f,0x00,0x00,
0xff,0x00,0x0f,0x0f,0x00,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,
0xff,0xff,0x00,0x0f,0x0f,0x00,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,
0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,
0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,
0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,
0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,
0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,
0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
0xff,0xff,0xff,0xff,0xff,0xff,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xff,
};

extern PegBitmap gbSysButtonBitmap;
extern PegBitmap gbMixPatternBitmap;
extern PegBitmap gbHandBitmap;

COLORVAL MonoColors[16] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1};

#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// CreatePegScreen- Called by startup code to instantiate the PegScreen
// class we are going to run with.
/*--------------------------------------------------------------------------*/
#ifndef PEGWIN32
PegScreen *CreatePegScreen(void)
{
    PegRect Rect;
    Rect.Set(0, 0, PEG_VIRTUAL_XSIZE - 1, PEG_VIRTUAL_YSIZE - 1);
    PegScreen *pScreen = new MonoScreen(Rect);
    return pScreen;
}
#endif

/*--------------------------------------------------------------------------*/
// Constructor- initialize video memory addresses
/*--------------------------------------------------------------------------*/
#ifdef PEGWIN32
MonoScreen::MonoScreen(HWND hWnd, PegRect &Rect) : PegScreen(Rect)
#else
MonoScreen::MonoScreen(PegRect &Rect) : PegScreen(Rect)
#endif
{
    mdNumColors = 2;  

   #ifdef PEGWIN32
    mhPalette = NULL;
   #endif

    mwHRes = Rect.Width();
    mwVRes = Rect.Height();

    mpScanPointers = new UCHAR PEGFAR *[Rect.Height()];
    miPitch = mwHRes >> 3;

   #ifdef PEGWIN32
    
    // Windows bitmaps must be modulo-4 byte in width:
    miPitch += 3;
    miPitch &= 0xfffc;

   #endif

    UCHAR PEGFAR *CurrentPtr = GetVideoAddress();

    for (SIGNED iLoop = 0; iLoop < Rect.Height(); iLoop++)
    {
        mpScanPointers[iLoop] = CurrentPtr;
        CurrentPtr += miPitch;
    }

    mLastPointerPos.x = Rect.Width() / 2;
    mLastPointerPos.y = Rect.Height() / 2;
    mbPointerHidden = FALSE;
    mwDrawNesting = 0;

    ConfigureController();        // set up controller registers

    SetPalette(0, 2, DefPalette);

    #ifdef PEG_RUNTIME_COLOR_CHECK

    BLACK = MonoColors[0];
    RED = MonoColors[1];        
    GREEN = MonoColors[2];
    BROWN = MonoColors[3];        
    BLUE = MonoColors[4];        
    MAGENTA = MonoColors[5];
    CYAN = MonoColors[6];
    LIGHTGRAY = MonoColors[7];  
    DARKGRAY = MonoColors[8];   
    LIGHTRED = MonoColors[9];   
    LIGHTGREEN = MonoColors[10]; 
    YELLOW = MonoColors[11];
    LIGHTBLUE = MonoColors[12];  
    LIGHTMAGENTA = MonoColors[13];
    LIGHTCYAN = MonoColors[14];  
    WHITE = MonoColors[15];

    gbSysButtonBitmap.pStart = (UCHAR *) ucMonoSysButtonBitmap;
    gbMixPatternBitmap.pStart = (UCHAR *) ucMonoMixPatternBitmap;
    gbHandBitmap.pStart = (UCHAR *) ucMonoHandBitmap;

    #endif

#ifdef PEGWIN32

   // Some setup stuff for the BitBlitting function:
   mHWnd = hWnd;
   RECT lSize;
   ::GetClientRect(mHWnd, &lSize);

   mwWinRectXOffset = (lSize.right - mwHRes) / 2;
   mwWinRectYOffset = (lSize.bottom -mwVRes) / 2;
#endif

}




/*--------------------------------------------------------------------------*/
// *** This function must be filled in by the developer ***
/*--------------------------------------------------------------------------*/
UCHAR PEGFAR *MonoScreen::GetVideoAddress(void)
{

#ifdef PEGWIN32
    DWORD dSize = mwHRes / 8;       // width in bytes

    // must be a multiple of 4 bytes wide:
    dSize += 3;
    dSize &= 0xfffffffc;
    dSize *= mwVRes;
    
    UCHAR *pMem = new UCHAR[dSize];
    return pMem; 

#else
    return s_ucBuf;
#endif
}


/*--------------------------------------------------------------------------*/
// Destructor
/*--------------------------------------------------------------------------*/
MonoScreen::~MonoScreen()
{
    #ifdef PEGWIN32

    delete [] mpScanPointers[0];

    #endif

    delete mpScanPointers;
}

/*--------------------------------------------------------------------------*/
void MonoScreen::BeginDraw(PegThing *)
{
    LOCK_PEG
    if (!mwDrawNesting)
    {
       #ifdef PEGWIN32
        mHdc = GetDC(mHWnd);
       #endif

       #ifdef PEG_MOUSE_SUPPORT
        if (miInvalidCount)
        {
            if (mInvalid.Overlap(mCapture.Pos()))
            {
                HidePointer();
                mbPointerHidden = TRUE;
            }
        }
       #endif
    }
    mwDrawNesting++;
}


/*--------------------------------------------------------------------------*/
void MonoScreen::BeginDraw(PegThing *Caller, PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        return;
    }
    LOCK_PEG
    mpSaveScanPointers = mpScanPointers;

    if (pMap->wHeight && pMap->wWidth && pMap->pStart)
    {
        miPitch = (pMap->wWidth + 7) >> 3;

        mpScanPointers = new UCHAR PEGFAR *[pMap->wHeight];
        UCHAR PEGFAR *CurrentPtr = pMap->pStart;
        for (SIGNED iLoop = 0; iLoop < pMap->wHeight; iLoop++)
        {
            mpScanPointers[iLoop] = CurrentPtr;
            CurrentPtr += miPitch;
        }
        mVirtualRect.Set(0, 0, pMap->wWidth - 1, pMap->wHeight - 1);
        mbVirtualDraw = TRUE;
    }
}

/*--------------------------------------------------------------------------*/
void MonoScreen::EndDraw()
{
    mwDrawNesting--;

    if (!mwDrawNesting)
    {
       #ifdef PEG_MOUSE_SUPPORT
        if (mbPointerHidden)
        {
            SetPointer(mLastPointerPos);
            mbPointerHidden = FALSE;
        }
       #endif

       #ifdef PEGWIN32
        MemoryToScreen();
        ReleaseDC(mHWnd, mHdc);

       #else
        MemoryToScreen();

       #endif
        while(miInvalidCount > 0)
        {
            miInvalidCount--;
            UNLOCK_PEG
        }
    }
    UNLOCK_PEG
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::EndDraw(PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        mbVirtualDraw = FALSE;
        delete [] mpScanPointers;
        mpScanPointers = mpSaveScanPointers;
        miPitch = mwHRes >> 3;
        UNLOCK_PEG
    }
}

/*--------------------------------------------------------------------------*/
// CreateBitmap: The default version creates an 8-bpp bitmap.
/*--------------------------------------------------------------------------*/
PegBitmap *MonoScreen::CreateBitmap(SIGNED wWidth, SIGNED wHeight)
{
   PegBitmap *pMap = NULL;

    if (wWidth && wHeight)
    {
        pMap = new PegBitmap;
        pMap->wWidth = wWidth;
        pMap->wHeight = wHeight;
        wWidth += 7;
        wWidth >>= 3;
        pMap->pStart = new UCHAR[(DWORD) wWidth * (DWORD) wHeight];
        if (!pMap->pStart)
        {
            delete pMap;
            return NULL;
        }
        // fill the whole thing with 0x00:
        memset(pMap->pStart, 0x00, (DWORD) wWidth * (DWORD) wHeight);
        pMap->uFlags = 0;   // raw format
        pMap->uBitsPix = 1;
    }
    return pMap;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::LineView(SIGNED wXStart, SIGNED wYStart, SIGNED wXEnd, 
    SIGNED wYEnd, PegRect &Rect, PegColor Color, SIGNED wWidth)
{
    if (wYStart == wYEnd)
    {
        HorizontalLine(Rect.wLeft, Rect.wRight, Rect.wTop, Color.uForeground, wWidth);
        return;
    }
    if (wXStart == wXEnd)
    {
        VerticalLine(Rect.wTop, Rect.wBottom, Rect.wLeft, Color.uForeground, wWidth);
        return;
    }

    SIGNED dx = abs(wXEnd - wXStart);
    SIGNED dy = abs(wYEnd - wYStart);

    if (((dx >= dy && (wXStart > wXEnd)) ||
        ((dy > dx) && wYStart > wYEnd)))
    {
        PEGSWAP(wXEnd, wXStart);
        PEGSWAP(wYEnd, wYStart);
    }

    SIGNED y_sign = ((int) wYEnd - (int) wYStart) / dy;
    SIGNED x_sign = ((int) wXEnd - (int) wXStart) / dx;
    SIGNED decision;

    SIGNED wCurx, wCury, wNextx, wNexty, wpy, wpx;

    if (dx >= dy)
    {
        for (wCurx = wXStart, wCury = wYStart, wNextx = wXEnd,
             wNexty = wYEnd, decision = (dx >> 1);
             wCurx <= wNextx; wCurx++, wNextx--, decision += dy)
        {
            if (decision >= dx)
            {
                decision -= dx;
                wCury += y_sign;
                wNexty -= y_sign;
            }
            for (wpy = wCury - wWidth / 2;
                 wpy <= wCury + wWidth / 2; wpy++)
            {
                if (wCurx >= Rect.wLeft &&
                    wCurx <= Rect.wRight &&
                    wpy >= Rect.wTop &&
                    wpy <= Rect.wBottom)
                {
                    PlotPointView(wCurx, wpy, Color.uForeground);
                }
            }

            for (wpy = wNexty - wWidth / 2;
                 wpy <= wNexty + wWidth / 2; wpy++)
            {
                if (wNextx >= Rect.wLeft &&
                    wNextx <= Rect.wRight &&
                    wpy >= Rect.wTop &&
                    wpy <= Rect.wBottom)
                {
                    PlotPointView(wNextx, wpy, Color.uForeground);
                }
            }
        }
    }
    else
    {
        for (wCurx = wXStart, wCury = wYStart, wNextx = wXEnd,
                wNexty = wYEnd, decision = (dy >> 1);
            wCury <= wNexty; wCury++, wNexty--, decision += dx)
        {
            if (decision >= dy)
            {
                decision -= dy;
                wCurx += x_sign;
                wNextx -= x_sign;
            }
            for (wpx = wCurx - wWidth / 2;
                 wpx <= wCurx + wWidth / 2; wpx++)
            {
                if (wpx >= Rect.wLeft &&
                    wpx <= Rect.wRight &&
                    wCury >= Rect.wTop &&
                    wCury <= Rect.wBottom)
                {
                    PlotPointView(wpx, wCury, Color.uForeground);
                }
            }

            for (wpx = wNextx - wWidth / 2;
                 wpx <= wNextx + wWidth / 2; wpx++)
            {
                if (wpx >= Rect.wLeft &&
                    wpx <= Rect.wRight &&
                    wNexty >= Rect.wTop &&
                    wNexty <= Rect.wBottom)
                {
                    PlotPointView(wpx, wNexty, Color.uForeground);
                }
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR *Put;
    UCHAR uVal;
    UCHAR uFill = 0;

    if (Color)
    {
        uFill = 0xff;
    }
    if (wXEnd < wXStart)
    {
        return;
    }
                        
    while(wWidth-- > 0)
    {
        SIGNED iLen = wXEnd - wXStart + 1;
        Put = mpScanPointers[wYPos] + (wXStart >> 3);

        switch(wXStart & 7)
        {
        case 1:
            if (iLen >= 7)
            {
                uVal = *Put;
                uVal &= 0x80;
                uVal |= uFill & 0x7F;
                *Put++ = uVal;
                iLen -= 7;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXStart + iLen, wYPos, Color);
                }
                wYPos++;
                continue;
            }
            break;

        case 2:
            if (iLen >= 6)
            {
                uVal = *Put;
                uVal &= 0xc0;
                uVal |= uFill & 0x3f;
                *Put++ = uVal;
                iLen -= 6;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXStart + iLen, wYPos, Color);
                }
                wYPos++;
                continue;
            }
            break;

        case 3:
            if (iLen >= 5)
            {
                uVal = *Put;
                uVal &= 0xe0;
                uVal |= uFill & 0x1f;
                *Put++ = uVal;
                iLen -= 5;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXStart + iLen, wYPos, Color);
                }
                wYPos++;
                continue;
            }
            break;

        case 4:
            if (iLen >= 4)
            {
                uVal = *Put;
                uVal &= 0xf0;
                uVal |= uFill & 0x0f;
                *Put++ = uVal;
                iLen -= 4;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXStart + iLen, wYPos, Color);
                }
                wYPos++;
                continue;
            }
            break;

        case 5:
            if (iLen >= 3)
            {
                uVal = *Put;
                uVal &= 0xf8;
                uVal |= uFill & 0x07;
                *Put++ = uVal;
                iLen -= 3;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXStart + iLen, wYPos, Color);
                }
                wYPos++;
                continue;
            }
            break;

        case 6:
            if (iLen >= 2)
            {
                uVal = *Put;
                uVal &= 0xfc;
                uVal |= uFill & 0x03;
                *Put++ = uVal;
                iLen -= 2;
            }
            else
            {
                PlotPointView(wXStart, wYPos, Color);
                wYPos++;
                continue;
            }
            break;

        case 7:
            PlotPointView(wXStart, wYPos, Color);
            Put++;
            iLen--;
            break;

        default:
            break;
        }

        // most compilers seem to do a good job of optimizing 
        // memset to do 32-bit data writes. If your compiler doesn't
        // make the most of your CPU, you might want to re-write this
        // in assembly.

        if (iLen > 0)
        {
            memset(Put, uFill, iLen >> 3);
            Put += iLen >> 3;

            switch(wXEnd & 7)
            {
            case 6:
                uVal = *Put;
                uVal &= 0x01;
                uVal |= uFill & 0xfe;
                *Put = uVal;
                break;
                
            case 5:
                uVal = *Put;
                uVal &= 0x03;
                uVal |= uFill & 0xfc;
                *Put = uVal;
                break;
                
            case 4:
                uVal = *Put;
                uVal &= 0x07;
                uVal |= uFill & 0xf8;
                *Put = uVal;
                break;

            case 3:
                uVal = *Put;
                uVal &= 0x0f;
                uVal |= uFill & 0xf0;
                *Put = uVal;
                break;
                
            case 2:
                uVal = *Put;
                uVal &= 0x1f;
                uVal |= uFill & 0xe0;
                *Put = uVal;
                break;
                
            case 1:
                uVal = *Put;
                uVal &= 0x3f;
                uVal |= uFill & 0xc0;
                *Put = uVal;
                break;
                
            case 0:
                uVal = *Put;
                uVal &= 0x7f;
                uVal |= uFill & 0x80;
                *Put = uVal;
                break;
            }
        }
        wYPos++;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR uVal, uShift, uMask;
    COLORVAL uFill;
    UCHAR *Put;

    while(wWidth--)
    {
        uShift = (wXPos & 7);
        uFill = Color << (7 - uShift);
        uMask = 0x80 >> uShift;
        Put = mpScanPointers[wYStart] + (wXPos >> 3);
        SIGNED iLen = wYEnd - wYStart;

        while(iLen-- >= 0)
        {
            uVal = *Put;
            uVal &= ~uMask;
            uVal |= uFill;
            *Put = uVal;
            Put += miPitch;
        }
        wXPos++;
    }
}

#define SOLID_XOR
#ifdef SOLID_XOR

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
    UCHAR *Put = mpScanPointers[wYPos] + (wXStart >> 3);
    SIGNED iLen = wXEnd - wXStart;

    if (!iLen)
    {
        return;
    }
    // xor the left odd bits
    if (wXStart & 7)
    {
        short bit = wXStart & 7;
        UCHAR mask = 0x80 >> bit;
        while (bit<8)
        {
            *Put ^= mask;
            ++bit;
            mask >>= 1;
        }
        ++Put;
        wXStart += 8-(wXStart & 7);
    }
    // xor the center span
    while(wXStart < wXEnd - 7)
    {
        *Put++ ^= 0xFF;
        wXStart += 8;
    }
    // xor the right odd bits
    short bit = wXStart&7;
    UCHAR mask = 0x80 >> bit;
    while (wXStart <= wXEnd)
    {
        if (!mask)
        {
            mask = 0x80;
            ++Put;
        }
        *Put ^= mask;
        ++wXStart;
        mask >>= 1;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
{
    UCHAR uVal = 0x80;

    if (wXPos & 7)
    {
        uVal >>= wXPos & 7;
    }
    while (wYStart <= wYEnd)
    {
        UCHAR *Put = mpScanPointers[wYStart] + (wXPos >> 1);
        *Put ^= uVal;
        wYStart += 1;
    }
}

#else // SOLID_XOR

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
    wXStart += 7;
    wXStart &= 0xfffc;
    UCHAR *Put = mpScanPointers[wYPos] + (wXStart >> 3);
    SIGNED iLen = wXEnd - wXStart;

    if (!iLen)
    {
        return;
    }
    while(wXStart < wXEnd - 7)
    {
        *Put++ ^= 0x55;
        wXStart += 8;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
{
    UCHAR uVal = 0x80 >> (wXPos & 7);

    while (wYStart <= wYEnd)
    {
        UCHAR *Put = mpScanPointers[wYStart] + (wXPos >> 3);
        *Put ^= uVal;
        wYStart += 2;
    }
}

#endif // SOLID_XOR

/*--------------------------------------------------------------------------*/
void MonoScreen::Capture(PegCapture *Info, PegRect &CaptureRect)
{
    PegBitmap *pMap = Info->Bitmap();

    if (CaptureRect.wLeft < 0)
    {
        CaptureRect.wLeft = 0;
    }

    if (CaptureRect.wTop < 0)
    {
        CaptureRect.wTop = 0;
    }

    // If the capture is not evenly aligned, align it and capture one more
    // pixel to the left:

    while (CaptureRect.wLeft & 7)
    {
        CaptureRect.wLeft--;
    }

    while ((CaptureRect.wRight & 7) != 7)
    {
        CaptureRect.wRight++;
    }

    Info->SetPos(CaptureRect);
    LONG Size = (pMap->wWidth * pMap->wHeight) >> 3;
    Info->Realloc(Size);

    UCHAR *GetStart = mpScanPointers[CaptureRect.wTop] + (CaptureRect.wLeft >> 3);

    // make room for the memory bitmap:

    pMap->uFlags = 0;       // raw format
    pMap->uBitsPix = 1;     // 2 bits per pixel

    // fill in the image with our captured info:

    UCHAR *Put = pMap->pStart;

    for (WORD wLine = 0; wLine < pMap->wHeight; wLine++)
    {
        memcpy(Put, GetStart, pMap->wWidth >> 3);
        GetStart += miPitch;
        Put += pMap->wWidth >> 3;
    }
    Info->SetValid(TRUE);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::BitmapView(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    UCHAR uVal, uVal1, uPix;
    UCHAR *Get, *Put;

    if (IS_RLE(Getmap))
    {
        DrawRleBitmap(Where, View, Getmap);
    }
    else
    {
        if (Getmap->uBitsPix == 1)
        {
            if (Where.x & 7)
            {
                DrawUnalignedBitmap(Where, Getmap, View);
            }
            else
            {
                DrawFastBitmap(Where, Getmap, View);
            }
        }
        else
        {
            // here for a source bitmap of 8-bpp:

            SIGNED iWidth = View.Width();
            Get = Getmap->pStart;
            Get += (View.wTop - Where.y) * Getmap->wWidth;
            Get += View.wLeft - Where.x;

            for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
            {
                Put = mpScanPointers[wLine] + (View.wLeft >> 3);
                uVal = *Put;
                uPix = View.wLeft & 7;

                for (SIGNED wLoop1 = View.wLeft; wLoop1 <= View.wRight; wLoop1++)
                {
                    uVal1 = *Get++;

                    if (uVal1 != Getmap->dTransColor)
                    {
                        #ifdef PEG_RUNTIME_COLOR_CHECK
                        uVal1 = (UCHAR) MonoColors[uVal1 & 0x0f];
                        #endif

                        uVal &= ~(0x80 >> uPix);
                        uVal |= uVal1 << (7 - uPix);
                    }
                    if (uPix == 7)
                    {
                        *Put++ = uVal;
                        uVal = *Put;
                        uPix = 0;
                    }
                    else
                    {
                        uPix++;
                    }
                }

                if (uPix)
                {
                    *Put = uVal;
                }
                Get += Getmap->wWidth - iWidth;
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
// here for an aligned 1 bpp bitmap, no shifting required.
/*--------------------------------------------------------------------------*/
void MonoScreen::DrawFastBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    // always padded to nearest full byte per scan line:

    UCHAR uVal;
    UCHAR *GetStart = Getmap->pStart;
    UCHAR *bitdata;
    UCHAR mask = 0;
    WORD wBytesPerLine = (Getmap->wWidth + 7) >> 3;
    int nSrcBytesPerLine = (Getmap->wHeight + 7) >> 3;
    int nSrcHeight = Getmap->wWidth;
    int nSrcWidth = Getmap->wHeight;

    memset (m_rotscratch , 0, wBytesPerLine * Getmap->wHeight);
    // rotate the bitmap in scratch space, then use the normal routines to blit it to screen buffer.    
    for (int y = 0; y < nSrcHeight; ++y)       // iterate over input data
    {
        bitdata = GetStart;                         // iterate over each bit of input data
        mask = 0x80;
        for (int x = 0; x < nSrcWidth; ++x)    // iterate over input data
        {
            if (!mask)
            {
                mask = 0x80;                        // get another input byte
                ++bitdata;
            }
            if (*bitdata & mask)
                *(m_rotscratch+(wBytesPerLine*(Getmap->wHeight-1 - x) + (y>>3))) |= (0x80 >> (y&7));
            mask >>= 1;
        }
        GetStart += nSrcBytesPerLine;
    }

    GetStart = m_rotscratch;
    GetStart += (View.wTop - Where.y) * wBytesPerLine;
    GetStart += (View.wLeft - Where.x) >> 3;

    for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
    {
        UCHAR *Get = GetStart;
        UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 3);
        SIGNED iCount = View.Width();

        switch(View.wLeft & 7)
        {
        case 0:
            break;

        case 1:
            uVal = *Put;
            uVal &= 0x80;
            uVal |= *Get++ & 0x7f;
            *Put++ = uVal;
            iCount -= 7;
            break;

        case 2:
            uVal = *Put;
            uVal &= 0xc0;
            uVal |= *Get++ & 0x3f;
            *Put++ = uVal;
            iCount -= 6;
            break;

        case 3:
            uVal = *Put;
            uVal &= 0xe0;
            uVal |= *Get++ & 0x1f;
            *Put++ = uVal;
            iCount -= 5;
            break;

        case 4:
            uVal = *Put;
            uVal &= 0xf0;
            uVal |= *Get++ & 0x0f;
            *Put++ = uVal;
            iCount -= 4;
            break;

        case 5:
            uVal = *Put;
            uVal &= 0xf8;
            uVal |= *Get++ & 0x07;
            *Put++ = uVal;
            iCount -= 3;
            break;

        case 6:
            uVal = *Put;
            uVal &= 0xfc;
            uVal |= *Get++ & 0x03;
            *Put++ = uVal;
            iCount -= 2;
            break;

        case 7:
            uVal = *Put;
            uVal &= 0xfe;
            uVal |= *Get++ & 0x01;
            *Put++ = uVal;
            iCount--;
            break;
        }

        if (iCount > 0)
        {       
            // copy 8 pixels at a time:
            memcpy(Put, Get, iCount >> 3);
            Put += iCount >> 3;
            Get += iCount >> 3;

            switch (View.wRight & 7)
            {
            case 7:
                break;

            case 6:
                uVal = *Put;
                uVal &= 0x01;
                uVal |= *Get & 0xfe;
                *Put = uVal;
                break;

            case 5:
                uVal = *Put;
                uVal &= 0x03;
                uVal |= *Get & 0xfc;
                *Put = uVal;
                break;

            case 4:
                uVal = *Put;
                uVal &= 0x07;
                uVal |= *Get & 0xf8;
                *Put = uVal;
                break;

            case 3:
                uVal = *Put;
                uVal &= 0x0f;
                uVal |= *Get & 0xf0;
                *Put = uVal;
                break;

            case 2:
                uVal = *Put;
                uVal &= 0x1f;
                uVal |= *Get & 0xe0;
                *Put = uVal;
                break;

            case 1:
                uVal = *Put;
                uVal &= 0x3f;
                uVal |= *Get & 0xc0;
                *Put = uVal;
                break;

            case 0:
                uVal = *Put;
                uVal &= 0x7f;
                uVal |= *Get & 0x80;
                *Put = uVal;
                break;
            }
        }
        GetStart += wBytesPerLine;
    }
}

/*--------------------------------------------------------------------------*/
// here for a misaligned monochrome bitmap, bit shifting required.
/*--------------------------------------------------------------------------*/
void MonoScreen::DrawUnalignedBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    UCHAR uVal, uiPix, uoPix, uVal1;
    SIGNED iCount;
    UCHAR *Get;
    // always padded to nearest full byte per scan line:
    UCHAR *GetStart = Getmap->pStart;
    UCHAR *bitdata;
    UCHAR mask = 0;
    WORD wBytesPerLine = (Getmap->wWidth + 7) >> 3;
    int nSrcBytesPerLine = (Getmap->wHeight + 7) >> 3;
    int nSrcHeight = Getmap->wWidth;
    int nSrcWidth = Getmap->wHeight;
    memset (m_rotscratch , 0, wBytesPerLine * Getmap->wHeight);
    // rotate the bitmap in scratch space, then use the normal routines to blit it to screen buffer.    
    for (int y = 0; y < nSrcHeight; ++y)       // iterate over input data
    {
        bitdata = GetStart;                         // iterate over each bit of input data
        mask = 0x80;
        for (int x = 0; x < nSrcWidth; ++x)    // iterate over input data
        {
            if (!mask)
            {
                mask = 0x80;                        // get another input byte
                ++bitdata;
            }
            if (*bitdata & mask)
                *(m_rotscratch+(wBytesPerLine*(Getmap->wHeight-1 - x) + (y>>3))) |= (0x80 >> (y&7));
            mask >>= 1;
        }
        GetStart += nSrcBytesPerLine;
    }

    GetStart = m_rotscratch;
    GetStart += (View.wTop - Where.y) * wBytesPerLine;

    GetStart += (View.wLeft - Where.x) >> 3;

    for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
    {
        Get = GetStart;
        uVal1 = *Get++;
        iCount = View.Width();
        UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 3);

        uiPix = (View.wLeft - Where.x) & 7;
        uVal1 <<= uiPix;

        uoPix = View.wLeft & 7;
        uVal = *Put;
        uVal &= 0xff << (8 - uoPix);
        uVal >>= 7 - uoPix;

        while (iCount-- > 0)
        {
            uVal |= uVal1 >> 7;
            uoPix++;

            if (uoPix == 8)
            {
                uoPix =0;
                *Put++ = uVal;
                uVal = 0;
            }
            else
            {
                uVal <<= 1;
            }
            uiPix++;
            if (uiPix == 8)
            {
                uiPix = 0;
                uVal1 = *Get++;
            }
            else
            {
                uVal1 <<= 1;
            }
        }

        if (uoPix)
        {
            uVal1 = *Put;
            uVal1 &= 0xff >> uoPix;
            uVal1 |= uVal << (7 - uoPix);
            *Put = uVal1;
        }
        GetStart += wBytesPerLine;
    }
}




/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::DrawRleBitmap(const PegPoint Where, const PegRect View,
    const PegBitmap *Getmap)
{
    UCHAR *Get = Getmap->pStart;
    UCHAR uVal;
    SIGNED uCount;

    SIGNED wLine = Where.y;

    uCount = 0;

    while (wLine < View.wTop)
    {
        uCount = 0;

        while(uCount < Getmap->wWidth)
        {
            uVal = *Get++;
            if (uVal & 0x80)
            {
                uVal = (uVal & 0x7f) + 1;
                uCount += uVal;
                Get += uVal;
            }
            else
            {
                Get++;
                uCount += uVal + 1;
            }
        }
        wLine++;
    }

    while (wLine <= View.wBottom)
    {
        SIGNED wLoop1 = Where.x;

        while (wLoop1 < Where.x + Getmap->wWidth)
        {
            uVal = *Get++;

            if (uVal & 0x80)        // raw packet?
            {
                uCount = (uVal & 0x7f) + 1;
                
                while (uCount--)
                {
                    uVal = *Get++;
                    if (wLoop1 >= View.wLeft &&
                        wLoop1 <= View.wRight &&
                        uVal != Getmap->dTransColor)
                    {
                        PlotPointView(wLoop1, wLine, uVal);
                    }
                    wLoop1++;
                }
            }
            else
            {
                uCount = uVal + 1;
                uVal = *Get++;

                if (uVal == Getmap->dTransColor)
                {
                    wLoop1 += uCount;
                }
                else
                {
                    while(uCount--)
                    {
                        if (wLoop1 >= View.wLeft &&
                            wLoop1 <= View.wRight)
                        {
                            PlotPointView(wLoop1, wLine, uVal);
                        }
                        wLoop1++;
                    }
                }
            }
        }
        wLine++;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::RectMoveView(PegThing *Caller, const PegRect &View,
     const SIGNED xMove, const SIGNED yMove)
{
    PegCapture BlockCapture;
    PegRect CaptureRect = View;
    Capture(&BlockCapture, CaptureRect);
    BlockCapture.Shift(xMove, yMove);
    Bitmap(Caller, BlockCapture.Point(), BlockCapture.Bitmap(), TRUE);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::DrawTextView(PegPoint Where, const TCHAR *Text, PegColor &Color,
    PegFont *Font, SIGNED iCount, PegRect &Rect)
{
   #ifdef PEG_UNICODE
    TCHAR PEGFAR *pCurrentChar = (TCHAR PEGFAR *) Text;
    PegFont *pFontStart = Font;
   #else
    UCHAR PEGFAR *pCurrentChar = (UCHAR PEGFAR *) Text;
   #endif

    UCHAR PEGFAR *pGetData;
    UCHAR PEGFAR *pGetDataBase;
    WORD  wIndex;
    WORD  wBitOffset;
    SIGNED  wXpos = Where.x;
    WORD cVal = *pCurrentChar++;
    SIGNED iCharWidth;

   #ifdef DO_OUTLINE_TEXT
    if (IS_OUTLINE(Font))
    {
        DrawOutlineText(Where, Text, Color, Font, iCount, Rect);
        return;
    }
   #endif

    while(cVal && wXpos <= Rect.wRight)
    {
        if (iCount == 0)
        {
            return;
        }
        iCount--;

       #ifdef PEG_UNICODE
        Font = pFontStart;

        while(Font)
        {
            if (cVal >= Font->wFirstChar &&
                cVal <= Font->wLastChar)
            {
                break;
            }
            Font = Font->pNext;
        }
        if (!Font)                 // this font doesn't contain this glyph?
        {
            cVal = *pCurrentChar++; // just skip to next char
            continue;               
        }

        wIndex = cVal - (WORD) Font->wFirstChar;

        if (IS_VARWIDTH(Font))
        {
            wBitOffset = Font->pOffsets[wIndex];
            iCharWidth = Font->pOffsets[wIndex+1] - wBitOffset;
        }
        else
        {
            iCharWidth = (SIGNED) Font->pOffsets;
            wBitOffset = iCharWidth * wIndex;
        }

       #else

        wIndex = cVal - (WORD) Font->wFirstChar;
        wBitOffset = Font->pOffsets[wIndex];
        iCharWidth = Font->pOffsets[wIndex+1] - wBitOffset;

       #endif

        if (wXpos + iCharWidth > Rect.wRight)
        {
            iCharWidth = Rect.wRight - wXpos + 1;
        }

        WORD ByteOffset = wBitOffset / 8;
        pGetDataBase = Font->pData + ByteOffset;
        pGetDataBase += (Rect.wTop - Where.y) * Font->wBytesPerLine;

        for (SIGNED ScanRow = Rect.wTop; ScanRow <= Rect.wBottom; ScanRow++)
        {
            pGetData = pGetDataBase;
            UCHAR InMask = 0x80 >> (wBitOffset & 7);
            WORD wBitsOutput = 0;
            UCHAR cData;

           #ifdef PEG_UNICODE
            if (ScanRow - Where.y < Font->uHeight)
            {
                cData = *pGetData++;
            }
            else
            {
                cData = 0;
            }
           #else
            cData = *pGetData++;
           #endif

            while(wBitsOutput < iCharWidth)
            {
                if (!InMask)
                {
                    InMask = 0x80;
                    // read a byte:

                   #ifdef PEG_UNICODE
                    if (ScanRow - Where.y < Font->uHeight)
                    {
                        cData = *pGetData++;
                    }
                    else
                    {
                        cData = 0;
                    }
                   #else
                    cData = *pGetData++;
                   #endif
                }

                if (wXpos >= Rect.wLeft)
                {
                    if (cData & InMask)        // is this bit a 1?
                    {
                        PlotPointView(wXpos, ScanRow, Color.uForeground);
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
            wXpos -= iCharWidth;
        }
        wXpos += iCharWidth;
        cVal = *pCurrentChar++;
    }
}


/*--------------------------------------------------------------------------*/
void MonoScreen::HidePointer(void)
{
#ifndef PEGWIN32

   #ifdef PEG_MOUSE_SUPPORT
    PegThing *pt = NULL;
    PegPresentationManager *pp = pt->Presentation();
    Restore(pp, &mCapture, TRUE);
    mCapture.SetValid(FALSE);
   #endif
#endif
}

/*--------------------------------------------------------------------------*/
void MonoScreen::SetPointer(PegPoint Where)
{
#ifndef PEGWIN32

   #ifdef PEG_MOUSE_SUPPORT
    LOCK_PEG
    HidePointer();
    mLastPointerPos = Where;

    PegThing *pt = NULL;
    PegPresentationManager *pp = pt->Presentation();
    Where.x -= miCurXOffset;
    Where.y -= miCurYOffset;

    PegRect MouseRect;
    MouseRect.wLeft = Where.x;
    MouseRect.wTop =  Where.y;
    MouseRect.wBottom = MouseRect.wTop + mpCurPointer->wHeight - 1;
    MouseRect.wRight = MouseRect.wLeft + mpCurPointer->wWidth - 1;
    Capture(&mCapture, MouseRect);
    Bitmap(pp, Where, mpCurPointer, TRUE);
    UNLOCK_PEG
   #endif
#endif
}

/*--------------------------------------------------------------------------*/
void MonoScreen::SetPointerType(UCHAR bType)
{
   #ifdef PEGWIN32
    if (bType < NUM_POINTER_TYPES)
    {
        mpCurPointer = mpPointers[bType].Bitmap;
        miCurXOffset = mpPointers[bType].xOffset;
        miCurYOffset = mpPointers[bType].yOffset;
    }

    switch(bType)
    {
    case PPT_NORMAL:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        break;

    case PPT_VSIZE:
        SetCursor(LoadCursor(NULL, IDC_SIZENS));
        break;

    case PPT_HSIZE:
        SetCursor(LoadCursor(NULL, IDC_SIZEWE));
        break;

    case PPT_NWSE_SIZE:
        SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
        break;

    case PPT_NESW_SIZE:
        SetCursor(LoadCursor(NULL, IDC_SIZENESW));
        break;

    case PPT_IBEAM:
        SetCursor(LoadCursor(NULL, IDC_IBEAM));
        break;

    case PPT_HAND:
        SetCursor(LoadCursor(NULL, IDC_CROSS));
        break;
    }
   #endif
   #ifdef PEG_MOUSE_SUPPORT
    if (bType < NUM_POINTER_TYPES)
    {
        LOCK_PEG
        HidePointer();
        mpCurPointer = mpPointers[bType].Bitmap;
        miCurXOffset = mpPointers[bType].xOffset;
        miCurYOffset = mpPointers[bType].yOffset;
        SetPointer(mLastPointerPos);
        UNLOCK_PEG
    }
   #endif
}


/*--------------------------------------------------------------------------*/
void MonoScreen::ResetPalette(void)
{
    SetPalette(0, 2, DefPalette);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR *MonoScreen::GetPalette(DWORD *pPutSize)
{
    *pPutSize = 2;
    return muPalette;
}


#ifdef PEGWIN32

typedef struct 
{
    BITMAPINFOHEADER bmhead;
    RGBQUAD  ColorTable[256];
} BMHEAD;

BMHEAD BMhead;

typedef struct
{
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[256];
} PEG_WIN_PALETTE;

PEG_WIN_PALETTE WinPal;

#endif


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet)
{
   #ifdef PEGWIN32

    BMhead.bmhead.biSize = sizeof(BITMAPINFOHEADER);
    BMhead.bmhead.biWidth = mwHRes;
    BMhead.bmhead.biHeight = -mwVRes;
    BMhead.bmhead.biPlanes = 1;
    BMhead.bmhead.biBitCount = 1;
    BMhead.bmhead.biCompression = BI_RGB;
    BMhead.bmhead.biSizeImage = mwHRes * mwVRes;
    BMhead.bmhead.biClrUsed = iNum;
    BMhead.bmhead.biClrImportant = iNum;
    WinPal.palNumEntries = iNum;
    WinPal.palVersion = 0x0300;

    UCHAR *pPut = muPalette;

    for (WORD loop = 0; loop < iNum; loop++)
    {
        *pPut++ = *pGet;
        BMhead.ColorTable[loop].rgbRed = *pGet++;
        *pPut++ = *pGet;
        BMhead.ColorTable[loop].rgbGreen = *pGet++;
        *pPut++ = *pGet;
        BMhead.ColorTable[loop].rgbBlue = *pGet++;

        WinPal.palPalEntry[loop].peRed = BMhead.ColorTable[loop].rgbRed;
        WinPal.palPalEntry[loop].peGreen = BMhead.ColorTable[loop].rgbGreen;
        WinPal.palPalEntry[loop].peBlue = BMhead.ColorTable[loop].rgbBlue;
        WinPal.palPalEntry[loop].peFlags = 0;
    }
    if (mhPalette)
    {
        DeleteObject(mhPalette);
    }
    mhPalette = CreatePalette((LOGPALETTE *) &WinPal);

   #else
    // (epg,9/11/2001): shouldn't be needed in a mono screen driver, never was implemented on previous projects.
      //** Error- the SetPalette function must be filled in **
   #endif
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void MonoScreen::ConfigureController(void)
{
    #ifdef PEGWIN32
    #else
	LCDEnable();
	LCDClear();
#if defined(LCD_BACKLIGHT)
	LCDSetBacklight(LCD_BACKLIGHT_ON);
#endif
	memset(s_ucBuf, 0xFF, DISPLAY_BUF_SIZE);
    #endif

}


/*--------------------------------------------------------------------------*/
// function to blast our memory out to the windows screen. This is only used
// for testing purposes, and can be deleted.

void MonoScreen::MemoryToScreen(void)
{
#ifdef PEGWIN32
    if (!miInvalidCount)
    {
        return;
    }
    PegRect Copy;
    Copy.wTop = 0;
    Copy.wLeft = 0;
    Copy.wRight = mwHRes - 1;
    Copy.wBottom = mwVRes - 1;
    Copy &= mInvalid;

    SetMapMode(mHdc, MM_TEXT);

    HPALETTE hOldPal = SelectPalette(mHdc, mhPalette, FALSE);
    RealizePalette(mHdc);

    SIGNED iTop = Copy.wTop + mwWinRectYOffset;
    SIGNED iLeft = Copy.wLeft + mwWinRectXOffset;

    SIGNED iWidth = Copy.Width();
    SIGNED iHeight = Copy.Height();

    if (Copy.wTop + iHeight == mwVRes)
    {
        iHeight--;
    }

    StretchDIBits(mHdc, iLeft, iTop, iWidth, iHeight,
        Copy.wLeft, mwVRes - (Copy.wTop + iHeight), iWidth, iHeight,
        mpScanPointers[0], (const struct tagBITMAPINFO *) &BMhead,
        DIB_RGB_COLORS, SRCCOPY);

    SelectObject(mHdc, hOldPal);
#else
    LCDWriteInverted1to4(s_ucBuf, DISPLAY_BUF_SIZE);
#endif      // the WIN32 if
}



