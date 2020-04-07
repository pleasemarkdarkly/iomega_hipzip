/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// l4scrn.cpp - PegScreen driver template for 4 bit-per-pixel
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
// is used ONLY for testing the driver. These sections of code can be removed
// to improve readability without affecting your target system.
//
// This driver is intended for systems which have direct, linear (i.e. not
// paged) access to the video frame buffer. This driver does not implement
// double buffering, although that could be added.
//
// All available configuration flags are found in the L4Screen.hpp header file.
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
//    The initial pallet programmed is the standard PEG 16-color palette.
//
// Known Limitations:
// 
// This driver implements off-screen drawing using the native 4-bpp format.
// This means that transparency information is lost if a bitmap is drawn
// to the off-screen buffer. The tradeoff here is that off-screen drawing
// uses less memory, while supporting transparency in off-screen drawing
// would double the memory requirements for the off-screen bitmap.
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include "stdlib.h"
#include "string.h"
#include <gui/peg/peg.hpp>
#include <devs/lcd/lcd.h>

#ifdef GRAYSCALE

// These are 16 grayscale values used to provide equivalant luminance
// to the default 16-color color palette. We defined "RGB" grayscale
// values for testing, however on your system you will probably only
// need one component from each row. These are also 8-bit values, and will
// need to be shifted right to match your palette register width.

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

#else

#define MAKEPALETTE
#include "pal256.hpp"   // use first 16 color of default palette
#undef MAKEPALETTE

#endif

#define DISPLAY_BUF_SIZE	LCD_HEIGHT * LCD_WIDTH * LCD_BITS_PER_PIXEL / 8
static UCHAR s_ucBuf[ DISPLAY_BUF_SIZE ];

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
    PegScreen *pScreen = new L4Screen(Rect);
    return pScreen;
}
#endif


/*--------------------------------------------------------------------------*/
// Constructor- initialize video memory addresses
/*--------------------------------------------------------------------------*/
#ifdef PEGWIN32
L4Screen::L4Screen(HWND hWnd, PegRect &Rect) : PegScreen(Rect)
#else
L4Screen::L4Screen(PegRect &Rect) : PegScreen(Rect)
#endif
{
    mdNumColors = 16;  

   #ifdef PEGWIN32
    mhPalette = NULL;
   #endif

    mwHRes = Rect.Width();
    mwVRes = Rect.Height();

    mpScanPointers = new UCHAR PEGFAR *[Rect.Height()];

    UCHAR PEGFAR *CurrentPtr = GetVideoAddress();
    WORD wPitch = mwHRes >> 1;

   #ifdef PEGWIN32

    // Windows bitmaps must be modulo-4 byte in width:
    wPitch += 3;
    wPitch &= 0xfffc;

   #endif

    for (SIGNED iLoop = 0; iLoop < Rect.Height(); iLoop++)
    {
        mpScanPointers[iLoop] = CurrentPtr;
        CurrentPtr += wPitch;
    }

    mLastPointerPos.x = Rect.Width() / 2;
    mLastPointerPos.y = Rect.Height() / 2;
    mbPointerHidden = FALSE;
    mwDrawNesting = 0;

    ConfigureController();        // set up controller registers


#ifdef PEGWIN32

   // Some setup stuff for the BitBlitting function:

   mHWnd = hWnd;
   mhPalette = NULL;
   RECT lSize;
   ::GetClientRect(mHWnd, &lSize);

   mwWinRectXOffset = (lSize.right - mwHRes) / 2;
   mwWinRectYOffset = (lSize.bottom -mwVRes) / 2;
#endif

    #ifdef GRAYSCALE
    SetPalette(0, 16, GrayPalette);
    #else
    SetPalette(0, 16, DefPalette256);
    #endif
}


/*--------------------------------------------------------------------------*/
// *** This function must be filled in by the developer ***
/*--------------------------------------------------------------------------*/
UCHAR PEGFAR *L4Screen::GetVideoAddress(void)
{
#ifdef PEGWIN32

    DWORD dSize = mwHRes / 2;
    dSize += 3;
    dSize &= 0xfffc;
    dSize *= mwVRes;
    UCHAR *pMem = new UCHAR[dSize];
    return pMem; 

#else
    // generate a 'function must return a value' error:
    return s_ucBuf;
#endif
}


/*--------------------------------------------------------------------------*/
// Destructor
/*--------------------------------------------------------------------------*/
L4Screen::~L4Screen()
{
    #ifdef PEGWIN32

    delete [] mpScanPointers[0];

    #endif

    delete mpScanPointers;
}

