/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// ProScrn1.cpp - Profile mode monochrome screen driver template. This driver
//      assumes linear frame buffer access. This driver can be configured
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
// The driver can be configured to run with a screen that has been rotated
// 90 degrees counter-clockwise or 90 degrees clockwise.
//
// This driver does NOT support RUNTIME_COLOR_CHECK operation.
//
// All available configuration flags are found in the proscrn1.hpp header file.
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
// The vertical resolution of the screen (after rotation) MUST be evenly 
// divisible by eight.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include "stdlib.h"
#include "string.h"
#include <gui/peg/peg.hpp>
#include <devs/lcd/lcd.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_SCRN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_SCRN );  // debugging prefix : (20) scrn

#ifdef PEGWIN32
UCHAR DefPalette[2*3] = {
0, 255, 0,        // black
0, 0, 0   // white
};
#else
UCHAR DefPalette[2*3] = {
0, 0, 0,        // black
255, 255, 255   // white
};
#endif

#define DISPLAY_BUF_SIZE	LCD_HEIGHT * LCD_WIDTH / 8
static UCHAR s_ucBuf[ DISPLAY_BUF_SIZE ];

/*--------------------------------------------------------------------------*/


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
    PegScreen *pScreen = new ProMonoScreen(Rect);
    return pScreen;
}
#endif

/*--------------------------------------------------------------------------*/
// Constructor- initialize video memory addresses
/*--------------------------------------------------------------------------*/
#ifdef PEGWIN32
ProMonoScreen::ProMonoScreen(HWND hWnd, PegRect &Rect) : PegScreen(Rect)
#else
ProMonoScreen::ProMonoScreen(PegRect &Rect) : PegScreen(Rect)
#endif
{
    mdNumColors = 2;  

   #ifdef PEGWIN32
    mhPalette = NULL;
   #endif

    mwHRes = Rect.Width();
    mwVRes = Rect.Height();
    miPitch = (mwVRes + 7) >> 3;

   #ifdef PEGWIN32
    // Windows bitmaps must be modulo-4 byte in width:
    miPitch += 3;
    miPitch &= 0xfffc;
   #endif

    mpScanPointers = new UCHAR PEGFAR *[mwHRes];

    UCHAR PEGFAR *CurrentPtr = GetVideoAddress();

   #ifdef ROTATE_CCW
    CurrentPtr += miPitch - 1;

    for (SIGNED iLoop = 0; iLoop < mwHRes; iLoop++)
    {
        mpScanPointers[iLoop] = CurrentPtr;
        CurrentPtr += miPitch;
    }

   #else
    CurrentPtr += miPitch * (mwHRes - 1);

    for (SIGNED iLoop = 0; iLoop < mwHRes; iLoop++)
    {
        mpScanPointers[iLoop] = CurrentPtr;
        CurrentPtr -= miPitch;
    }

   #endif

    mLastPointerPos.x = Rect.Width() / 2;
    mLastPointerPos.y = Rect.Height() / 2;
    mbPointerHidden = FALSE;
    mwDrawNesting = 0;

    ConfigureController();        // set up controller registers

    SetPalette(0, 2, DefPalette);

#ifdef PEGWIN32

   // Some setup stuff for the BitBlitting function:
   mHWnd = hWnd;
   RECT lSize;
   ::GetClientRect(mHWnd, &lSize);

   mwWinRectXOffset = (lSize.right - mwHRes) / 2;
   mwWinRectYOffset = (lSize.bottom -mwVRes) / 2;

   // make a second buffer to hold the bitmap data rotated back to
   // portrait mode:
   WORD wBytesPerLine = mwHRes >> 3;
   wBytesPerLine += 3;
   wBytesPerLine &= 0xfffc;
   mpDisplayBuff = new UCHAR[wBytesPerLine * mwVRes];
#endif

}


/*--------------------------------------------------------------------------*/
// *** This function must be filled in by the developer ***
/*--------------------------------------------------------------------------*/
UCHAR PEGFAR *ProMonoScreen::GetVideoAddress(void)
{
#ifdef PEGWIN32

    DWORD dSize = mwHRes / 8 * mwVRes;
    UCHAR *pMem = new UCHAR[dSize];
    memset(pMem, 0xff, dSize);      // set it all to WHITE
    return pMem; 

#else
    return s_ucBuf;
#endif
}


/*--------------------------------------------------------------------------*/
// Destructor
/*--------------------------------------------------------------------------*/
ProMonoScreen::~ProMonoScreen()
{
  #ifdef PEGWIN32
    delete mpDisplayBuff;

    UCHAR *pMem = mpScanPointers[0];
    
   #ifdef ROTATE_CCW
    pMem -= (mwVRes >> 3);
    pMem++;
   #else
    pMem -= (mwVRes >> 3) * (mwHRes - 1);
   #endif
    delete pMem;

  #endif

    delete mpScanPointers;
}


/*--------------------------------------------------------------------------*/
void ProMonoScreen::BeginDraw(PegThing *)
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
void ProMonoScreen::BeginDraw(PegThing *Caller, PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        return;
    }
    LOCK_PEG
    mpSaveScanPointers = mpScanPointers;

    if (pMap->wHeight && pMap->wWidth && pMap->pStart)
    {
        WORD wBytesPerLine = (pMap->wHeight + 7) >> 3;

        mpScanPointers = new UCHAR PEGFAR *[pMap->wWidth];
        UCHAR PEGFAR *CurrentPtr = pMap->pStart;

      #ifdef ROTATE_CCW
        CurrentPtr += wBytesPerLine - 1;
      #endif

        for (SIGNED iLoop = 0; iLoop < pMap->wWidth; iLoop++)
        {
            mpScanPointers[iLoop] = CurrentPtr;
            CurrentPtr += wBytesPerLine;
        }

        mVirtualRect.Set(0, 0, pMap->wWidth - 1, pMap->wHeight - 1);
        miPitch = wBytesPerLine;
        mbVirtualDraw = TRUE;
    }
}

/*--------------------------------------------------------------------------*/
void ProMonoScreen::EndDraw()
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
void ProMonoScreen::EndDraw(PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        mbVirtualDraw = FALSE;
        delete [] mpScanPointers;
        mpScanPointers = mpSaveScanPointers;
        miPitch = (mwVRes + 7) >> 3;
        UNLOCK_PEG
    }
}