/*--------------------------------------------------------------------------*/
void L4Screen::BeginDraw(PegThing *)
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
void L4Screen::BeginDraw(PegThing *Caller, PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        return;
    }
    LOCK_PEG
    mpSaveScanPointers = mpScanPointers;

    if (pMap->wHeight && pMap->wWidth && pMap->pStart)
    {
        WORD wBytesPerLine = (pMap->wWidth + 1) >> 1;

        mpScanPointers = new UCHAR PEGFAR *[pMap->wHeight];
        UCHAR PEGFAR *CurrentPtr = pMap->pStart;
        for (SIGNED iLoop = 0; iLoop < pMap->wHeight; iLoop++)
        {
            mpScanPointers[iLoop] = CurrentPtr;
            CurrentPtr += wBytesPerLine;
        }
        mVirtualRect.Set(0, 0, pMap->wWidth - 1, pMap->wHeight - 1);
        mbVirtualDraw = TRUE;
    }
}

/*--------------------------------------------------------------------------*/
void L4Screen::EndDraw()
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
void L4Screen::EndDraw(PegBitmap *pMap)
{
    if (mbVirtualDraw)
    {
        mbVirtualDraw = FALSE;
        delete [] mpScanPointers;
        mpScanPointers = mpSaveScanPointers;
        UNLOCK_PEG
    }
}