/*--------------------------------------------------------------------------*/
// CreateBitmap: The default version creates an 8-bpp bitmap, so we override
// to create a 1-bpp bitmap.
/*--------------------------------------------------------------------------*/
PegBitmap *ProMonoScreen::CreateBitmap(SIGNED wWidth, SIGNED wHeight)
{
   PegBitmap *pMap = NULL;

    if (wWidth && wHeight)
    {
        pMap = new PegBitmap;
        pMap->wWidth = wWidth;
        pMap->wHeight = wHeight;
        wHeight += 7;
        wHeight >>= 3;
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
void ProMonoScreen::LineView(SIGNED wXStart, SIGNED wYStart, SIGNED wXEnd, 
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
void ProMonoScreen::HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR uVal, uShift, uMask;
    COLORVAL uFill;
    UCHAR *Put;

    SIGNED iOffset = miPitch;

   #ifdef ROTATE_CW
    if (mbVirtualDraw)
    {
        iOffset = -miPitch;
    }
   #endif

    while(wWidth-- > 0)
    {
        #ifdef ROTATE_CCW
        uShift = (wYPos & 7);     // 0,0 is lsb 
        #else
        uShift = 7 - (wYPos & 7); // 0,0 is msb
        #endif
        uFill = Color << uShift;
        uMask = ~(0x01 << uShift);

        #ifdef ROTATE_CCW
        Put = mpScanPointers[wXStart] - (wYPos >> 3);
        #else
        Put = mpScanPointers[wXStart] + (wYPos >> 3);
        #endif
        SIGNED iLen = wXEnd - wXStart;

        while(iLen-- >= 0)
        {
            uVal = *Put;
            uVal &= uMask;
            uVal |= uFill;
            *Put = uVal;

            #ifdef ROTATE_CCW
            Put += iOffset;
            #else
            Put -= iOffset;
            #endif
        }
        wYPos++;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR *Put;
    UCHAR uVal;
    UCHAR uFill = 0;

    if (Color)
    {
        uFill = 0xff;
    }
    if (wYEnd < wYStart)
    {
        return;
    }
                        
    while(wWidth-- > 0)
    {
        SIGNED iLen = wYEnd - wYStart + 1;

       #ifdef ROTATE_CCW

        Put = mpScanPointers[wXPos] - (wYStart >> 3);

        switch(wYStart & 7)
        {
        case 1:
            if (iLen >= 7)
            {
                uVal = *Put;
                uVal &= 0x01;
                uVal |= uFill & 0xfe;
                *Put-- = uVal;
                iLen -= 7;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
                continue;
            }
            break;

        case 2:
            if (iLen >= 6)
            {
                uVal = *Put;
                uVal &= 0x03;
                uVal |= uFill & 0xfc;
                *Put-- = uVal;
                iLen -= 6;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
                continue;
            }
            break;

        case 3:
            if (iLen >= 5)
            {
                uVal = *Put;
                uVal &= 0x07;
                uVal |= uFill & 0xf8;
                *Put-- = uVal;
                iLen -= 5;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
                continue;
            }
            break;

        case 4:
            if (iLen >= 4)
            {
                uVal = *Put;
                uVal &= 0x0f;
                uVal |= uFill & 0xf0;
                *Put-- = uVal;
                iLen -= 4;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
                continue;
            }
            break;

        case 5:
            if (iLen >= 3)
            {
                uVal = *Put;
                uVal &= 0x1f;
                uVal |= uFill & 0xe0;
                *Put-- = uVal;
                iLen -= 3;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
                continue;
            }
            break;

        case 6:
            if (iLen >= 2)
            {
                uVal = *Put;
                uVal &= 0x3f;
                uVal |= uFill & 0xc0;
                *Put-- = uVal;
                iLen -= 2;
            }
            else
            {
                PlotPointView(wXPos, wYStart, Color);
                wXPos++;
                continue;
            }
            break;

        case 7:
            PlotPointView(wXPos, wYStart, Color);
            Put--;
            iLen--;
            break;

        default:
            break;
        }

        // most compilers seem to do a good job of optimizing 
        // memset to do 32-bit data writes. If your compiler doesn't
        // make the most of your CPU, you might want to re-write this
        // in assembly.

        if (iLen >= 8)
        {
            Put -= (iLen >> 3) - 1;
            memset(Put, uFill, iLen >> 3);
            Put--;
        }
        if (iLen)
        {
            switch(wYEnd & 7)
            {
            case 6:
                uVal = *Put;
                uVal &= 0x80;
                uVal |= uFill & 0x7f;
                *Put = uVal;
                break;
                
            case 5:
                uVal = *Put;
                uVal &= 0xc0;
                uVal |= uFill & 0x3f;
                *Put = uVal;
                break;
                
            case 4:
                uVal = *Put;
                uVal &= 0xe0;
                uVal |= uFill & 0x1f;
                *Put = uVal;
                break;

            case 3:
                uVal = *Put;
                uVal &= 0xf0;
                uVal |= uFill & 0x0f;
                *Put = uVal;
                break;
                
            case 2:
                uVal = *Put;
                uVal &= 0xf8;
                uVal |= uFill & 0x07;
                *Put = uVal;
                break;
                
            case 1:
                uVal = *Put;
                uVal &= 0xfc;
                uVal |= uFill & 0x03;
                *Put = uVal;
                break;
                
            case 0:
                uVal = *Put;
                uVal &= 0xfe;
                uVal |= uFill & 0x01;
                *Put = uVal;
                break;
            }
        }

       #else        // ROTATE CW

        Put = mpScanPointers[wXPos] + (wYStart >> 3);

        switch(wYStart & 7)
        {
        case 1:
            if (iLen >= 7)
            {
                uVal = *Put;
                uVal &= 0x80;
                uVal |= uFill & 0x7f;
                *Put++ = uVal;
                iLen -= 7;
            }
            else
            {
                while(iLen-- > 0)
                {
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
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
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
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
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
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
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
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
                    PlotPointView(wXPos, wYStart + iLen, Color);
                }
                wXPos++;
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
                PlotPointView(wXPos, wYStart, Color);
                wXPos++;
                continue;
            }
            break;

        case 7:
            PlotPointView(wXPos, wYStart, Color);
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

        if (iLen >= 8)
        {
            memset(Put, uFill, iLen >> 3);
            Put += iLen >> 3;
        }
        if (iLen)
        {
            switch(wYEnd & 7)
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
       #endif

        wXPos++;
    }
}

#define SOLID_XOR
#ifndef SOLID_XOR

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
   #ifdef ROTATE_CCW
    UCHAR uVal = 0x01 << (wYPos & 7);
   #else
    UCHAR uVal = 0x80 >> (wYPos & 7);
   #endif

    while (wXStart <= wXEnd)
    {
       #ifdef ROTATE_CCW
        UCHAR *Put = mpScanPointers[wXStart] - (wYPos >> 3);
       #else
        UCHAR *Put = mpScanPointers[wXStart] + (wYPos >> 3);
       #endif
        *Put ^= uVal;
        wXStart += 2;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
{
    wYStart += 7;
    wYStart &= 0xfffc;
    #ifdef ROTATE_CCW
    UCHAR *Put = mpScanPointers[wXPos] - (wYStart >> 3);
    #else
    UCHAR *Put = mpScanPointers[wXPos] + (wYStart >> 3);
    #endif
    SIGNED iLen = wYEnd - wYStart;

    if (iLen <= 0)
    {
        return;
    }
    while(wYStart < wYEnd - 7)
    {
        #ifdef ROTATE_CCW
        *Put-- ^= 0x55;
        #else
        *Put++ ^= 0x55;
        #endif
        wYStart += 8;
    }
}

#else

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
#ifndef PEGWIN32
#ifdef TRACE_DRAW
DEBUG("HorizontalLineXOR\n");
#endif	// TRACE_DRAW
#endif	// PEGWIN32
   #ifdef ROTATE_CCW
    UCHAR uVal = 0x01 << (wYPos & 7);
   #else
    UCHAR uVal = 0x80 >> (wYPos & 7);
   #endif

    while (wXStart <= wXEnd)
    {
       #ifdef ROTATE_CCW
        UCHAR *Put = mpScanPointers[wXStart] - (wYPos >> 3);
       #else
        UCHAR *Put = mpScanPointers[wXStart] + (wYPos >> 3);
       #endif
        *Put ^= uVal;
        wXStart += 2;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
{
#ifndef PEGWIN32
#ifdef TRACE_DRAW
DEBUG("VerticalLineXOR\n");
#endif	// TRACE_DRAW
#endif	// PEGWIN32
#if 0
    wYStart += 7;
    wYStart &= 0xfffc;
    #ifdef ROTATE_CCW
    UCHAR *Put = mpScanPointers[wXPos] - (wYStart >> 3);
    #else
    UCHAR *Put = mpScanPointers[wXPos] + (wYStart >> 3);
    #endif
    SIGNED iLen = wYEnd - wYStart;

    if (iLen <= 0)
    {
        return;
    }
    while(wYStart < wYEnd - 7)
    {
        #ifdef ROTATE_CCW
        *Put-- ^= 0xFF;
        #else
        *Put++ ^= 0xFF;
        #endif
        wYStart += 8;
    }
#else
#ifdef ROTATE_CCW
	SIGNED wYByteAlignedStart = wYStart + 7;
    wYByteAlignedStart &= 0xfff8;
    UCHAR *Put = mpScanPointers[wXPos] - ((wYStart - 1) >> 3);
#else
//	++wYEnd;
	SIGNED wYByteAlignedStart = wYStart + 7;
    wYByteAlignedStart &= 0xfff8;
//    UCHAR *Put = mpScanPointers[wXPos] + (wYStart >> 3);
	// ecm: This is a hack, and it seems to work, though I couldn't tell you why.
	UCHAR *Put = mpScanPointers[wXPos] + ((wYStart - 1) >> 3);
#endif
	// Fill in the first few pixels.
	switch (wYByteAlignedStart - wYStart)
	{
#ifdef ROTATE_CCW
		case 1:
			*Put ^= 0x80;
			break;
		case 2:
			*Put ^= 0xC0;
			break;
		case 3:
			*Put ^= 0xE0;
			break;
		case 4:
			*Put ^= 0xF0;
			break;
		case 5:
			*Put ^= 0xF8;
			break;
		case 6:
			*Put ^= 0xFC;
			break;
		case 7:
			*Put ^= 0xFE;
			break;
#else
		case 1:
			*Put ^= 0x01;
			break;
		case 2:
			*Put ^= 0x03;
			break;
		case 3:
			*Put ^= 0x07;
			break;
		case 4:
			*Put ^= 0x0F;
			break;
		case 5:
			*Put ^= 0x1F;
			break;
		case 6:
			*Put ^= 0x3F;
			break;
		case 7:
			*Put ^= 0x7F;
			break;
#endif
	};
	// If the XOR area falls entirely within byte boundaries, the XOR the
	// area we mistakenly XOR'ed above.
	switch (wYByteAlignedStart - wYEnd - 1)
	{
#ifdef ROTATE_CCW
		case 1:
			*Put ^= 0x80;
			break;
		case 2:
			*Put ^= 0xC0;
			break;
		case 3:
			*Put ^= 0xE0;
			break;
		case 4:
			*Put ^= 0xF0;
			break;
		case 5:
			*Put ^= 0xF8;
			break;
		case 6:
			*Put ^= 0xFC;
			break;
		case 7:
			*Put ^= 0xFE;
			break;
#else
		case 1:
			*Put ^= 0x01;
			break;
		case 2:
			*Put ^= 0x03;
			break;
		case 3:
			*Put ^= 0x07;
			break;
		case 4:
			*Put ^= 0x0F;
			break;
		case 5:
			*Put ^= 0x1F;
			break;
		case 6:
			*Put ^= 0x3F;
			break;
		case 7:
			*Put ^= 0x7F;
			break;
#endif
	}

#ifdef ROTATE_CCW
	--Put;
#else
	++Put;
#endif

    while(wYByteAlignedStart <= wYEnd - 7)
    {
#ifdef ROTATE_CCW
		*Put-- ^= 0xFF;
#else
		*Put++ ^= 0xFF;
#endif
        wYByteAlignedStart += 8;
    }
	// Fill in the last few pixels.
	switch (wYEnd - wYByteAlignedStart)
	{
#ifdef ROTATE_CCW
		case 0:
			*Put ^= 0x01;
			break;
		case 1:
			*Put ^= 0x03;
			break;
		case 2:
			*Put ^= 0x07;
			break;
		case 3:
			*Put ^= 0x0F;
			break;
		case 4:
			*Put ^= 0x1F;
			break;
		case 5:
			*Put ^= 0x3F;
			break;
		case 6:
			*Put ^= 0x7F;
			break;
		case 7:
			*Put ^= 0xFF;
			break;
#else
		case 0:
			*Put ^= 0x80;
			break;
		case 1:
			*Put ^= 0xC0;
			break;
		case 2:
			*Put ^= 0xE0;
			break;
		case 3:
			*Put ^= 0xF0;
			break;
		case 4:
			*Put ^= 0xF8;
			break;
		case 5:
			*Put ^= 0xFC;
			break;
		case 6:
			*Put ^= 0xFE;
			break;
#endif
	};


#endif
}

#endif
/*--------------------------------------------------------------------------*/
void ProMonoScreen::Capture(PegCapture *Info, PegRect &CaptureRect)
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
    if (CaptureRect.wBottom >= mwVRes)
    {
        CaptureRect.wBottom = mwVRes - 1;
    }
    if (CaptureRect.wRight >= mwHRes)
    {
        CaptureRect.wRight = mwHRes - 1;
    }
    if (CaptureRect.wLeft > CaptureRect.wRight ||
        CaptureRect.wTop > CaptureRect.wBottom)
    {
        return;
    }
    // If the capture is not evenly aligned, align it and capture one more
    // pixel to the left:
    while (CaptureRect.wTop & 7)
    {
        CaptureRect.wTop--;
    }
    while ((CaptureRect.wBottom & 7) != 7)
    {
        CaptureRect.wBottom++;
    }

    Info->SetPos(CaptureRect);
    LONG Size = (pMap->wWidth * pMap->wHeight) >> 3;
    Info->Realloc(Size);

#ifdef ROTATE_CCW
    UCHAR* GetStart = GetVideoAddress();

    // make room for the memory bitmap:
    pMap->uFlags = 0;       // raw format
    pMap->uBitsPix = 1;     // 1 bits per pixel
    UCHAR *Put = pMap->pStart;
    int nDestByteWidth = (CaptureRect.wRight - CaptureRect.wLeft+7) >> 3;

    // loop through target byte locations, constructing each byte from the screen as we go.
    for (int y = CaptureRect.wTop; y <= CaptureRect.wBottom; ++y)
    {
        // note that I'm considering x as a byte-level coordinate (y is pixel level), to match the destination format.
        for (int x = (CaptureRect.wLeft+7)>>3; x <= ((CaptureRect.wRight+7)>>3); ++x)
        {
            // for this byte, reign in all the source bytes, extract the relevant bit from each, and cram them together.
            UCHAR cSourceMask = 1 << (y&7);
            // x is a byte offset, so no need to >>3 mwVRes...
            UCHAR* pGet = GetStart + (x * mwVRes) + (7-(y>>3));
            UCHAR cDestByte = 0;
            for (int nDestBit = 8; nDestBit > 0; --nDestBit)
            {
                if (*pGet & cSourceMask)
                    cDestByte |= 1 << (nDestBit-1);
                pGet+=mwVRes>>3;
            }
            *(Put + y * nDestByteWidth + x) = cDestByte;
        }
    }
#else
    DBASSERT( DBG_SCRN , ( 1==0 ) , "SCRN:non-rotated screen capture not implemented!\n"); 
#endif
    Info->SetValid(TRUE);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::BitmapView(const PegPoint Where, const PegBitmap *Getmap,
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
            #ifdef ROTATE_CCW
            if (((Where.y + Getmap->wHeight - 1)  & 7) != 7)
            #else
            if (Where.y & 7)
            #endif
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

            SIGNED iHeight = View.Height();
            Get = Getmap->pStart;
            Get += (View.wLeft - Where.x) * Getmap->wHeight;

            #ifdef ROTATE_CCW
            Get += (Where.y + Getmap->wHeight - 1 - View.wBottom);
            #else
            Get += View.wTop - Where.y;
            #endif

            for (SIGNED wCol = View.wLeft; wCol <= View.wRight; wCol++)
            {
               #ifdef ROTATE_CCW
                Put = mpScanPointers[wCol] - (View.wBottom >> 3);
                uPix = View.wBottom & 7;
                uVal = *Put;

                for (SIGNED wRow = View.wBottom; wRow >= View.wTop; wRow--)
                {
                    uVal1 = *Get++;

                    if (uVal1 != 0xff)
                    {
                        uVal &= ~(1 << uPix);
                        uVal |= uVal1 << uPix;
                    }
                    if (!uPix)
                    {
                        *Put++ = uVal;
                        uVal = *Put;
                        uPix = 7;
                    }
                    else
                    {
                        uPix--;
                    }
                }
                if (uPix != 7)
                {
                    *Put = uVal;
                }

               #else        // rotate CW

                Put = mpScanPointers[wCol] + (View.wTop >> 3);
                uPix = 7 - (View.wTop & 7);
                uVal = *Put;

                for (SIGNED wRow = View.wTop; wRow <= View.wBottom; wRow++)
                {
                    uVal1 = *Get++;

                    if (uVal1 != 0xff)
                    {
                        uVal &= ~(0x01 << uPix);
                        uVal |= uVal1 << uPix;
                    }
                    if (!uPix)
                    {
                        *Put++ = uVal;
                        uVal = *Put;
                        uPix = 7;
                    }
                    else
                    {
                        uPix--;
                    }
                }
                if (uPix != 7)
                {
                    *Put = uVal;
                }

               #endif

                Get += Getmap->wHeight - iHeight;
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
// here for an aligned 1 bpp bitmap, no shifting required.
/*--------------------------------------------------------------------------*/
void ProMonoScreen::DrawFastBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    // always padded to nearest full byte per scan line:

    UCHAR uVal;
    WORD wBytesPerLine = (Getmap->wHeight + 7) >> 3;
    UCHAR *GetStart = Getmap->pStart;
    GetStart += (View.wLeft - Where.x) * wBytesPerLine;

   #ifdef ROTATE_CCW
    GetStart += (Where.y + Getmap->wHeight - 1 - View.wBottom) >> 3;
   #else
    GetStart += (View.wTop - Where.y) >> 3;
   #endif

    for (SIGNED wCol = View.wLeft; wCol <= View.wRight; wCol++)
    {
        UCHAR *Get = GetStart;
        SIGNED iCount = View.Height();

       #ifdef ROTATE_CCW
        UCHAR *Put = mpScanPointers[wCol] - (View.wBottom >> 3);

        switch(View.wBottom & 7)
        {
        case 0:
            uVal = *Put;
            uVal &= 0xfe;
            uVal |= *Get++ & 0x01;
            *Put++ = uVal;
            iCount--;
            break;

        case 1:
            uVal = *Put;
            uVal &= 0xfc;
            uVal |= *Get++ & 0x03;
            *Put++ = uVal;
            iCount -= 2;
            break;

        case 2:
            uVal = *Put;
            uVal &= 0xf8;
            uVal |= *Get++ & 0x07;
            *Put++ = uVal;
            iCount -= 3;
            break;

        case 3:
            uVal = *Put;
            uVal &= 0xf0;
            uVal |= *Get++ & 0x0f;
            *Put++ = uVal;
            iCount -= 4;
            break;

        case 4:
            uVal = *Put;
            uVal &= 0xe0;
            uVal |= *Get++ & 0x1f;
            *Put++ = uVal;
            iCount -= 5;
            break;

        case 5:
            uVal = *Put;
            uVal &= 0xc0;
            uVal |= *Get++ & 0x3f;
            *Put++ = uVal;
            iCount -= 6;
            break;

        case 6:
            uVal = *Put;
            uVal &= 0x80;
            uVal |= *Get++ & 0x7f;
            *Put++ = uVal;
            iCount -= 7;
            break;

        case 7:
            break;
        }

        if (iCount >= 8)
        {
            SIGNED iTemp = iCount >> 3;       
            // copy 8 pixels at a time:
            memcpy(Put, Get, iTemp);
            Put += iTemp;
            Get += iTemp;
            iCount -= iTemp << 3;
        }

        if (iCount > 0)
        {
            switch (View.wTop & 7)
            {
            case 7:
                uVal = *Put;
                uVal &= 0x7f;
                uVal |= *Get & 0x80;
                *Put = uVal;
                break;

            case 6:
                uVal = *Put;
                uVal &= 0x3f;
                uVal |= *Get & 0xc0;
                *Put = uVal;
                break;

            case 5:
                uVal = *Put;
                uVal &= 0x1f;
                uVal |= *Get & 0xe0;
                *Put = uVal;
                break;

            case 4:
                uVal = *Put;
                uVal &= 0x0f;
                uVal |= *Get & 0xf0;
                *Put = uVal;
                break;

            case 3:
                uVal = *Put;
                uVal &= 0x07;
                uVal |= *Get & 0xf8;
                *Put = uVal;
                break;

            case 2:
                uVal = *Put;
                uVal &= 0x03;
                uVal |= *Get & 0xfc;
                *Put = uVal;
                break;

            case 1:
                uVal = *Put;
                uVal &= 0x01;
                uVal |= *Get & 0xfe;
                *Put = uVal;
                break;

            case 0:
                // should never get here!
                break;
            }
        }
       #else

        UCHAR *Put = mpScanPointers[wCol] + (View.wTop >> 3);

        switch(View.wTop & 7)
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

        if (iCount >= 8)
        {
            SIGNED iTemp = iCount >> 3;       
            // copy 8 pixels at a time:
            memcpy(Put, Get, iTemp);
            Put += iTemp;
            Get += iTemp;
            iCount -= iTemp << 3;
        }

        if (iCount > 0)
        {
            switch (View.wBottom & 7)
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
       #endif

        GetStart += wBytesPerLine;
    }
}

/*--------------------------------------------------------------------------*/
// here for a misaligned monochrome bitmap, bit shifting required.
/*--------------------------------------------------------------------------*/
void ProMonoScreen::DrawUnalignedBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    UCHAR uVal, uiPix, uoPix, uVal1;
    SIGNED iCount;
    UCHAR *Get;

    // always padded to nearest full byte per scan line:

    WORD wBytesPerLine = (Getmap->wHeight + 7) >> 3;
    UCHAR *GetStart = Getmap->pStart;
    GetStart += (View.wLeft - Where.x) * wBytesPerLine;

   #ifdef ROTATE_CCW
    GetStart += (Where.y + Getmap->wHeight - 1 - View.wBottom) >> 3;
   #else
    GetStart += (View.wTop - Where.y) >> 3;
   #endif

    for (SIGNED wCol = View.wLeft; wCol <= View.wRight; wCol++)
    {
        Get = GetStart;
        uVal1 = *Get++;
        iCount = View.Height();

       #ifdef ROTATE_CCW
        UCHAR *Put = mpScanPointers[wCol] - (View.wBottom >> 3);

        uiPix = ((Where.y + Getmap->wHeight - 1) - View.wBottom) & 7;
        uVal1 <<= uiPix;        // shift input pixels out the msb

        uoPix = View.wBottom & 7;
        uVal = *Put;
        uVal >>= uoPix + 1;

        while (iCount-- > 0)
        {
            uVal <<= 1;
            uVal |= uVal1 >> 7;

            if (!uoPix)
            {
                uoPix = 8;
                *Put++ = uVal;
                uVal = 0;
            }
            uoPix--;
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

        if (uoPix != 7)
        {
            uVal1 = *Put;
            uVal1 &= 0xff >> (7 - uoPix);
            uVal <<= uoPix + 1;
            uVal1 |= uVal;
            *Put = uVal1;
        }

       #else    // ROTATE_CW

        UCHAR *Put = mpScanPointers[wCol] + (View.wTop >> 3);

        uiPix = (View.wTop - Where.y) & 7;
        uVal1 <<= uiPix;        // shift input pixels out the msb

        uoPix = View.wTop & 7;
        uVal = *Put;
        uVal >>= 8 - uoPix;

        while (iCount-- > 0)
        {
            uVal <<= 1;
            uVal |= uVal1 >> 7;

            if (uoPix == 7)
            {
                uoPix = 0;
                *Put++ = uVal;
                uVal = 0;
            }
            else
            {
                uoPix++;
            }
            if (uiPix == 7)
            {
                uiPix = 0;
                uVal1 = *Get++;
            }
            else
            {
                uiPix++;
                uVal1 <<= 1;
            }
        }

        if (uoPix)
        {
            uVal1 = *Put;
            uVal1 &= 0xff >> uoPix;
            uVal <<= 8 - uoPix;
            uVal1 |= uVal;
            *Put = uVal1;
        }
       #endif
        GetStart += wBytesPerLine;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::DrawRleBitmap(const PegPoint Where, const PegRect View,
    const PegBitmap *Getmap)
{
    UCHAR *Get = Getmap->pStart;
    UCHAR uVal;
    SIGNED uCount;

    SIGNED wCol = Where.x;

    uCount = 0;

    while (wCol < View.wLeft)
    {
        uCount = 0;

        while(uCount < Getmap->wHeight)
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
        wCol++;
    }

    while (wCol <= View.wRight)
    {
       #ifdef ROTATE_CCW
        SIGNED wLoop1 = Where.y + Getmap->wHeight - 1;
        while (wLoop1 >= Where.y)
        {
       #else
        SIGNED wLoop1 = Where.y;
        while(wLoop1 < Where.y + Getmap->wHeight)
        {
       #endif

            uVal = *Get++;

            if (uVal & 0x80)        // raw packet?
            {
                uCount = (uVal & 0x7f) + 1;
                
                while (uCount--)
                {
                    uVal = *Get++;
                    if (wLoop1 <= View.wBottom &&
                        wLoop1 >= View.wTop &&
                        uVal != 0xff)
                    {
                        PlotPointView(wCol, wLoop1, uVal);
                    }
                    #ifdef ROTATE_CCW
                    wLoop1--;
                    #else
                    wLoop1++;
                    #endif
                }
            }
            else
            {
                uCount = uVal + 1;
                uVal = *Get++;

                if (uVal == 0xff)
                {
                    #ifdef ROTATE_CCW
                    wLoop1 -= uCount;
                    #else
                    wLoop1 += uCount;
                    #endif
                }
                else
                {
                    while(uCount--)
                    {
                        if (wLoop1 <= View.wBottom &&
                            wLoop1 >= View.wTop)
                        {
                            PlotPointView(wCol, wLoop1, uVal);
                        }
                        #ifdef ROTATE_CCW
                        wLoop1--;
                        #else
                        wLoop1++;
                        #endif
                    }
                }
            }
        }
        wCol++;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::RectMoveView(PegThing *Caller, const PegRect &View,
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
void ProMonoScreen::DrawTextView(PegPoint Where, const TCHAR *Text, PegColor &Color,
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
//#undef PEG_UNICODE
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
void ProMonoScreen::HidePointer(void)
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
void ProMonoScreen::SetPointer(PegPoint Where)
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
void ProMonoScreen::SetPointerType(UCHAR bType)
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
void ProMonoScreen::ResetPalette(void)
{
    SetPalette(0, 2, DefPalette);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR *ProMonoScreen::GetPalette(DWORD *pPutSize)
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
void ProMonoScreen::SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet)
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
      //** Error- the SetPalette function must be filled in **
   #endif
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void ProMonoScreen::ConfigureController(void)
{
#ifdef PEGWIN32
#else
	LCDEnable();
	LCDClear();
	//LCDSetBacklight(LCD_BACKLIGHT_ON);
	memset(s_ucBuf, 0xFF, DISPLAY_BUF_SIZE);
#endif

}


/*--------------------------------------------------------------------------*/
// function to blast our memory out to the windows screen. This is only used
// for testing purposes, and can be deleted.

void ProMonoScreen::MemoryToScreen(void)
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

    // Rotate the frame buffer data back to portrait mode in our display
    // buffer. This step is only required when running under Win32:

    UCHAR *pPutStart = mpDisplayBuff;
    WORD wBytesPerLine = mwHRes >> 3;
    wBytesPerLine += 3;
    wBytesPerLine &= 0xfffc;

    for (WORD wLoop = 0; wLoop < mwVRes; wLoop++)
    {
        UCHAR *pGet = mpScanPointers[0];
        UCHAR *pPut = pPutStart;

       #ifdef ROTATE_CCW
        pGet -= wLoop >> 3;
        UCHAR uMask = 1 << (wLoop & 7);
        UCHAR uVal = 0;
        UCHAR uTemp;

        for (WORD wLoop1 = 0; wLoop1 < mwHRes >> 3; wLoop1++)
        {
            for (WORD uBit = 0; uBit < 8; uBit++)
            {
                uVal <<= 1;
                uTemp = *pGet & uMask;
                uTemp >>= (wLoop & 7);
                uVal |= uTemp;
                pGet += mwVRes >> 3;
            }
            *pPut++ = uVal;
            uVal = 0;
        }
       #else
        pGet += wLoop >> 3;
        UCHAR uMask = 0x80 >> (wLoop & 7);
        UCHAR uVal = 0;
        UCHAR uTemp;

        for (WORD wLoop1 = 0; wLoop1 < mwHRes >> 3; wLoop1++)
        {
            for (WORD uBit = 0; uBit < 8; uBit++)
            {
                uVal <<= 1;
                uTemp = *pGet & uMask;
                uTemp >>= 7 - (wLoop & 7);
                uVal |= uTemp;
                pGet -= mwVRes >> 3;
            }
            *pPut++ = uVal;
            uVal = 0;
        }
       #endif
       pPutStart += wBytesPerLine;
    }

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

    StretchDIBits(mHdc, 0, 0, PEG_VIRTUAL_XSIZE-1, PEG_VIRTUAL_YSIZE-1,
        0, 0, PEG_VIRTUAL_XSIZE-1, PEG_VIRTUAL_YSIZE-1,
        mpDisplayBuff, (const struct tagBITMAPINFO *) &BMhead,
        DIB_RGB_COLORS, SRCCOPY);



    SelectObject(mHdc, hOldPal);
#else
    LCDWriteRaw(s_ucBuf, DISPLAY_BUF_SIZE);
#endif
}