/*--------------------------------------------------------------------------*/
// CreateBitmap: The default version creates an 8-bpp bitmap.
/*--------------------------------------------------------------------------*/
PegBitmap *L4Screen::CreateBitmap(SIGNED wWidth, SIGNED wHeight)
{
   PegBitmap *pMap = NULL;

    if (wWidth && wHeight)
    {
        pMap = new PegBitmap;
        pMap->wWidth = wWidth;
        pMap->wHeight = wHeight;
        wWidth++;
        wWidth /= 2;
        pMap->pStart = new UCHAR[(DWORD) wWidth * (DWORD) wHeight];
        if (!pMap->pStart)
        {
            delete pMap;
            return NULL;
        }
        // fill the whole thing with BLACK:
        memset(pMap->pStart, BLACK, (DWORD) wWidth * (DWORD) wHeight);
        pMap->uFlags = 0;
        pMap->uBitsPix = 4;
    }
    return pMap;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::LineView(SIGNED wXStart, SIGNED wYStart, SIGNED wXEnd, 
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
void L4Screen::HorizontalLine(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR *Put;
    UCHAR uVal;
    UCHAR uFill = (UCHAR) (Color | (Color << 4));

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
            uVal |= Color;
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
void L4Screen::VerticalLine(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos,
    COLORVAL Color, SIGNED wWidth)
{
    UCHAR uFill = (UCHAR) (Color | (Color << 4));

    while(wYStart <= wYEnd)
    {
        UCHAR *Put = mpScanPointers[wYStart] + (wXPos >> 1);
        SIGNED iLen = wWidth;

        if (wXPos & 1)
        {
            UCHAR uVal = *Put;
            uVal &= 0xf0;
            uVal |= Color;
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


#define SOLID_XOR
#ifdef SOLID_XOR

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
    UCHAR *Put = mpScanPointers[wYPos] + (wXStart >> 1);
    UCHAR uVal;
    SIGNED iLen = wXEnd - wXStart;

    if (!iLen)
        return;

    if (wXStart & 1)
    {
        *Put++ ^= 0x0f;
        wXStart += 1;
    }
    while(wXStart < wXEnd)
    {
        *Put++ ^= 0xff;
        wXStart += 2;
    }
    if (wXStart == wXEnd)
    {
        *Put++ ^= 0xf0;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
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

#else

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::HorizontalLineXOR(SIGNED wXStart, SIGNED wXEnd, SIGNED wYPos)
{
    UCHAR *Put = mpScanPointers[wYPos] + (wXStart >> 1);
    SIGNED iLen = wXEnd - wXStart;

    if (!iLen)
    {
        return;
    }
    while(wXStart < wXEnd - 1)
    {
        *Put++ ^= 0x0f;
        wXStart += 2;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::VerticalLineXOR(SIGNED wYStart, SIGNED wYEnd, SIGNED wXPos)
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
        wYStart += 2;
    }
}

#endif

/*--------------------------------------------------------------------------*/
void L4Screen::Capture(PegCapture *Info, PegRect &CaptureRect)
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

    if (CaptureRect.wLeft & 1)
    {
        CaptureRect.wLeft--;
    }

    if (CaptureRect.Width() & 1)
    {
        CaptureRect.wRight++;
    }

    Info->SetPos(CaptureRect);
    LONG Size = (pMap->wWidth * pMap->wHeight) >> 1;
    Info->Realloc(Size);

    UCHAR *GetStart = mpScanPointers[CaptureRect.wTop] + (CaptureRect.wLeft >> 1);

    // make room for the memory bitmap:

    pMap->uFlags = 0;        // raw format
    pMap->uBitsPix = 4;     // 4 bits per pixel

    // fill in the image with our captured info:

    UCHAR *Put = pMap->pStart;

    for (WORD wLine = 0; wLine < pMap->wHeight; wLine++)
    {
        memcpy(Put, GetStart, pMap->wWidth >> 1);
        GetStart += mwHRes >> 1;
        Put += pMap->wWidth >> 1;
    }
    Info->SetValid(TRUE);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::BitmapView(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    UCHAR uVal, uVal1;

    if (IS_RLE(Getmap))
    {
        DrawRleBitmap(Where, View, Getmap);
    }
    else
    {
        if (Getmap->uBitsPix == 4)
        {
            if (Where.x & 1)
            {
                DrawUnaligned16ColorBitmap(Where, Getmap, View);
            }
            else
            {
                Draw16ColorBitmap(Where, Getmap, View);
            }
        }
        else
        {
            // here for a source bitmap of 8-bpp:

            SIGNED iWidth = View.Width();
            UCHAR *Get = Getmap->pStart;
            Get += (View.wTop - Where.y) * Getmap->wWidth;
            Get += View.wLeft - Where.x;

            if (HAS_TRANS(Getmap))
            {
	            for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
	            {
	                UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 1);
	                if (View.wLeft & 1)
	                {
	                    uVal1 = *Put & 0xf0;
	                }
	
	                for (SIGNED wLoop1 = View.wLeft; wLoop1 <= View.wRight; wLoop1++)
	                {
	                    uVal = *Get++;
	                    if (uVal == Getmap->dTransColor)
	                    {
	                        if (wLoop1 & 1)
	                        {
	                            uVal = *Put;
	                            uVal &= 0x0f;
	                            uVal |= uVal1;
	                            *Put++ = uVal;
	                        }
	                        else
	                        {
	                            uVal1 = *Put & 0xf0;
	                        }
	                        continue;
	                    }
	
	                    if (wLoop1 & 1)
	                    {
	                        uVal1 |= uVal;
	                        *Put++ = uVal1;
	                    }
	                    else
	                    {
	                        uVal1 = uVal << 4;
	                    }
	                }
	                if (!(View.wRight & 1))
	                {
	                    uVal = *Put;
	                    uVal &= 0x0f;
	                    uVal |= uVal1;
	                    *Put = uVal;
	                }                    
	                Get += Getmap->wWidth - iWidth;
	            }
            }
            else
            {
	            for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
	            {
	                UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 1);
	                if (View.wLeft & 1)
	                {
	                    uVal1 = *Put & 0xf0;
	                }
	
	                for (SIGNED wLoop1 = View.wLeft; wLoop1 <= View.wRight; wLoop1++)
	                {
	                    uVal = *Get++;
	                    if (wLoop1 & 1)
	                    {
	                        uVal1 |= uVal;
	                        *Put++ = uVal1;
	                    }
	                    else
	                    {
	                        uVal1 = uVal << 4;
	                    }
	                }
	                if (!(View.wRight & 1))
	                {
	                    uVal = *Put;
	                    uVal &= 0x0f;
	                    uVal |= uVal1;
	                    *Put = uVal;
	                }                    
	                Get += Getmap->wWidth - iWidth;
	            }
            }
        }
    }
}


/*--------------------------------------------------------------------------*/
// here for an aligned 16-color bitmap, no nibble shifting required.
/*--------------------------------------------------------------------------*/
void L4Screen::Draw16ColorBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    // always padded to nearest full byte per scan line:

    UCHAR uVal;
    WORD wBytesPerLine = (Getmap->wWidth + 1) / 2;
    UCHAR *GetStart = Getmap->pStart;
    GetStart += (View.wTop - Where.y) * wBytesPerLine;
    GetStart += (View.wLeft - Where.x) >> 1;

    for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
    {
        UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 1);
        UCHAR *Get = GetStart;
        SIGNED iCount = View.Width();

        if (View.wLeft & 1)
        {
            uVal = *Put;
            uVal &= 0xf0;
            uVal |= *Get++ & 0x0f;
            *Put++ = uVal;
            iCount--;
        }

        if (iCount > 0)
        {
            // copy two pixels at a time:
            memcpy(Put, Get, iCount >> 1);

            // check for an odd width:

            if (iCount & 1)
            {
                Put += iCount >> 1;
                Get += iCount >> 1;
                uVal = *Put;
                uVal &= 0x0f;
                uVal |= *Get & 0xf0;
                *Put = uVal;
            }
        }
        GetStart += wBytesPerLine;
    }
}

/*--------------------------------------------------------------------------*/
// here for a misaligned 16-color bitmap, nibble shifting required.
/*--------------------------------------------------------------------------*/
void L4Screen::DrawUnaligned16ColorBitmap(const PegPoint Where, const PegBitmap *Getmap,
    const PegRect &View)
{
    UCHAR uVal, uVal1;
    SIGNED iCount;
    UCHAR *Get;

    // always padded to nearest full byte per scan line:

    WORD wBytesPerLine = (Getmap->wWidth + 1) / 2;
    UCHAR *GetStart = Getmap->pStart;
    GetStart += (View.wTop - Where.y) * wBytesPerLine;
    GetStart += (View.wLeft - Where.x) >> 1;

    for (SIGNED wLine = View.wTop; wLine <= View.wBottom; wLine++)
    {
        Get = GetStart;
        iCount = View.Width();

        // do the first pixel:

        uVal1 = *Get++;
        UCHAR *Put = mpScanPointers[wLine] + (View.wLeft >> 1);
        uVal = *Put;
        uVal &= 0xf0;
        uVal |= uVal1 >> 4;
        *Put++ = uVal;
        iCount--;

        while (iCount >= 2)
        {
            // do two pixels at a time:
            uVal = uVal1 << 4;
            uVal1 = *Get++;
            uVal |= uVal1 >> 4;
            *Put++ = uVal;
            iCount -= 2;
        }

        if (iCount)     // trailing pixel??
        {
            uVal = *Put;
            uVal &= 0x0f;
            uVal |= uVal1 << 4;
            *Put = uVal;
        }
        GetStart += wBytesPerLine;
    }
}




/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::DrawRleBitmap(const PegPoint Where, const PegRect View,
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
void L4Screen::RectMoveView(PegThing *Caller, const PegRect &View,
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
void L4Screen::DrawTextView(PegPoint Where, const TCHAR *Text, PegColor &Color,
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
void L4Screen::HidePointer(void)
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
void L4Screen::SetPointer(PegPoint Where)
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
void L4Screen::SetPointerType(UCHAR bType)
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
void L4Screen::ResetPalette(void)
{
    #ifdef GRAYSCALE
    SetPalette(0, 16, GrayPalette);
    #else
    SetPalette(0, 16, DefPalette256);
    #endif
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR *L4Screen::GetPalette(DWORD *pPutSize)
{
    *pPutSize = 16;
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
void L4Screen::SetPalette(SIGNED iFirst, SIGNED iNum, const UCHAR *pGet)
{
   #ifdef PEGWIN32

    BMhead.bmhead.biSize = sizeof(BITMAPINFOHEADER);
    BMhead.bmhead.biWidth = mwHRes;
    BMhead.bmhead.biHeight = -mwVRes;
    BMhead.bmhead.biPlanes = 1;
    BMhead.bmhead.biBitCount = 4;
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
		UCHAR *pPut = muPalette;

		for (WORD loop = 0; loop < iNum; loop++)
		{
			*pPut++ = *pGet++;
			*pPut++ = *pGet++;
			*pPut++ = *pGet++;
		}
   #endif
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void L4Screen::ConfigureController(void)
{
    #ifdef PEGWIN32
    #else
	LCDEnable();
	LCDClear();
#if defined(LCD_BACKLIGHT)
	LCDSetBacklight(LCD_BACKLIGHT_ON);
#endif
	memset(s_ucBuf, 0x00, DISPLAY_BUF_SIZE);
    #endif

}


/*--------------------------------------------------------------------------*/
// function to blast our memory out to the windows screen. This is only used
// for testing purposes, and can be deleted.


void L4Screen::MemoryToScreen(void)
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

    //SIGNED iTop = Copy.wTop;
    //SIGNED iLeft = Copy.wLeft;
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
#else   // PEGWIN32

#define CONVERT_1BPP_DISPLAY_TO_4BPP 0
#if CONVERT_1BPP_DISPLAY_TO_4BPP
    LCDWrite1to4(s_ucBuf, DISPLAY_BUF_SIZE);
#else
    LCDWriteRaw(s_ucBuf, DISPLAY_BUF_SIZE);
#endif

#endif  // PEGWIN32
}

