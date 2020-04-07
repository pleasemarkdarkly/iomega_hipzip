/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pscreen.cpp - Abstract PegScreen class. All supported environments must
//  include this module.
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
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include "stdlib.h"
#include "string.h"
#include <gui/peg/peg.hpp>

#if defined(_PEGVGASCRN_) || defined(_8106_SCREEN_)
#if defined(CADUL)
#include <builtin.h>
#elif !defined(POSIXPEG)
#define __COLORS        // don't want conio colors
#include <conio.h>
#endif
#endif



extern PegBitmap gbMouse;

#ifdef PEG_MOUSE_SUPPORT
extern PegBitmap gbVertSizeBitmap;
extern PegBitmap gbHorzSizeBitmap;
extern PegBitmap gbNWSESizeBitmap;
extern PegBitmap gbNESWSizeBitmap;
extern PegBitmap gbBeamBitmap;
extern PegBitmap gbHandBitmap;
#endif

/*------------------------ Constructor -------------------------------------*/
//
//
//
/*--------------------------------------------------------------------------*/

PegScreen::PegScreen(const PegRect & Rect)
{
    mwDrawNesting = 0;
    mdNumColors = 0;
    mwHRes = Rect.Width();
    mwVRes = Rect.Height();
    miInvalidCount = 0;
    mbVirtualDraw = FALSE;
    mwTotalViewports = 0;

    mpPointers[PPT_NORMAL].Bitmap = &gbMouse;
    mpPointers[PPT_NORMAL].xOffset = 0;
    mpPointers[PPT_NORMAL].yOffset = 0;

#ifdef PEG_MOUSE_SUPPORT
    mpPointers[PPT_VSIZE].Bitmap = &gbVertSizeBitmap;
    mpPointers[PPT_VSIZE].xOffset = gbVertSizeBitmap.wWidth / 2;
    mpPointers[PPT_VSIZE].yOffset = gbVertSizeBitmap.wHeight / 2;

    mpPointers[PPT_HSIZE].Bitmap = &gbHorzSizeBitmap;
    mpPointers[PPT_HSIZE].xOffset = gbHorzSizeBitmap.wWidth / 2;
    mpPointers[PPT_HSIZE].yOffset = gbHorzSizeBitmap.wHeight / 2;

    mpPointers[PPT_NWSE_SIZE].Bitmap = &gbNWSESizeBitmap;
    mpPointers[PPT_NWSE_SIZE].xOffset = gbNWSESizeBitmap.wWidth / 2;
    mpPointers[PPT_NWSE_SIZE].yOffset = gbNWSESizeBitmap.wHeight / 2;

    mpPointers[PPT_NESW_SIZE].Bitmap = &gbNESWSizeBitmap;
    mpPointers[PPT_NESW_SIZE].xOffset = gbNESWSizeBitmap.wWidth / 2;
    mpPointers[PPT_NESW_SIZE].yOffset = gbNESWSizeBitmap.wHeight / 2;

    mpPointers[PPT_IBEAM].Bitmap = &gbBeamBitmap;
    mpPointers[PPT_IBEAM].yOffset = gbBeamBitmap.wHeight / 2;

   #if defined(PEGWIN32) || defined(PEGWIN16)
    mpPointers[PPT_IBEAM].xOffset = gbBeamBitmap.wWidth / 2;
   #else
    mpPointers[PPT_IBEAM].xOffset = 5;
   #endif

    mpPointers[PPT_HAND].Bitmap = &gbHandBitmap;
    mpPointers[PPT_HAND].yOffset = 0;

   #if defined(PEGWIN32) || defined(PEGWIN16)
    mpPointers[PPT_HAND].xOffset = 0;
   #else
    mpPointers[PPT_HAND].xOffset = 7;
   #endif

#endif

    mpCurPointer = mpPointers[0].Bitmap;
    miCurXOffset = 0;
    miCurYOffset = 0;

   #ifdef PEG_FULL_CLIPPING
    AllocateViewportBlock();
   #endif
}


/*--------------------------------------------------------------------------*/
// CreateBitmap: The default version creates an 8-bpp bitmap.
/*--------------------------------------------------------------------------*/
PegBitmap *PegScreen::CreateBitmap(SIGNED wWidth, SIGNED wHeight)
{
   PegBitmap *pMap = NULL;

    if (wWidth && wHeight)
    {
        pMap = new PegBitmap;
        pMap->wWidth = wWidth;
        pMap->wHeight = wHeight;
        pMap->uFlags = 0;
        pMap->dTransColor = TRANSPARENCY;
        pMap->uBitsPix = 8;

        DWORD dSize = (DWORD) wWidth * (DWORD) wHeight;

       #ifdef USE_VID_MEM_MANAGER
        pMap->pStart = AllocBitmap(dSize);

        if (!pMap->pStart)  // out of video memory?
        {
            pMap->pStart = new UCHAR[dSize];    // try local memory
        }
        else
        {
            pMap->uFlags = BMF_HAS_TRANS|BMF_SPRITE;
        }
       #else
        pMap->pStart = new UCHAR[dSize];
       #endif

        if (!pMap->pStart)
        {
            delete pMap;
            return NULL;
        }
        // fill the whole thing with transparent:
        memset(pMap->pStart, TRANSPARENCY, dSize);
    }
    return pMap;
}

/*--------------------------------------------------------------------------*/
// DestroyBitmap: 
/*--------------------------------------------------------------------------*/
void PegScreen::DestroyBitmap(PegBitmap *pMap)
{
    if (pMap->pStart)
    {
        if (IS_SPRITE(pMap))
        {
            FreeBitmap(pMap->pStart);
        }
        else
        {
            delete pMap->pStart;
        }
    }
    delete pMap;
}

/*--------------------------------------------------------------------------*/
void PegScreen::Invalidate(const PegRect &Rect)
{
    LOCK_PEG
    if (miInvalidCount)
    {
        mInvalid |= Rect;
    }
    else
    {
        mInvalid = Rect;
    }
    miInvalidCount++;
}


/*--------------------------------------------------------------------------*/
UCHAR PegScreen::GetPointerType(void)
{
    for (UCHAR wLoop = 0; wLoop < NUM_POINTER_TYPES; wLoop++)
    {
        if (mpPointers[wLoop].Bitmap == mpCurPointer)
        {
            return (wLoop);
        }
    }
    return 0;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::PutPixel(PegThing *Caller, SIGNED x, SIGNED y, COLORVAL Color)
{
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }

    if (!Caller->mClip.Contains(x, y))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        PlotPointView(x, y, Color);
        return;
    }

    // find the parent window object:

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
	    if (pView->mView.Contains(x, y))
	    {
	        PlotPointView(x, y, Color);
	        break;
	    }
        pView = pView->pNext;
    }

#else

    PlotPointView(x, y, Color);

#endif
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
COLORVAL PegScreen::GetPixel(PegThing *Caller, SIGNED x, SIGNED y)
{
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return 0;
    }

    if (!Caller->mClip.Contains(x, y))
    {
        return 0;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        return GetPixelView(x, y);
    }

    // find the parent window object:

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
	    if (pView->mView.Contains(x, y))
	    {
	        return GetPixelView(x, y);
	    }
        pView = pView->pNext;
    }
    return 0;

#else

    return GetPixelView(x, y);

#endif
}



/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Line(PegThing *Caller, SIGNED wXStart, SIGNED wYStart,
    SIGNED wXEnd, SIGNED wYEnd, const PegColor &Color, SIGNED wWidth)
{
    if (!Caller || wWidth < 1)
    {
        return;
    }
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }

    if (wWidth > 1)
    {
        if (WideLine(Caller, wXStart, wYStart, wXEnd, wYEnd,
            Color, wWidth))
        {
            return;
        }
    }

    PegRect Rect;
    Rect.wLeft = wXStart;
    Rect.wRight = wXEnd;
    Rect.wTop = wYStart;
    Rect.wBottom = wYEnd;

    if (wYStart > wYEnd)
    {
        PEGSWAP(Rect.wTop, Rect.wBottom);
    }
    if (wXStart > wXEnd)
    {
        PEGSWAP(Rect.wLeft, Rect.wRight);
    }

    if (!ClipRect(Rect, Caller->mClip))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        LineView(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, wWidth);
        return;
    }

    PegRect tRect;

    // find the parent window object:

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
	    if (pView->mView.Contains(Rect))
	    {
	        LineView(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, wWidth);
	        break;
	    }
	    else
	    {    
	        if (pView->mView.Overlap(Rect))
	        {
	            tRect = Rect & pView->mView;
	            LineView(wXStart, wYStart, wXEnd, wYEnd, tRect, Color, wWidth);
	        }
	    }
        pView = pView->pNext;
    }

#else

    LineView(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, wWidth);

#endif
}


/*--------------------------------------------------------------------------*/
// WideLine-
//
// Notes: Wide horizontal and vertical lines are drawn with filled
//  rectangles to insure that clipping is done properly.
/*--------------------------------------------------------------------------*/

BOOL PegScreen::WideLine(PegThing *Caller, SIGNED xStart, SIGNED yStart,
    SIGNED xEnd, SIGNED yEnd, const PegColor &LineColor, SIGNED iWidth)
{
    PegColor Color(LineColor.uForeground, LineColor.uForeground,CF_FILL);

    PegRect LineRect;
    LineRect.Set(xStart, yStart, xEnd, yEnd);

    if (yStart == yEnd)         // wide horz line?
    {
        LineRect.wBottom += iWidth - 1;
        if (xStart > xEnd)
        {
            PEGSWAP(LineRect.wLeft, LineRect.wRight);
        }
        Rectangle(Caller, LineRect, Color, 0);

        return TRUE;
    }

    if (xStart == xEnd)         // wide vertical line?
    {
        if (yStart > yEnd)
        {
            PEGSWAP(LineRect.wTop, LineRect.wBottom);
        }
        LineRect.wRight += iWidth - 1;
        Rectangle(Caller, LineRect, Color, 0);
        return TRUE;
    }
    return FALSE;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Rectangle(PegThing *Caller, const PegRect &InRect,
    const PegColor &Color, SIGNED wWidth)
{
    if (!Caller)
    {
        return;
    }
    PegRect Rect = InRect;

    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }

    if (wWidth > 1)
    {
        // draw four wide lines:

        WideLine(Caller, InRect.wLeft, InRect.wTop, InRect.wRight,
            InRect.wTop, Color, wWidth);

        WideLine(Caller, InRect.wLeft, InRect.wBottom - wWidth + 1,
            InRect.wRight, InRect.wBottom - wWidth + 1, Color, wWidth);

        WideLine(Caller, InRect.wLeft, InRect.wTop, InRect.wLeft,
            InRect.wBottom, Color, wWidth);

        WideLine(Caller, InRect.wRight - wWidth + 1, InRect.wTop,
            InRect.wRight - wWidth + 1, InRect.wBottom, Color, wWidth);

        if (!(Color.uFlags & CF_FILL))
        {
            return;
        }
        Rect -= wWidth;
        wWidth = 0;
    }

    if (!ClipRect(Rect, Caller->mClip))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        Rectangle(InRect, Rect, Color, wWidth);
        return;
    }

    PegRect tRect;
    // find the parent window object:
    
    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
        if (pView->mView.Contains(Rect))
        {
            Rectangle(InRect, Rect, Color, wWidth);
            break;
        }
        else
        {
            if (pView->mView.Overlap(Rect))
            {
                tRect = Rect & pView->mView;
                Rectangle(InRect, tRect, Color, wWidth );
            }
        }
        pView = pView->pNext;
    }

#else

    Rectangle(InRect, Rect, Color, wWidth);
    
#endif

}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Rectangle(const PegRect &InRect, PegRect &Rect, const PegColor &Color,
    SIGNED wWidth)
{
    if (wWidth == 1)
    {
        // outline the rectangle:

        if (InRect.wTop == Rect.wTop)
        {
            HorizontalLine(Rect.wLeft, Rect.wRight, Rect.wTop,
                Color.uForeground, wWidth);
            Rect.wTop++;
        }

        if (InRect.wBottom == Rect.wBottom)
        {
            HorizontalLine(Rect.wLeft, Rect.wRight, Rect.wBottom - wWidth + 1,
                Color.uForeground, wWidth);
            Rect.wBottom--;
        }

        if (Rect.wLeft == InRect.wLeft)
        {
            VerticalLine(Rect.wTop, Rect.wBottom, Rect.wLeft,
                Color.uForeground, wWidth);
            Rect.wLeft++;
        }

        if (Rect.wRight == InRect.wRight)
        {
            VerticalLine(Rect.wTop, Rect.wBottom, Rect.wRight - wWidth + 1,
                Color.uForeground, wWidth);
            Rect.wRight--;
        }
    }

   #ifdef PEG_PROFILE_MODE

    if ((Color.uFlags & CF_FILL) && (Rect.wTop <= Rect.wBottom))
    {
        VerticalLine(Rect.wTop, Rect.wBottom, Rect.wLeft,
            Color.uBackground, Rect.Width());
    }

   #else

    if ((Color.uFlags & CF_FILL) && (Rect.wLeft <= Rect.wRight))
    {
        HorizontalLine(Rect.wLeft, Rect.wRight, Rect.wTop,
            Color.uBackground, Rect.Height());
    }

   #endif

}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::RectangleXOR(PegThing *Caller, const PegRect &InRect)
{
    PegRect Rect = InRect;

    if (!Caller)
    {
        return;
    }
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }
    if (!ClipRect(Rect, Caller->mClip))
    {
        return;
    }

    // outline the rectangle:

    if (InRect.wTop == Rect.wTop)
    {
        HorizontalLineXOR(Rect.wLeft, Rect.wRight, Rect.wTop);
    }
    if (InRect.wBottom == Rect.wBottom)
    {
        HorizontalLineXOR(Rect.wLeft, Rect.wRight, Rect.wBottom);
    }
    if (Rect.wLeft == InRect.wLeft)
    {
        VerticalLineXOR(Rect.wTop, Rect.wBottom, Rect.wLeft);
    }
    if (Rect.wRight == InRect.wRight)
    {
        VerticalLineXOR(Rect.wTop, Rect.wBottom, Rect.wRight);
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::InvertRect(PegThing *Caller, PegRect &InRect)
{
    PegRect Rect = InRect;

    if (!Caller)
    {
        return;
    }
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }
    if (!ClipRect(Rect, Caller->mClip))
    {
        return;
    }

#if defined(ROTATE_CW) || defined(ROTATE_CCW)
	for (int i = Rect.wLeft; i <= Rect.wRight; ++i)
		VerticalLineXOR(Rect.wTop, Rect.wBottom, i);
#else
	for (int i = Rect.wTop; i <= Rect.wBottom; ++i)
		HorizontalLineXOR(Rect.wLeft, Rect.wRight, i);
#endif
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Restore(PegThing *Caller, PegCapture *Info, BOOL bOnTop)
{
    if (Info->IsValid())
    {
        Bitmap(Caller, Info->Point(), Info->Bitmap(), bOnTop);
    }
}


/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Bitmap(PegThing *Caller, PegPoint Where,
    PegBitmap *Getmap, BOOL bOnTop)
{
    if (!Caller)
    {
        return;
    }
    PegRect Rect;
    Rect.wLeft = Where.x;
    Rect.wTop = Where.y;
    Rect.wRight = Rect.wLeft + Getmap->wWidth - 1;
    Rect.wBottom = Rect.wTop + Getmap->wHeight - 1;

    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }

    if (bOnTop)
    {
        if (ClipRectNoInvalid(Rect, Caller->mClip))
        {
            Rect &= Caller->mClient;
            BitmapView(Where, Getmap, Rect);
        }
        return;
    }
    else
    {
        if (!ClipRect(Rect, Caller->mClip))
        {
            return;
        }
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        BitmapView(Where, Getmap, Rect);
        return;
    }

    // find the parent window object:

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    PegRect View;
    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
        if (pView->mView.Contains(Rect))
        {
            BitmapView(Where, Getmap, Rect);
            break;
        }
        else
        {
            if (pView->mView.Overlap(Rect))
            {
                View = Rect & pView->mView;
                BitmapView(Where, Getmap, View);
            }
        }
        pView = pView->pNext;
    }
#else

     BitmapView(Where, Getmap, Rect);

#endif

}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::BitmapFill(PegThing *Caller, PegRect InRect,
    PegBitmap *Getmap)
{
    PegPoint Put;
    PegRect OldClip = Caller->mClip;
    Caller->mClip &= InRect;
    Put.x = InRect.wLeft;
    Put.y = InRect.wTop;

    while(1)
    {
        while(1)
        {
            Bitmap(Caller, Put, Getmap);
            Put.x += Getmap->wWidth;

            if (Put.x > InRect.wRight)
            {
                break;
            }
        }
        
        Put.y += Getmap->wHeight;
       
        if (Put.y > InRect.wBottom)
        {
            break;
        }
        Put.x = InRect.wLeft;
    }
    Caller->mClip = OldClip;
}


/*--------------------------------------------------------------------------*/
// RectMove- This function is a low-overhead Rectangle Move operation. It is 
// the responsibility of the caller to only call this function when "on top",
// i.e. no other windows are covering or partially covering the caller.
//
// This function is used for fast scrolling of the client area of a window.
// The moved rectangle is clipped only to the caller's clipping area, and does
// not do viewport validation.
//
// This function used the driver level "RectMoveView", which is hardware
// accelerated whenever possible.
/*--------------------------------------------------------------------------*/
void PegScreen::RectMove(PegThing *Caller, PegRect MoveRect, PegPoint MoveTo)
{
    if (!Caller)
    {
        return;
    }
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }
    SIGNED xMove = MoveTo.x - MoveRect.wLeft;
    SIGNED yMove = MoveTo.y - MoveRect.wTop;

    if (!ClipRect(MoveRect, Caller->mClip))
    {
        return;
    }

    RectMoveView(Caller, MoveRect, xMove, yMove);
}


/*--------------------------------------------------------------------------*/
// ViewportMove- This function is similar to RectMove, however this version
// does viewport validation. This can produce interesting effects, such as 
// scrolling the background of a window underneath child windows without
// affecting the child windows.
//
// This function is implemented using "Capture" and "Bitmap". This is required
// to add viewport validation. If the derived PegScreen class uses SPRITE
// style bitmaps, this automatically makes use of that feature.
/*--------------------------------------------------------------------------*/
void PegScreen::ViewportMove(PegThing *Caller, PegRect MoveRect, PegPoint MoveTo)
{
    if (!Caller)
    {
        return;
    }
    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }
    SIGNED xMove = MoveTo.x - MoveRect.wLeft;
    SIGNED yMove = MoveTo.y - MoveRect.wTop;

    if (!ClipRect(MoveRect, Caller->mClip))
    {
        return;
    }

    if (mbVirtualDraw)
    {
        RectMoveView(Caller, MoveRect, xMove, yMove);
        return;
    }

    PegCapture BlockCapture;
    PegRect CaptureRect = MoveRect;
    Capture(&BlockCapture, CaptureRect);
    BlockCapture.Shift(xMove, yMove);
    Bitmap(Caller, BlockCapture.Point(), BlockCapture.Bitmap(), FALSE);
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
SIGNED PegScreen::TextWidth(const TCHAR *Text, PegFont *Font, SIGNED iLen)
{
    SIGNED iWidth = 0;

    if (!Text || !Font)
    {
        return 0;
    }

   #ifdef PEG_UNICODE

    WORD cVal;
    PegFont *pFont;

/*
    if (!IS_VARWIDTH(Font))     // fixed width font? Not supported...
    {
        WORD wCharWidth = (WORD) Font->pOffsets;
        cVal = *Text++;
        while(cVal && iLen--)
        {
            iWidth += wCharWidth;
            cVal = *Text++;
        }
        return iWidth;
    }
*/

    cVal = *Text++;

    while(cVal && iLen--)
    {
        pFont = Font;
        while(pFont)
        {
            if (cVal >= pFont->wFirstChar &&
                cVal <= pFont->wLastChar)
            {
                break;
            }
            pFont = pFont->pNext;
        }

        if (!pFont)
        {
            cVal = *Text++;
            continue;
        }

        WORD *pOffset = pFont->pOffsets;
        WORD wOffset = cVal - pFont->wFirstChar;
        iWidth += *(pOffset + wOffset + 1) - *(pOffset + wOffset);
        cVal = *Text++;
    }
    return iWidth;

   #else

    WORD wStart = Font->wFirstChar;
    UCHAR *pt = (UCHAR *) Text;
    WORD cVal = *pt++;

    WORD *pOffset = Font->pOffsets;

    while (cVal && iLen--)
    {
        WORD wOffset = cVal - wStart;
        iWidth += *(pOffset + wOffset + 1) - *(pOffset + wOffset);
        cVal = *pt++;
    }
    return(iWidth);
   #endif
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
SIGNED PegScreen::TextHeight(const TCHAR *Text, PegFont *Font)
{
    if (!Text || !Font)
    {
        return 0;
    }
   #ifdef PEG_UNICODE
    UCHAR uMax = Font->uHeight;
    Font = Font->pNext;

    while(Font)
    {
        if (Font->uHeight > uMax)
        {
            uMax = Font->uHeight;
        }
        Font = Font->pNext;
    }
    return uMax;
   #else
    return(Font->uHeight);    
   #endif
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::DrawText(PegThing *Caller, PegPoint Where, const TCHAR *Text,
    PegColor &Color, PegFont *Font, SIGNED iCount)
{
    if (!Text)
    {
        return;
    }

    PegRect Rect;
    Rect.wLeft = Where.x;
    Rect.wTop = Where.y;
    Rect.wRight = Rect.wLeft + TextWidth(Text, Font) - 1;
    Rect.wBottom = Rect.wTop + TextHeight(Text, Font) - 1;

    if (Caller)
    {
        if (!(Caller->StatusIs(PSF_VISIBLE)))
        {
            return;
        }

        if (!ClipRect(Rect, Caller->mClip))
        {
            return;
        }
    }


#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        DrawTextView(Where, Text, Color, Font, iCount, Rect);
        return;
    }

    // find the parent window object:

    PegRect tRect;
    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->ViewportList();

    while(pView)
    {
        if (pView->mView.Contains(Rect))
        {
            DrawTextView(Where, Text, Color, Font, iCount, Rect);
            break;
        }
        else
        {
            if (pView->mView.Overlap(Rect))
            {
                tRect = Rect & pView->mView;
                DrawTextView(Where, Text, Color, Font, iCount, tRect);
            }
        }
        pView = pView->pNext;
    }
#else

    DrawTextView(Where, Text, Color, Font, iCount, Rect);

#endif
}


/*--------------------------------------------------------------------------*/
// USE_VID_MEM_MANAGER is a definition turned on by PegScreen drivers
// that use extra video memory for very fast bitmap manipulation.
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
// If we get here, it means that someone overwrote a PegBitmap that was in
// video memory. This is a catastrophic error. For debugging, we will just
// wait here forever.
/*--------------------------------------------------------------------------*/

#ifdef USE_VID_MEM_MANAGER

LONG lVidMemDamageCount;

void VidMemDamaged(void)
{
    while(1)
    {
        lVidMemDamageCount++;
    }
}
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#ifdef USE_VID_MEM_MANAGER
void PegScreen::InitVidMemManager(UCHAR *pStart, UCHAR *pEnd)
{

    mpFreeVidMem = (VID_MEM_BLOCK *) pStart;
    mpFreeVidMem->lMagic = 0x12345678;
    mpFreeVidMem->pNext = NULL;
    mpFreeVidMem->pPrev = NULL;
    mpFreeVidMem->pNextFree = NULL;
    mpFreeVidMem->lSize = (pEnd - pStart) - sizeof(VID_MEM_BLOCK);
}
#else
void PegScreen::InitVidMemManager(UCHAR *, UCHAR *)
{
}
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR *pLastBlockSave;

#ifdef USE_VID_MEM_MANAGER
UCHAR *PegScreen::AllocBitmap(DWORD dSize)
{

    LOCK_PEG
    VID_MEM_BLOCK *pBlock = mpFreeVidMem;
    VID_MEM_BLOCK *pPrevFree = NULL;
    UCHAR *pTemp;

    // insure 4-byte alignment:

    dSize += 3;
    dSize &= 0xfffffffcL;

    while(pBlock && pBlock->lSize < dSize)
    {
        pPrevFree = pBlock;
        pBlock = pBlock->pNextFree;
    }

    if (!pBlock)
    {
        UNLOCK_PEG
        return NULL;
    }

	 // see if we can split this block:

    if (pBlock->lSize - dSize > 1024)
    {
        // yeah, it would be reasonable to split this block since it is
        // at least 1K bigger than needed:

        pTemp = (UCHAR *) pBlock;
        pTemp += dSize + sizeof(VID_MEM_BLOCK);
        VID_MEM_BLOCK *pNewBlock = (VID_MEM_BLOCK *) pTemp;

        pNewBlock->pNextFree = pBlock->pNextFree;
        pNewBlock->pPrev = pBlock;
        pNewBlock->pNext = pBlock->pNext;
        pNewBlock->lSize = pBlock->lSize - dSize - sizeof(VID_MEM_BLOCK);
        pNewBlock->lMagic = 0x12345678;

		pBlock->pNext = pNewBlock;
        pBlock->pNextFree = pNewBlock;
        pBlock->lSize = dSize;

	 }

	 // unlink this block from the free list:

    if (pPrevFree)
    {
        pPrevFree->pNextFree = pBlock->pNextFree;
    }
    else
	{
        mpFreeVidMem = pBlock->pNextFree;
    }
    pBlock->pNextFree = NULL;       // mark this block as used.
    UNLOCK_PEG

    pTemp = (UCHAR *) pBlock;
    pTemp += sizeof(VID_MEM_BLOCK);
    pLastBlockSave = pTemp;
    return pTemp;
}

#else
UCHAR *PegScreen::AllocBitmap(DWORD)
{
	 return NULL;
}
#endif

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
#ifdef USE_VID_MEM_MANAGER
void PegScreen::FreeBitmap(UCHAR *pStart)
{
	 LOCK_PEG
	 pStart -= sizeof(VID_MEM_BLOCK);
	 VID_MEM_BLOCK *pBlock = (VID_MEM_BLOCK *) pStart;

	 if (pBlock->lMagic != 0x12345678)
	 {
		  VidMemDamaged();
		  UNLOCK_PEG
		  return;         // invalid block!
	 }

	 // check for an overwrite:
	 if (pBlock->pNext)
	 {
		  if (pBlock->pNext->lMagic != 0x12345678)
		  {
				VidMemDamaged();
				UNLOCK_PEG
				return;     // invalid block!
		  }
	 }

	 // everything checks out OK, put this block back into free list:

	 pBlock->pNextFree = mpFreeVidMem;
	 mpFreeVidMem = pBlock;

	 if (pBlock->pPrev)
	 {
		  if (pBlock->pPrev->pNextFree)
		  {
				// previous block is also free, combine them:

				pBlock->pPrev->pNext = pBlock->pNext;
				pBlock->pPrev->lSize += pBlock->lSize + sizeof(VID_MEM_BLOCK);
//				pBlock->pPrev->pNextFree = pBlock->pNextFree;

				if (pBlock->pNext)
				{
					 pBlock->pNext->pPrev = pBlock->pPrev;
				}
				pBlock = pBlock->pPrev;
				mpFreeVidMem = pBlock;
		  }
	 }

	 if (pBlock->pNext)
	 {
		  if (pBlock->pNext->pNextFree)
		  {
				// next block is free, combine them:

				pBlock->pNextFree = pBlock->pNext->pNextFree;
				pBlock->lSize += pBlock->pNext->lSize + sizeof(VID_MEM_BLOCK);

                // update links to remove next block:

				pBlock->pNext = pBlock->pNext->pNext;
                pBlock->pNext->pPrev = pBlock;
		  }
	 }

	 UNLOCK_PEG
}
#else
void PegScreen::FreeBitmap(UCHAR *)
{
}
#endif

/*--------------------------------------------------------------------------*/
/*
	  Begin PEG_FULL_GRAPHICS functions. These functions are only
	  included when PEG_FULL_GRAPHICS is defined. These functions
     include Polygon, PatternLine, Circle, and Ellipse.

     The PEG_FULL_GRAPHICS functions do not allow angular starting
     and ending points. For example, this Circle function always draws
     a full circle.

     The PEG_FP_GRAPHICS functions DO allow angular start/end
     parameters for circle (ARC) and Ellipse
*/     
/*--------------------------------------------------------------------------*/

#ifdef PEG_FULL_GRAPHICS

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Polygon(PegThing *Caller, PegPoint *pPoints, SIGNED iNumPoints,
    const PegColor &Color, SIGNED iWidth)
{
    if (Color.uFlags & CF_FILL)
    {
        FillPolygon(Caller, pPoints, iNumPoints, Color);

        if (Color.uForeground == Color.uBackground || !iWidth)
        {
            return;
        }
    }

    // Draw line segments to ouline the polygon:
    PegPoint *pGet = pPoints;

    for (WORD wLoop = 0; wLoop < iNumPoints - 1; wLoop++)
    {
        Line(Caller, pGet->x, pGet->y, (pGet + 1)->x, (pGet + 1)->y,
            Color, iWidth);
        pGet++;
    }

    if (iNumPoints > 2)
    {
        if (*pPoints != *pGet) 
        {
            // close the polygon:
            Line(Caller, pPoints->x, pPoints->y, pGet->x, pGet->y,
                Color, iWidth);
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::FillPolygon(PegThing *Caller, PegPoint *pPoints,
    SIGNED iNumPoints, const PegColor &Color)
{
    PegRect LimitRect = Caller->mClip;

    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }

    if (!ClipRect(LimitRect, Caller->mClip))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        FillPolygon(LimitRect, pPoints, iNumPoints, Color);
        return;
    }

    // find the parent window object:

    PegRect tRect;
    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->mpViewportList;

    while(pView)
    {
        if (pView->mView.Contains(LimitRect))
        {
            FillPolygon(LimitRect, pPoints, iNumPoints, Color);
            break;
        }
        else
        {
            if (pView->mView.Overlap(LimitRect))
            {
                tRect = LimitRect & pView->mView;
                FillPolygon(tRect, pPoints, iNumPoints, Color);
            }
        }
        pView = pView->pNext;
    }
#else

    FillPolygon(LimitRect, pPoints, iNumPoints, Color);

#endif
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::FillPolygon(const PegRect &LimitRect, PegPoint *pPoints,
    SIGNED iNumPoints,  const PegColor &Color)
{
    SIGNED iLoop, iMiny, iMaxy, iHeight;
    SIGNED *pLineEnds;

    iMiny = mwVRes;
    iMaxy = 0;

    // find the min and max y values:

    for(iLoop = 0; iLoop < iNumPoints; iLoop++)
    {
        if ((pPoints + iLoop)->y > iMaxy)
        {
            iMaxy = (pPoints + iLoop)->y;
        }

        if ((pPoints + iLoop)->y < iMiny)
        {
            iMiny = (pPoints + iLoop)->y;
        }
    }

    iHeight = iMaxy - iMiny + 1;
    pLineEnds = new SIGNED[iHeight * 2];

    // initialize line end points:

    for (iLoop = 0; iLoop < iHeight * 2; iLoop += 2)
    {
        pLineEnds[iLoop] = mwHRes;
        pLineEnds[iLoop + 1] = 0;
    }

    PegPoint *pGet = pPoints;
    SIGNED wXStart, wXEnd, wYStart, wYEnd;
    SIGNED wCurx, wCury, wNextx, wNexty, iIndex, iIndex1;

    for (WORD wLoop = 0; wLoop < iNumPoints - 1; wLoop++)
    {
        wXStart = pGet->x;
        wYStart = pGet->y;
        pGet++;
        wXEnd = pGet->x;
        wYEnd = pGet->y;

        // do an inline line draw, keeping track of the horizontal line end
        // points:

        SIGNED dx = abs(wXEnd - wXStart);
        SIGNED dy = abs(wYEnd - wYStart);

        if (!dx && !dy)
        {
            continue;
        }

        if (((dx >= dy && (wXStart > wXEnd)) ||
            ((dy > dx) && wYStart > wYEnd)))
        {
            PEGSWAP(wXEnd, wXStart);
            PEGSWAP(wYEnd, wYStart);
        }

        SIGNED y_sign, x_sign;
        y_sign = x_sign = 1;

        if (dy)
        {
            y_sign = ((int) wYEnd - (int) wYStart) / dy;
        }
        if (dx)
        {
            x_sign = ((int) wXEnd - (int) wXStart) / dx;
        }
        SIGNED decision;

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

  	            iIndex = (wCury - iMiny) << 1;

                if (wCurx < pLineEnds[iIndex])
                {
                    pLineEnds[iIndex] = wCurx;
                }
	    
                if (wCurx > pLineEnds[iIndex + 1])
                {
                    pLineEnds[iIndex + 1] = wCurx;
                }
	    
                iIndex1 = (wNexty - iMiny) << 1;
                if (wNextx < pLineEnds[iIndex1])
                {
                    pLineEnds[iIndex1] = wNextx;
                }
    
                if (wNextx > pLineEnds[iIndex1 + 1])
                {
                    pLineEnds[iIndex1 + 1] = wNextx;
                }
            }
        }
        else
        {
            for (wCurx = wXStart, wCury = wYStart, wNextx = wXEnd,
                    wNexty = wYEnd, decision = (dy >> 1);
                wCury <= wNexty; wCury++, wNexty--, decision += dx)
            {
	            iIndex = (wCury - iMiny) << 1;
                iIndex1 = (wNexty - iMiny) << 1;

                if (decision >= dy)
                {
                    decision -= dy;
                    wCurx += x_sign;
                    wNextx -= x_sign;
                }

                if (wCurx < pLineEnds[iIndex])
                {
                    pLineEnds[iIndex] = wCurx;
                }
	    
                if (wCurx > pLineEnds[iIndex + 1])
                {
                    pLineEnds[iIndex + 1] = wCurx;
                }

                if (wNextx < pLineEnds[iIndex1])
	            {
	                pLineEnds[iIndex1] = wNextx;
	            }
	    
                if (wNextx > pLineEnds[iIndex1 + 1])
                {
                    pLineEnds[iIndex1 + 1] = wNextx;
                }
            }
        }
    }

    // fill the polygon:

    wCury = iMiny;

    for (iLoop = 0; iLoop < iHeight * 2; iLoop += 2)
    {
        if (wCury >= LimitRect.wTop && wCury <= LimitRect.wBottom)
        {
            if (pLineEnds[iLoop] < LimitRect.wLeft)
            {
                pLineEnds[iLoop] = LimitRect.wLeft;
            }
            if (pLineEnds[iLoop + 1] > LimitRect.wRight)
            {
                pLineEnds[iLoop + 1] = LimitRect.wRight;
            }
            if (pLineEnds[iLoop] <= pLineEnds[iLoop + 1])
            {
                HorizontalLine(pLineEnds[iLoop], pLineEnds[iLoop + 1],
                    wCury, Color.uBackground, 1);
            }
        }
        wCury++;
    }
    delete [] pLineEnds;
}


/*--------------------------------------------------------------------------*/
// PatternLine- Draws a dashed line with pattern indicated.
/*--------------------------------------------------------------------------*/

void PegScreen::PatternLine(PegThing *Caller, SIGNED wXStart, SIGNED wYStart,
    SIGNED wXEnd, SIGNED wYEnd, PegColor &Color, SIGNED wWidth,
    DWORD dPattern)
{
    PegRect Rect;
    Rect.wLeft = wXStart;
    Rect.wRight = wXEnd;
    Rect.wTop = wYStart;
    Rect.wBottom = wYEnd;

    if (wYStart > wYEnd)
    {
        PEGSWAP(Rect.wTop, Rect.wBottom);
    }
    if (wXStart > wXEnd)
    {
        PEGSWAP(Rect.wLeft, Rect.wRight);
    }

    if (!ClipRect(Rect, Caller->mClip))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        PatternLine(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, 
        wWidth, dPattern);
        return;
    }

    // find the parent window object:
    PegRect tRect;

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->mpViewportList;

    while(pView)
    {
        if (pView->mView.Contains(Rect))
        {
            PatternLine(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, 
                wWidth, dPattern);
            break;
        }
        else
        {
            if (pView->mView.Overlap(Rect))
            {
                tRect = Rect & pView->mView;
                PatternLine(wXStart, wYStart, wXEnd, wYEnd, tRect, Color, 
                    wWidth, dPattern);
            }
        }
        pView = pView->pNext;
    }
#else
    PatternLine(wXStart, wYStart, wXEnd, wYEnd, Rect, Color, 
        wWidth, dPattern);
#endif
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::PatternLine(SIGNED wXStart, SIGNED wYStart, SIGNED wXEnd, 
    SIGNED wYEnd, PegRect &Rect, const PegColor &Color, SIGNED wWidth, DWORD dPattern)
{
    DWORD dMask = 0x00000001L;

    SIGNED dx = abs(wXEnd - wXStart);
    SIGNED dy = abs(wYEnd - wYStart);

    if (((dx >= dy && (wXStart > wXEnd)) ||
        ((dy > dx) && wYStart > wYEnd)))
    {
        PEGSWAP(wXEnd, wXStart);
        PEGSWAP(wYEnd, wYStart);
    }

    SIGNED wCurx, wCury, wNextx, wNexty, wpy, wpx;
    SIGNED y_sign, x_sign, decision;

    if(dy)
    {
        y_sign = ((int) wYEnd - (int) wYStart) / dy;
    }
    else
    {
        y_sign = 0;
    }
    
    if(dx)
    {
        x_sign = ((int) wXEnd - (int) wXStart) / dx;
    }
    else
    {
        x_sign = 0;
    }

    if (dx >= dy)
    {
        for (wCurx = wXStart, wCury = wYStart, wNextx = wXEnd,
             wNexty = wYEnd, decision = (dx >> 1);
             wCurx <= wNextx; wCurx++, wNextx--, decision += dy)
        {
            dMask = dMask != 1 ? dMask >> 1 : 0x80000000L;

            if (decision >= dx)
            {
                decision -= dx;
                wCury += y_sign;
                wNexty -= y_sign;
            }
            if (dPattern & dMask)
            {
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
    }
    else
    {
        for (wCurx = wXStart, wCury = wYStart, wNextx = wXEnd,
            wNexty = wYEnd, decision = (dy >> 1);
            wCury <= wNexty; wCury++, wNexty--, decision += dx)
        {
            dMask = dMask != 1 ? dMask >> 1 : 0x80000000L;
            if (decision >= dy)
            {
                decision -= dy;
                wCurx += x_sign;
                wNextx -= x_sign;
            }
            if (dPattern & dMask)
            {
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
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Circle(PegThing *Caller, SIGNED xCenter, SIGNED yCenter,
    SIGNED r, PegColor &Color, SIGNED iWidth)
{
    if (r <= 0)
    {
        return;
    }

    PegRect LimitRect, BoundRect;
    LimitRect.Set(xCenter - r, yCenter - r, xCenter + r, yCenter + r);
    BoundRect = LimitRect;

    if (!(Caller->StatusIs(PSF_VISIBLE)))
    {
        return;
    }
    if (!ClipRect(LimitRect, Caller->mClip))
    {
        return;
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        if (LimitRect.Contains(BoundRect))
        {
            CircleFast(xCenter, yCenter, r, Color, iWidth);
        }
        else
        {
            Circle(LimitRect, xCenter, yCenter, r, Color, iWidth);
        }
        return;
    }

    // find the parent window object:

    PegRect tRect;

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->mpViewportList;

    while(pView)
    {
        if (pView->mView.Contains(LimitRect))
        {
            if (LimitRect.Contains(BoundRect))
            {
                CircleFast(xCenter, yCenter, r, Color, iWidth);
            }
            else
            {
                Circle(LimitRect, xCenter, yCenter, r, Color, iWidth);
            }
            break;
        }
        else
        {
            if (pView->mView.Overlap(LimitRect))
            {
                tRect = LimitRect & pView->mView;
                Circle(tRect, xCenter, yCenter, r, Color, iWidth);
            }
        }
        pView = pView->pNext;
    }
#else

    CircleFast(xCenter, yCenter, r, Color, iWidth);

#endif
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Circle(const PegRect &LimitRect, SIGNED xCenter,
    SIGNED yCenter, SIGNED r, PegColor &Color, SIGNED iWidth)
{
    if (Color.uFlags & CF_FILL)
    {
        if (iWidth && Color.uForeground != Color.uBackground)
        {
            FillCircle(LimitRect, xCenter, yCenter, r, Color.uForeground);
            r -= iWidth;

            if (r > 0)
            {
                FillCircle(LimitRect, xCenter, yCenter, r, Color.uBackground);
            }
        }
        else
        {
            FillCircle(LimitRect, xCenter, yCenter, r, Color.uBackground);
        }
    }
    else
    {
        OutlineCircle(LimitRect, xCenter, yCenter, r, Color.uForeground, iWidth);
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::CircleFast(SIGNED xCenter, SIGNED yCenter, SIGNED r,
    PegColor &Color, SIGNED iWidth)
{
    if (Color.uFlags & CF_FILL)
    {
        if (iWidth && Color.uForeground != Color.uBackground)
        {
            FillCircleFast(xCenter, yCenter, r, Color.uForeground);
            r -= iWidth;

            if (r > 0)
            {
                FillCircleFast(xCenter, yCenter, r, Color.uBackground);
            }
        }
        else
        {
            FillCircleFast(xCenter, yCenter, r, Color.uBackground);
        }
    }
    else
    {
        OutlineCircleFast(xCenter, yCenter, r, Color.uForeground, iWidth);
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::FillCircle(const PegRect &LimitRect, SIGNED xCenter,
    SIGNED yCenter, SIGNED r, COLORVAL Color)
{
    SIGNED row, col_start, col_end, px, py;
    LONG sum;
    py = r << 1;
    px = 0;
    sum = -(r << 1);

    while(px <= py)
    {
        if (!(px & 1))
        {
            row = yCenter + (py >> 1);
            col_end = xCenter + (px >> 1);
            col_start = xCenter - (px >> 1);

            if (row >= LimitRect.wTop && row <= LimitRect.wBottom)
            {
                if (col_start < LimitRect.wLeft)
                {
                    col_start = LimitRect.wLeft;
                }
                if (col_end > LimitRect.wRight)
                {
                    col_end = LimitRect.wRight;
                }
                if (col_start <= col_end)
                {
                    HorizontalLine(col_start, col_end, row, Color, 1);
                }
            }
            row = yCenter - (py >> 1);
            col_end = xCenter + (px >> 1);
            col_start = xCenter - (px >> 1);                                           
            if (row >= LimitRect.wTop && row <= LimitRect.wBottom)
            {
                if (col_start < LimitRect.wLeft)
                {
                    col_start = LimitRect.wLeft;
                }
                if (col_end > LimitRect.wRight)
                {
                    col_end = LimitRect.wRight;
                }
                if (col_start <= col_end)
                {
                    HorizontalLine(col_start, col_end, row, Color, 1);
                }
            }
            row = yCenter + (px >> 1);
            col_end = xCenter + (py >> 1);
            col_start = xCenter - (py >> 1);                                           
            if (row >= LimitRect.wTop && row <= LimitRect.wBottom)
            {
                if (col_start < LimitRect.wLeft)
                {
                    col_start = LimitRect.wLeft;
                }
                if (col_end > LimitRect.wRight)
                {
                    col_end = LimitRect.wRight;
                }
                if (col_start <= col_end)
                {
                    HorizontalLine(col_start, col_end, row, Color, 1);
                }
            }
            row = yCenter - (px >> 1);
            col_end = xCenter + (py >> 1);
            col_start = xCenter - (py >> 1);                                           
            if (row >= LimitRect.wTop && row <= LimitRect.wBottom)
            {
                if (col_start < LimitRect.wLeft)
                {
                    col_start = LimitRect.wLeft;
                }
                if (col_end > LimitRect.wRight)
                {
                    col_end = LimitRect.wRight;
                }
                if (col_start <= col_end)
                {
                    HorizontalLine(col_start, col_end, row, Color, 1);
                }
            }
        }
        sum += px++;
        sum += px;

        if (sum >= 0)
        {
            sum -= py--;
            sum -= py;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::FillCircleFast(SIGNED xCenter, SIGNED yCenter, SIGNED r,
    COLORVAL Color)
{
    SIGNED row, col_start, col_end, px, py;
    LONG sum;
    py = r << 1;
    px = 0;
    sum = -(r << 1);

    while(px <= py)
    {
        if (!(px & 1))
        {
            row = yCenter + (py >> 1);
            col_end = xCenter + (px >> 1);
            col_start = xCenter - (px >> 1);

            HorizontalLine(col_start, col_end, row, Color, 1);
            row = yCenter - (py >> 1);
            col_end = xCenter + (px >> 1);
            col_start = xCenter - (px >> 1);                                           
            HorizontalLine(col_start, col_end, row, Color, 1);
            row = yCenter + (px >> 1);
            col_end = xCenter + (py >> 1);
            col_start = xCenter - (py >> 1);                                           
            HorizontalLine(col_start, col_end, row, Color, 1);
            row = yCenter - (px >> 1);
            col_end = xCenter + (py >> 1);
            col_start = xCenter - (py >> 1);                                           
            HorizontalLine(col_start, col_end, row, Color, 1);
        }
        sum += px++;
        sum += px;

        if (sum >= 0)
        {
            sum -= py--;
            sum -= py;
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::OutlineCircle(const PegRect &ClipRect, SIGNED xCenter,
    SIGNED yCenter, SIGNED r, COLORVAL Color, SIGNED iWidth)
{
    SIGNED row, col, px, py, i;
    LONG sum;
    py = r << 1;
    px = 0;
    sum = -(r << 1);
    PegRect LimitRect = ClipRect;

    while(px <= py)
    {
        col = xCenter + (px >> 1);
        row = yCenter + (py >> 1);

        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains(col, row - i))
	            PlotPointView(col, row -i, Color);
        }
        row = yCenter - (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
            if (LimitRect.Contains(col, row +i))
	            PlotPointView(col, row + i, Color);
        }
        col = xCenter - (px >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains(col, row + i))
	            PlotPointView(col, row + i, Color);           
        }
        row = yCenter + (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains(col, row - i))
	            PlotPointView(col, row - i, Color);
        }
        col = xCenter + (py >> 1);
        row = yCenter + (px >> 1);

        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains((col - i), row))
	            PlotPointView((col - i), row, Color);
        }

        row = yCenter - (px >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains((col - i), row))
	            PlotPointView((col - i), row, Color);            
        }

        col = xCenter - (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        if (LimitRect.Contains(col + i, row))
	            PlotPointView((col + i), row, Color);
        }

        row = yCenter + (px >> 1);

        for (i = 0; i < iWidth; i++)
        {
            if (LimitRect.Contains(col + i, row))
	            PlotPointView((col + i), row, Color);
        }
        sum += px++;
        if (sum >= 0)
            sum -= py--;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::OutlineCircleFast(SIGNED xCenter, SIGNED yCenter, SIGNED r,
    COLORVAL Color, SIGNED iWidth)
{
    SIGNED row, col, px, py, i;
    LONG sum;
    py = r << 1;
    px = 0;
    sum = -(r << 1);

    while(px <= py)
    {
        col = xCenter + (px >> 1);
        row = yCenter + (py >> 1);

        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView(col, row -i, Color);
        }
        row = yCenter - (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView(col, row + i, Color);
        }
        col = xCenter - (px >> 1);
        for (i = 0; i < iWidth; i++)
        {
            PlotPointView(col, row + i, Color);           
        }
        row = yCenter + (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView(col, row - i, Color);
        }
        col = xCenter + (py >> 1);
        row = yCenter + (px >> 1);

        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView((col - i), row, Color);
        }

        row = yCenter - (px >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView((col - i), row, Color);            
        }

        col = xCenter - (py >> 1);
        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView((col + i), row, Color);
        }

        row = yCenter + (px >> 1);

        for (i = 0; i < iWidth; i++)
        {
	        PlotPointView((col + i), row, Color);
        }
        sum += px++;
        if (sum >= 0)
            sum -= py--;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Ellipse(PegThing *Caller, const PegRect &Bound,
    PegColor Color, SIGNED iWidth)
{
#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
        if (Caller->mClip.Contains(Bound))
        {
            EllipseFast(Bound, Color, iWidth);
        }
        else
        {
            EllipseToView(Bound, Color, iWidth, Caller->mClip);
        }
        return;
    }

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->mpViewportList;
    PegRect ThisView;

    while(pView)
    {
        if (pView->mView.Overlap(Caller->mClip))
        {
            ThisView = pView->mView;
            ThisView &= Caller->mClip;

            if (ThisView.Contains(Bound))
            {
                EllipseFast(Bound, Color, iWidth);
                break;
            }
            else
            {
                EllipseToView(Bound, Color, iWidth, ThisView);
            }
        }
        pView = pView->pNext;
    }
#else
    EllipseFast(Bound, Color, iWidth);
#endif
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::EllipseToView(const PegRect &Bound, PegColor &color, SIGNED width,
    PegRect rView)
{
	SIGNED i, a, b, xCur, yCur, xOff, yOff, xCen, yCen;
	LONG sum, asq, bsq, px, py;

	a = (Bound.Height() - 1) >> 1;
	b = (Bound.Width() - 1) >> 1;

    if (a <= 1 || b <= 1)
    {
        return;
    }
	xCen = (Bound.wLeft + Bound.wRight) >> 1;
	yCen = (Bound.wTop + Bound.wBottom) >> 1;

	for (i = 1; i <= width; i++)
	{
		asq = (long)a*a;
		bsq = (long)b*b;
		py = a << 1;
		px = 0;
		sum = 0;
		while (asq * px <= bsq * py)
		{
			yOff = (py + 1) >> 1;
			xOff = (SIGNED) (px >> 1);
			xCur = xCen + xOff;
			yCur = yCen + yOff;
            if (rView.Contains(xCur, yCur))
            {
                PlotPointView(xCur, yCur, color.uForeground);
            }
			yCur = yCen - yOff;
            if (rView.Contains(xCur, yCur))
            {
    			PlotPointView(xCur, yCur, color.uForeground);
            }
			xCur = xCen - xOff;
            if (rView.Contains(xCur, yCur))
            {
    			PlotPointView(xCur, yCur, color.uForeground);
            }
			yCur = yCen + yOff;
            if (rView.Contains(xCur, yCur))
            {
    			PlotPointView(xCur, yCur, color.uForeground);
            }
			sum += asq*px++;
			sum += asq*px;
			if (sum > 0)
			{
				sum -= bsq*py--;
				sum -= bsq*py;
			}
		}
		sum = 0;
		py = 0;
		px = b << 1;
		while (asq * px >= bsq * py)
		{
    		yOff = (py + 1) >> 1;
			xOff = (SIGNED) (px >> 1);
			xCur = xCen + xOff;
			yCur = yCen + yOff;
            if (rView.Contains(xCur, yCur))
            {
   				PlotPointView(xCur, yCur, color.uForeground);
            }
			yCur = yCen - yOff;
            if (rView.Contains(xCur, yCur))
            {
                PlotPointView(xCur, yCur, color.uForeground);
            }
			xCur = xCen - xOff;
            if (rView.Contains(xCur, yCur))
            {
                PlotPointView(xCur, yCur, color.uForeground);
            }
            yCur = yCen + yOff;
            if (rView.Contains(xCur, yCur))
            {
 				PlotPointView(xCur, yCur, color.uForeground);
            }
			sum += bsq * py++;
			sum += bsq * py;
			if (sum > 0)
			{
				sum -= asq * px--;
				sum -= asq * px;
			}
		}
		a--;
		b--;

        if (!a || !b)
        {
            return;
        }
	}

    if (color.uFlags & CF_FILL)
    {
        PegRect FillRect = Bound;
        FillRect -= width;
        mpCoord = new SIGNED[FillRect.Height() * 2];
        rView &= FillRect;

        PlotEllipsePoints(FillRect);
        SIGNED *pGet = mpCoord;
        pGet += (rView.wTop - FillRect.wTop) * 2;
        SIGNED x1, x2;

        for (SIGNED iLoop = rView.wTop; iLoop <= rView.wBottom; iLoop++)
        {
            x1 = *pGet++;
            x2 = *pGet++;
            if (x1 < rView.wLeft)
            {
                x1 = rView.wLeft;
            }
            if (x2 > rView.wRight)
            {
                x2 = rView.wRight;
            }
            if (x1 <= x2)
            {
                HorizontalLine(x1, x2, iLoop, color.uBackground, 1);
            }
        }
        delete mpCoord;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::EllipseFast(const PegRect &Bound, PegColor &color, SIGNED width)
{
	SIGNED i, a, b, xCur, yCur, xOff, yOff, xCen, yCen;
	LONG sum, asq, bsq, px, py;

	a = (Bound.Height() - 1) >> 1;
	b = (Bound.Width() - 1) >> 1;

    if (a <= 1 || b <= 1)
    {
        return;
    }
	xCen = (Bound.wLeft + Bound.wRight) >> 1;
	yCen = (Bound.wTop + Bound.wBottom) >> 1;

	for (i = 1; i <= width; i++)
	{
		asq = (long)a*a;
		bsq = (long)b*b;
		py = a << 1;
		px = 0;
		sum = 0;
		while (asq * px <= bsq * py)
		{
			yOff = (py + 1) >> 1;
			xOff = (SIGNED) (px >> 1);
			xCur = xCen + xOff;
			yCur = yCen + yOff;
            PlotPointView(xCur, yCur, color.uForeground);
			yCur = yCen - yOff;
  			PlotPointView(xCur, yCur, color.uForeground);
			xCur = xCen - xOff;
  			PlotPointView(xCur, yCur, color.uForeground);
			yCur = yCen + yOff;
   			PlotPointView(xCur, yCur, color.uForeground);
			sum += asq*px++;
			sum += asq*px;
			if (sum > 0)
			{
				sum -= bsq*py--;
				sum -= bsq*py;
			}
		}
		sum = 0;
		py = 0;
		px = b << 1;
		while (asq * px >= bsq * py)
		{
			yOff = (py + 1) >> 1;
			xOff = (SIGNED) (px >> 1);
			xCur = xCen + xOff;
			yCur = yCen + yOff;
    		PlotPointView(xCur, yCur, color.uForeground);
			yCur = yCen - yOff;
            PlotPointView(xCur, yCur, color.uForeground);
			xCur = xCen - xOff;
            PlotPointView(xCur, yCur, color.uForeground);
            yCur = yCen + yOff;
  	    	PlotPointView(xCur, yCur, color.uForeground);
			sum += bsq * py++;
			sum += bsq * py;
			if (sum > 0)
			{
				sum -= asq * px--;
				sum -= asq * px;
			}
		}
		a--;
		b--;

        if (!a || !b)
        {
            return;
        }
	}

    if (color.uFlags & CF_FILL)
    {
        PegRect FillRect = Bound;
        FillRect -= width;
        mpCoord = new SIGNED[FillRect.Height() * 2];
        PlotEllipsePoints(FillRect);
        SIGNED *pGet = mpCoord;
        SIGNED x1, x2;

        for (SIGNED iLoop = FillRect.wTop; iLoop <= FillRect.wBottom; iLoop++)
        {
            x1 = *pGet++;
            x2 = *pGet++;
            if (x1 <= x2)
            {
                HorizontalLine(x1, x2, iLoop, color.uBackground, 1);
            }
        }
        delete mpCoord;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::PlotEllipsePoints(PegRect &Bound)
{
	SIGNED i, a, b, xCur, yCur, xOff, yOff, xCen, yCen;
	LONG sum, asq, bsq, px, py;
    SIGNED *pPut = mpCoord;

    for (i = Bound.wTop; i <= Bound.wBottom; i++)
    {
        *pPut++ = mwHRes;
        *pPut++ = 0;
    }

	a = (Bound.Height() - 1) >> 1;
	b = (Bound.Width() - 1) >> 1;

    if (a <= 1 || b <= 1)
    {
        return;
    }
	xCen = (Bound.wLeft + Bound.wRight) >> 1;
	yCen = (Bound.wTop + Bound.wBottom) >> 1;

	asq = (long)a*a;
	bsq = (long)b*b;
	py = a << 1;
	px = 0;
	sum = 0;
	while (asq * px <= bsq * py)
	{
		yOff = (py + 1) >> 1;
		yCur = yCen - yOff;

        if (yCur >= Bound.wTop)
        {
		    xOff = (SIGNED) (px >> 1);
    		xCur = xCen - xOff;

            pPut = mpCoord + ((yCur - Bound.wTop) << 1);
            if (xCur < *pPut)
            {
                *pPut = xCur;
            }
            pPut++;
		    xCur = xCen + xOff;
            if (xCur > *pPut)
            {
                *pPut = xCur;
            }
        }

		yCur = yCen + yOff;

        if (yCur <= Bound.wBottom)
        {
            pPut = mpCoord + ((yCur - Bound.wTop) << 1);
		    xCur = xCen - xOff;
            if (xCur < *pPut)
            {
                *pPut = xCur;
            }
            pPut++;
		    xCur = xCen + xOff;
            if (xCur > *pPut)
            {
                *pPut = xCur;
            }
        }
		sum += asq*px++;
		sum += asq*px;
		if (sum > 0)
		{
			sum -= bsq*py--;
			sum -= bsq*py;
		}
	}
	sum = 0;
	py = 0;
	px = b << 1;
	while (asq * px >= bsq * py)
	{
    	yOff = (py + 1) >> 1;
		yCur = yCen - yOff;

        if (yCur >= Bound.wTop)
        {
	        xOff = (SIGNED) (px >> 1);
		    xCur = xCen - xOff;

            pPut = mpCoord + ((yCur - Bound.wTop) << 1);
            if (xCur < *pPut)
            {
                *pPut = xCur;
            }
            pPut++;
		    xCur = xCen + xOff;
            if (xCur > *pPut)
            {
                *pPut = xCur;
            }   
        }
  		yCur = yCen + yOff;

        if (yCur <= Bound.wBottom)
        {
            pPut = mpCoord + ((yCur - Bound.wTop) * 2);
  		    xCur = xCen - xOff;
            if (xCur < *pPut)
            {
                *pPut = xCur;
            }
            pPut++;
            xCur = xCen + xOff;
            if (xCur > *pPut)
            {
                *pPut = xCur;
            }
        }
		sum += bsq * py++;
		sum += bsq * py;
		if (sum > 0)
		{
			sum -= asq * px--;
			sum -= asq * px;
		}
	}
}

#endif  // end of PEG_FULL_GRAPHICS functions


#ifdef PEG_FP_GRAPHICS

#define compare(a,b,c)  {if (a<b) b = a; if (a>c) c = a;}
SIGNED minrow;
SIGNED maxrow;

#include "math.h"
#include "float.h"

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Arc(PegThing *Caller, SIGNED xc, SIGNED yc, SIGNED radius,
    float start_angle, float end_angle, PegColor Color, SIGNED width)
{
    float temp;

    if (start_angle >= 0 && end_angle < 0)
    {
        temp = start_angle;
        start_angle = end_angle;
        end_angle = temp;
    }

    while(start_angle < 0)
    {
        start_angle += 360;
    }
	while (start_angle >= 360)
    {
		start_angle -= 360;
    }
    while(end_angle < 0)
    {
        end_angle += 360;
    }
	while (end_angle >= 360)
    {
		end_angle -= 360;
    }
    if (radius - (width >> 1) < 1)
    {
        radius = 1;
        width = 1;
    }

    if (Color.uFlags & CF_FILL)
    {
        mpCoord = new SIGNED[mwVRes * 2];
    }

#ifdef PEG_FULL_CLIPPING

    if (mbVirtualDraw)
    {
	    if (Color.uFlags & CF_FILL)
	    {
            ArcFill(xc, yc, radius, start_angle, end_angle,
               Color, Caller->mClip);
            if (width)
            {
                ArcToView(xc, yc, radius, start_angle, end_angle,
                    Color, width + 1, Caller->mClip);
                radius -= width;
            }
            delete mpCoord;
	    }
        else
        {
            ArcToView(xc, yc, radius, start_angle, end_angle,
                Color, width, Caller->mClip);
        }
        return;
    }

    while(!Caller->StatusIs(PSF_VIEWPORT))
    {
        Caller = Caller->Parent();
    }

    Viewport *pView = Caller->mpViewportList;
    PegRect ThisView;

    while(pView)
    {
        if (pView->mView.Overlap(Caller->mClip))
        {
            ThisView = pView->mView;
            ThisView &= Caller->mClip;

       	    if (Color.uFlags & CF_FILL)
	        {
	            ArcFill(xc, yc, radius, start_angle, end_angle,
	                Color, ThisView);
                if (width)
                {
                    ArcToView(xc, yc, radius, start_angle, end_angle,
                        Color, width, ThisView);
                }
            }
	        else
            {
                ArcToView(xc, yc, radius, start_angle, end_angle,
                    Color, width, ThisView);
            }
        }
        pView = pView->pNext;
    }

#else
    ArcToView(xc, yc, radius, start_angle, end_angle,
        Color, width, Caller->mClip);
#endif
    if (Color.uFlags & CF_FILL)
    {
        delete mpCoord;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::ArcToView(SIGNED xc, SIGNED yc, SIGNED radius, float start_angle,
	float end_angle, PegColor Color, SIGNED width, PegRect &View)
{
	SIGNED arcs[8], i, j, px, py, start_sector,
		end_sector,	x_start_test, x_end_test, y_start_test,
		y_end_test;
	LONG sum;

	for (j=0; j<8; j++)
		arcs[j] = 0;

	start_sector = (int)(start_angle/45) % 8;
	end_sector = (int)(end_angle/45) % 8;
	x_start_test = int(xc+radius*cos(start_angle * 0.017453));
	x_end_test = int(xc+radius*cos(end_angle * 0.017453));
	y_start_test = int(yc+radius*sin(start_angle * 0.017453));
	y_end_test = int(yc+radius*sin(end_angle * 0.017453));

	if (start_sector == end_sector)
	{
		if (end_angle < start_angle)
		{
			arcs[start_sector] = 5;
			for (i=0; i<8; i++)
			{
				if (i != start_sector)
					arcs[i] = 2;
			}
		}
		else
			arcs[start_sector] = 4;
	}
	else
	{
		arcs[start_sector] = 1;
		arcs[end_sector] = 3;
		for (i=(start_sector+1)%8; i!=end_sector; i= (i+1)%8)
			arcs[i] = 2;
	}
	for (j=0; j<width; j++)
	{
		py = radius << 1;
		px = 0;
		sum = -py;
		while (px <= py)
		{
			ArcPlot(xc+(px>>1), yc-(py>>1), -xc-(px>>1), arcs[1],
				Color.uForeground, -x_start_test, -x_end_test, View);
			ArcPlot(xc+(px>>1), yc+(py>>1), xc+(px>>1), arcs[6],
				Color.uForeground, x_start_test, x_end_test, View);
			ArcPlot(xc-(px>>1), yc-(py>>1), -xc+(px>>1), arcs[2],
				Color.uForeground, -x_start_test, -x_end_test, View);
			ArcPlot(xc-(px>>1), yc+(py>>1), xc-(px>>1), arcs[5],
				Color.uForeground, x_start_test, x_end_test, View);
			ArcPlot(xc+(py>>1), yc-(px>>1), yc+(px>>1), arcs[0],
				Color.uForeground, y_start_test, y_end_test, View);
			ArcPlot(xc+(py>>1), yc+(px>>1), yc-(px>>1), arcs[7],
				Color.uForeground, y_start_test, y_end_test, View);
			ArcPlot(xc-(py>>1), yc-(px>>1), -yc-(px>>1), arcs[3],
				Color.uForeground, -y_start_test, -y_end_test, View);
			ArcPlot(xc-(py>>1), yc+(px>>1), -yc+(px>>1), arcs[4],
				Color.uForeground, -y_start_test, -y_end_test, View);
			sum += px++;
			sum += px;
			if (sum > 0)
			{
				sum -= py--;
				sum -= py;
			}
		}
		radius++;
	}
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::ArcPlot(SIGNED x, SIGNED y, SIGNED test_var, SIGNED type,
    COLORVAL color,	SIGNED start_test, SIGNED end_test, PegRect &View)
{
    if (!View.Contains(x, y))
    {
        return;
    }

	switch(type)
	{
	case 1:
		if (test_var >= start_test)
			PlotPointView(x,y,color);
		break;

	case 2:
		PlotPointView(x,y,color);
		break;
	case 3:
		if (test_var <= end_test)
			PlotPointView(x,y,color);
		break;
	case 4:
		if ((test_var >= start_test) && (test_var <= end_test))
			PlotPointView(x,y,color);
		break;

	case 5:
		if ((test_var <= end_test) || (test_var >= start_test))
			PlotPointView(x,y,color);
		break;
	}
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::CheckArcPoint(SIGNED x, SIGNED y, SIGNED test_var,
    SIGNED type, SIGNED start, SIGNED end, PegRect &View)
{
    if (y < 0 || y >= mwVRes)
    {
        return;
    }
    int iStat = 0;

	switch(type)
	{
		case 1:
			if (test_var >= start)
                iStat = 1;
			break;

		case 2:
            iStat = 1;
			break;

		case 3:
			if (test_var <= end)
                iStat = 1;
			break;

		case 4:
			if ((test_var >= start) && (test_var <=  end))
                iStat = 1;
			break;

		case 5:
			if ((test_var <= end) || (test_var >= start))
                iStat = 1;
			break;

		default:
			return;
	}
    if (iStat)
    {
        SIGNED iIndex = y << 1;
	    compare(x, *(mpCoord + iIndex), *(mpCoord + iIndex + 1));
    	compare(y, minrow, maxrow);
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::ArcLine(SIGNED xs, SIGNED ys, SIGNED xe, SIGNED ye,
    PegRect &View)
{
	SIGNED dx, dy, y_sign, x, y, decision;

	if (xs > xe)
	{
		PEGSWAP(xs, xe);
        PEGSWAP(ys, ye);
	}
	dx = abs(xe - xs);
	dy = abs(ye - ys);
	y_sign = 1;
	if ((ye - ys) < 0)
		y_sign = -1;
	if (dx > dy)
	{
		for (x=xs,y=ys,decision=0; x<=xe; x++,decision+=dy)
		{
			if (decision>=dx)
			{
				decision -= dx;
				y+= y_sign;
			}

            if (y >= View.wTop && y <= View.wBottom)
            {
                SIGNED iIndex = y<<1;
  			    compare(x, *(mpCoord + iIndex), *(mpCoord + iIndex + 1));
    		    compare(y,minrow,maxrow);
            }
		}
	}
	else
	{
		for (x=xs,y=ys,decision=0; y!=ye; y+=y_sign,decision+=dx)
		{
			if (decision>=dy)
			{
				decision -= dy;
				x++;
			}
            if (y >= View.wTop && y <= View.wBottom)
            {
                SIGNED iIndex = y<<1;
                compare(x, *(mpCoord + iIndex), *(mpCoord + iIndex + 1));
  			    compare(y,minrow,maxrow);
            }
		}
	}
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::ArcFill(SIGNED xc, SIGNED yc, SIGNED radius, float start_angle,
	float end_angle, PegColor color, PegRect &View)
{
	LONG sum;
	SIGNED arc[8], i, j, px, py, start_sector, end_sector,
		x_start_test, x_end_test, y_start_test, y_end_test, flag=1;
	float temp_angle;

	temp_angle = (float) end_angle - start_angle;
	if (temp_angle < 0)
		temp_angle += 360;
	if (temp_angle > 180)
	{
		flag++;
		temp_angle = end_angle;
		end_angle = (start_angle + 180);
		end_angle = (float) fmod(end_angle, 360);
	}
    if (radius < 1)
    {
        radius = 1;
    }
	while (flag > 0)
	{
		minrow = mwVRes;
		maxrow = 0;
        SIGNED *p1 = mpCoord;

		for (i=0; i<mwVRes; i++)
		{
			*p1++ = mwHRes;
            *p1++ = 0;
		}
		for (j=0; j<8; j++)
			arc[j] = 0;
		start_sector = (int)(start_angle/45) % 8;
		end_sector = (int)(end_angle/45) % 8;
		x_start_test = (int)(xc+radius*cos(start_angle*.017453));
		x_end_test = (int)(xc+radius*cos(end_angle*.017453));
		y_start_test = (int)(yc+radius*sin(start_angle*.017453));
		y_end_test = (int)(yc+radius*sin(end_angle*.017453));
		//y_end_test = (int)(yc-radius*sin(end_angle*.017453));
		if (start_sector == end_sector)
		{
			if (end_angle < start_angle)
			{
				arc[start_sector] = 5;
				for (i=0; i<8; i++)
				{
					if (i != start_sector)
						arc[i] = 2;
				}
			}
			else
				arc[start_sector] = 4;
		}
		else
		{
			arc[start_sector] = 1;
			arc[end_sector] = 3;
			for (i=(start_sector+1)%8; i!=end_sector; i= (i+1)%8)
				arc[i] = 2;
		}
		py = radius << 1;
		px = 0;
		sum = -py;
		while (px <= py)
		{
			if (!(px & 1))
			{
				CheckArcPoint(xc+(px>>1), yc-(py>>1), -xc-(px>>1), arc[1],
					-x_start_test, -x_end_test, View);
				CheckArcPoint (xc+(px>>1), yc+(py>>1), xc+(px>>1), arc[6],
					x_start_test, x_end_test, View);
				CheckArcPoint (xc-(px>>1), yc-(py>>1), -xc+(px>>1), arc[2],
					-x_start_test, -x_end_test, View);
				CheckArcPoint (xc-(px>>1), yc+(py>>1), xc-(px>>1), arc[5],
					x_start_test, x_end_test, View);
				CheckArcPoint (xc+(py>>1), yc-(px>>1), yc+(px>>1), arc[0],
					y_start_test, y_end_test, View);
				CheckArcPoint (xc+(py>>1), yc+(px>>1), yc-(px>>1), arc[7],
					y_start_test, y_end_test, View);
				CheckArcPoint (xc-(py>>1), yc-(px>>1), -yc-(px>>1), arc[3],
					-y_start_test, -y_end_test, View);
				CheckArcPoint (xc-(py>>1), yc+(px>>1), -yc+(px>>1), arc[4],
					-y_start_test, -y_end_test, View);
			}
			sum += px++;
			sum += px;
			if (sum > 0)
			{
				sum -= py--;
				sum -= py;
			}
		}

        switch(start_sector)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            ArcLine(xc, yc, x_start_test, yc - (y_start_test - yc), View);
            break;

        case 4:
        case 5:
        case 6:
        case 7:
            ArcLine(xc, yc, x_start_test, yc + (yc - y_start_test), View);
            break;
        }

        switch(end_sector)
        {
        case 0:
        case 1:
        case 2:
        case 3:
            ArcLine(xc, yc, x_end_test, yc - (y_end_test - yc), View);
            break;

        case 4:
        case 5:
        case 6:
        case 7:
            ArcLine(xc, yc, x_end_test, yc + (yc - y_end_test), View);
            break;
        }

        SIGNED x1, x2;

        if (minrow < View.wTop)
        {
            minrow = View.wTop;
        }
        if (maxrow > View.wBottom)
        {
            maxrow = View.wBottom;
        }

		for (i=minrow; i<= maxrow; i++)
		{
            SIGNED iIndex = i << 1;
            x1 = *(mpCoord + iIndex);
            x2 = *(mpCoord + iIndex + 1);
            /*
			if (x1 == x2)
            {
    			if ((i != minrow) && (i != maxrow))
	    		{
		    		x1 = (*(mpCoord + iIndex - 2) + *(mpCoord + iIndex + 2))/2;
			    	x2 = (*(mpCoord + iIndex - 1) + *(mpCoord + iIndex + 3))/2;
				}
            }
            */
            if (x1 < View.wLeft)
            {
                x1 = View.wLeft;
            }
            if (x2 > View.wRight)
            {
                x2 = View.wRight;
            }
            if (x1 <= x2)
            {
			    HorizontalLine(x1, x2, i, color.uBackground, 1);
            }
		}
		for (i=0; i<8; i++)
			arc[i] = 0;
		flag--;
		start_angle = end_angle;
		end_angle = temp_angle;
	}
}


#endif  // end of PEG_FP_GRAPHICS functions


#ifdef PEG_FULL_CLIPPING

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::GenerateViewportList(PegThing *pStart)
{
    while(!pStart->StatusIs(PSF_VIEWPORT))
    {
        pStart = pStart->Parent();
    }
    FreeViewports(pStart);

    AddViewport(pStart, pStart->mReal);
}                   


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
Viewport *PegScreen::GetFreeViewport(void)
{
    Viewport *pNew;

    if (!mpFreeListStart)
    {
        AllocateViewportBlock();
    }
    pNew = mpFreeListStart;
    mpFreeListStart = pNew->pNext;
    return pNew;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::AllocateViewportBlock()
{
    mpFreeListStart = new Viewport[VIEWPORT_LIST_INCREMENT];
    mpFreeListEnd = mpFreeListStart;

    for (SIGNED wLoop = 0; wLoop < VIEWPORT_LIST_INCREMENT - 1; wLoop++)
    {
        mpFreeListEnd->pNext = mpFreeListEnd + 1;
        mpFreeListEnd++;
    }
    mpFreeListEnd->pNext = NULL;
    mwTotalViewports += VIEWPORT_LIST_INCREMENT;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::FreeViewports(PegThing *pCaller)
{
    Viewport *pStart = pCaller->mpViewportList;

    if (pStart)
    {
        if (!mpFreeListStart)
        {
            mpFreeListStart = pStart;
        }
        else
        {
            mpFreeListEnd->pNext = pStart;
        }
        mpFreeListEnd = pStart;
        while(mpFreeListEnd->pNext)
        {
            mpFreeListEnd = mpFreeListEnd->pNext;
        }
    }
    pCaller->mpViewportList = NULL;

    PegThing *pChild = pCaller->First();

    while(pChild)
    {
        if (pChild->StatusIs(PSF_VIEWPORT))
        {
            FreeViewports(pChild);
        }
        pChild = pChild->Next();
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::AddViewport(PegThing *pTarget, PegRect NewView)
{
    // scan the list, if I get through clean then add this this view
    // to the viewport list. If this view is completly covered, then 
    // just return. If this view is partially covered, then split it
    // into pieces and try to add the pieces:
    
    PegThing *Child;

    // This loop checks to see if any sibling objects that also have
    // VIEWPORT status are on top of this viewport:

    if (pTarget->Parent())
    {
        Child = pTarget->Parent()->First();

        while(Child && Child != pTarget)
        {
            if (Child->StatusIs(PSF_VIEWPORT))
            {
                if (Child->mReal.Contains(NewView))
                {
                    // In this case, a sibling window is completely
                    // covering the target window, just return;
                    return;
                }
                if (Child->mReal.Overlap(NewView))
                {
                    SplitView(pTarget, Child->mReal, NewView);
                    return;
                }
            }
            Child = Child->Next();
        }
    }

    // This loop checks to see if any children of the current object
    // have viewport status. If they do, and they overlap the current
    // viewport, the viewport has to be split up:

    Child = pTarget->First();
    while(Child)
    {
        if (Child->StatusIs(PSF_VIEWPORT))
        {
            if (Child->mClip.Overlap(NewView))
            {
                SplitView(pTarget, Child, NewView);
                return;
            }
        }
        Child = Child->Next();
    }

    // we made it through the list, add this guy in:

    Viewport *pNew = GetFreeViewport();
    pNew->mView = NewView;

    if (pTarget->mpViewportList)
    {
        pNew->pNext = pTarget->mpViewportList;
    }
    else
    {
        pNew->pNext = NULL;
    }
    pTarget->mpViewportList = pNew;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::SplitView(PegThing *pTarget, PegThing *Child, PegRect Under)
{
    SplitView(pTarget, Child->mClip, Under);
    PegRect ChildView = Under & Child->mClip;
    AddViewport(Child, ChildView);    
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::SplitView(PegThing *pTarget, PegRect OnTop, PegRect Under)
{
    PegRect NewRect;

    if (Under.wTop < OnTop.wTop)
    {
        // make a new rectangle covering the top of the new viewport:
        NewRect.wTop = Under.wTop;
        NewRect.wLeft = Under.wLeft;
        NewRect.wRight = Under.wRight;
        NewRect.wBottom = OnTop.wTop - 1;
        AddViewport(pTarget, NewRect);
    }

    if (Under.wBottom > OnTop.wBottom)
    {
        // make a new rectangle covering the bottom of the new viewport:
        NewRect.wBottom = Under.wBottom;
        NewRect.wTop = OnTop.wBottom + 1;
        NewRect.wLeft = Under.wLeft;
        NewRect.wRight = Under.wRight;
        AddViewport(pTarget, NewRect);
    }

    if (Under.wRight > OnTop.wRight)
    {
        NewRect.wLeft = OnTop.wRight + 1;
        NewRect.wRight = Under.wRight;

        if (Under.wTop > OnTop.wTop)
        {
            NewRect.wTop = Under.wTop;
        }
        else
        {
            NewRect.wTop = OnTop.wTop;
        }

        if (Under.wBottom < OnTop.wBottom)
        {
            NewRect.wBottom = Under.wBottom;
        }
        else
        {
            NewRect.wBottom = OnTop.wBottom;
        }
        AddViewport(pTarget, NewRect);
    }

    if (Under.wLeft < OnTop.wLeft)
    {
        NewRect.wLeft = Under.wLeft;
        NewRect.wRight = OnTop.wLeft - 1;

        if (Under.wTop > OnTop.wTop)
        {
            NewRect.wTop = Under.wTop;
        }
        else
        {
            NewRect.wTop = OnTop.wTop;
        }

        if (Under.wBottom < OnTop.wBottom)
        {
            NewRect.wBottom = Under.wBottom;
        }
        else
        {
            NewRect.wBottom = OnTop.wBottom;
        }
        AddViewport(pTarget, NewRect);
    }
}

#endif  // PEG_FULL_CLIPPING if



#ifdef PEG_VECTOR_FONTS

#include "pvecfont.hpp"

/*--------------------------------------------------------------------------*/
// FontLine- Draws a 'line' directly into a PegFont bitmap.
/*--------------------------------------------------------------------------*/
void PegScreen::FontLine(UCHAR *pData, WORD wBitOffset,
    PegPoint Start, PegPoint End, WORD wBytesPerRow)
{

    UCHAR *pBase;
    UCHAR uMask;

    if (Start.y == End.y)
    {
        if (Start.x > End.x)
        {
            PEGSWAP(Start.x, End.x)
        }
        wBitOffset += Start.x;
        pBase = pData + (wBitOffset >> 3);
        pBase += Start.y * wBytesPerRow;
        uMask = 0x80 >> (wBitOffset & 7);
        while(Start.x <= End.x)
        {
            *pBase |= uMask;
            uMask >>= 1;
            if (!uMask)
            {
                uMask = 0x80;
                pBase++;
            }
            Start.x++;
        }
        return;
    }
    if (Start.x == End.x)
    {
        if (Start.y > End.y)
        {
            PEGSWAP(Start.y, End.y);
        }
        wBitOffset += Start.x;
        pBase = pData + (wBitOffset >> 3);
        pBase += Start.y * wBytesPerRow;
        uMask = 0x80 >> (wBitOffset & 7);
        while(Start.y <= End.y)
        {
            *pBase |= uMask;
            pBase += wBytesPerRow;
            Start.y++;
        }
        return;
    }

    SIGNED dx = abs(End.x - Start.x);
    SIGNED dy = abs(End.y - Start.y);

    if (((dx >= dy && (Start.x > End.x)) ||
        ((dy > dx) && Start.y > End.y)))
    {
        PEGSWAP(End.x, Start.x);
        PEGSWAP(End.y, Start.y);
    }

    SIGNED y_sign = ((int) End.y - (int) Start.y) / dy;
    SIGNED x_sign = ((int) End.x - (int) Start.x) / dx;
    SIGNED decision;
    
    SIGNED wCurx, wCury;

    wBitOffset += Start.x;
    pBase = pData + (wBitOffset >> 3);
    pBase += Start.y * wBytesPerRow;

    if (dx >= dy)
    {
        SIGNED iPointerAdd = wBytesPerRow * y_sign;
        uMask = 0x80 >> (wBitOffset & 7);

        for (wCurx = Start.x, decision = (dx >> 1);
             wCurx <= End.x; wCurx++, decision += dy)
        {
            if (decision >= dx)
            {
                decision -= dx;
                pBase += iPointerAdd;
            }
            *pBase |= uMask;
            uMask >>= 1;

            if (!uMask)
            {
                uMask = 0x80;
                pBase++;
            }
        }
    }
    else
    {
        uMask = 0x80 >> (wBitOffset & 7);
        for (wCury = Start.y, decision = (dy >> 1);
            wCury <= End.y; wCury++, decision += dx)
        {
            if (decision >= dy)
            {
                decision -= dy;
                if (x_sign > 0)
                {
                    uMask >>= 1;
                    if (!uMask)
                    {
                        pBase++;
                        uMask = 0x80;
                    }
                }
                else
                {
                    uMask <<= 1;
                    if (!uMask)
                    {
                        uMask = 0x01;
                        pBase--;
                    }
                }
            }
            *pBase |= uMask;
            pBase += wBytesPerRow;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegFont *PegScreen::MakeFont(UCHAR uHeight, BOOL bBold,
    BOOL bItalic)
{
    PegFont *pFont = &VectorFont;
    PegPoint Current;
    PegPoint To;
    WORD InVal;
    Current.x = Current.y = 0;
    WORD wTotWidth = 0;
    WORD wBytesPerRow;
    LONG iVector;
    UCHAR *pWidths = (UCHAR *) pFont->pNext;

    PegFont *pNewFont = new PegFont;
    pNewFont->uType = 1;

    if (bBold)
    {
        uHeight++;
    }

    // fill in Ascent, Descent, Height, FirstChar, LastChar:

    iVector = pFont->uAscent * uHeight;
    iVector /= pFont->uHeight;
    pNewFont->uAscent = (UCHAR) iVector;
    iVector = pFont->uDescent * uHeight;
    iVector /= pFont->uHeight;
    pNewFont->uDescent = (UCHAR) iVector;
    pNewFont->uHeight = uHeight;
    pNewFont->wFirstChar = pFont->wFirstChar;
    pNewFont->wLastChar = pFont->wLastChar;

    // create the "bit offset" array:

    WORD *pOffsets = new WORD[pFont->wLastChar - pFont->wFirstChar + 1];
    pNewFont->pOffsets = pOffsets;
    pNewFont->pNext = NULL;

    WORD *pPutOffset = pOffsets;
    SIGNED iScale = uHeight  * 2 / pFont->uHeight / 3;

    if (bBold && bItalic)
    {
        iScale++;
    }

    for (InVal = pFont->wFirstChar; InVal <= pFont->wLastChar; InVal++)
    {
        *pPutOffset++ = wTotWidth;
        iVector = pWidths[InVal - pFont->wFirstChar];
        iVector *= uHeight;
        iVector /= pFont->uHeight;

        if (bItalic && iVector)
        {
            iVector += iScale;
        }
        wTotWidth += (SIGNED) iVector;

    }

    if (bBold && bItalic)
    {
        iScale--;
    }
    wTotWidth += 7;
    wBytesPerRow = wTotWidth >> 3;  // divide by 8 = byte width
    pNewFont->wBytesPerLine = wBytesPerRow;

    // create a memory block for character bitmap:

    UCHAR *pMem = new UCHAR[uHeight * wBytesPerRow];
    memset(pMem, 0, uHeight * wBytesPerRow);
    pNewFont->pData = pMem;
    wTotWidth = 0;
    Current.x = 0;
    Current.y = pNewFont->uAscent;

    for (InVal = pFont->wFirstChar; InVal <= pFont->wLastChar; InVal++)
    {
        if (!pWidths[InVal - pFont->wFirstChar])
        {
            continue;
        }
        signed char *pGet = (signed char *) pFont->pData;
        pGet += pFont->pOffsets[InVal - pFont->wFirstChar];

        UCHAR Opcode = (UCHAR) *pGet++;

        while(Opcode)
        {
            if (Opcode == 3)
            {
                iVector = (LONG) *pGet++ * uHeight;
                iVector /= pFont->uHeight;
                To.x = (SIGNED) iVector;
                iVector = *pGet++ * (SIGNED) uHeight;
                iVector /= (SIGNED) pFont->uHeight;

                if (bItalic)
                {
                    To.x += (iVector / 4) + iScale;
                }
                To.y = pNewFont->uAscent - (SIGNED) iVector;

                if (To.y < 0)
                {
                    To.y = 0;
                }
                if (To.y >= uHeight)
                {
                    To.y = uHeight - 1;
                }
                FontLine(pMem, wTotWidth, Current, To, wBytesPerRow);

                if (bBold)
                {
                    PegPoint p1, p2;
                    p1.x = Current.x;
                    p1.y = Current.y - 1;
                    p2.x = To.x;
                    p2.y = To.y - 1;

                    if (p1.y < 0)
                    {
                        p1.y = 0;
                    }
                    if (p2.y < 0)
                    {
                        p2.y = 0;
                    }

                    FontLine(pMem, wTotWidth + 1, Current, To, wBytesPerRow);
                    FontLine(pMem, wTotWidth, p1, p2, wBytesPerRow);
                }

                Current = To;
            }
            else
            {
                if (Opcode == 2)
                {
                    iVector = (LONG) *pGet++ * uHeight;
                    iVector /= pFont->uHeight;
                    Current.x = (SIGNED) iVector;
                    iVector = *pGet++ * (SIGNED) uHeight;
                    iVector /= (SIGNED) pFont->uHeight;
                    Current.y = pNewFont->uAscent - (SIGNED) iVector;

                    if (bItalic)
                    {
                        Current.x += iVector / 4 + iScale;
                    }

                    if (Current.y < 0)
                    {
                        Current.y = 0;
                    }
                    if (Current.y >= uHeight)
                    {
                        Current.y = uHeight - 1;
                    }
                }
            }
            Opcode = (UCHAR) *pGet++;
        }
        iVector = pWidths[InVal - pFont->wFirstChar];
        iVector *= uHeight;
        iVector /= pFont->uHeight;

        if (bItalic)
        {
            iVector += iScale;
            if (bBold)
            {
                iVector++;
            }
        }
        wTotWidth += (SIGNED) iVector;
    }
    return pNewFont;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::DeleteFont(PegFont *pFont)
{
    delete pFont->pData;
    delete pFont->pOffsets;
    delete pFont;
}

#endif  // PEG_VECTOR_FONTS if


#ifdef __BORLANDC__

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegScreen::InvalidOverlap(PegRect &Rect)
{
    if (miInvalidCount)
    {
        if (Rect.Overlap(mInvalid))
        {
            return TRUE;
        }
    }
    return FALSE;
}

#endif

#ifdef PEG_IMAGE_SCALING

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegBitmap *PegScreen::ResizeImage(PegBitmap *pSrc, SIGNED iWidth, SIGNED iHeight)
{
    if (!iWidth || !iHeight)
    {
        return NULL;
    }

    PegBitmap *pTarget = CreateBitmap(iWidth, iHeight);

    if (!pTarget)
    {
        return pTarget;
    }

    RectStretch(0, 0, pSrc->wWidth - 1, pSrc->wHeight - 1,
        0, 0, iWidth - 1, iHeight - 1, pSrc, pTarget);

    return pTarget;
}

/*--------------------------------------------------------------------------*/
/*
	xs1,ys1 - first point of source rectangle
	xs2,ys2 - second point of source rectangle
	xd1,yd1 - first point of destination rectangle
	xd2,yd2 - second point of destination rectangle
*/

/*--------------------------------------------------------------------------*/
void PegScreen::RectStretch(SIGNED xs1, SIGNED ys1, SIGNED xs2, SIGNED ys2,
    SIGNED xd1, SIGNED yd1, SIGNED xd2, SIGNED yd2,
    PegBitmap *pSrc, PegBitmap *pDest)
{
	SIGNED dx,dy,e,d,dx2;
    dx = yd2 - yd1;
    dy = ys2 - ys1;
	e = (dy << 1) - dx;
	dx2 = dx << 1;
	dy <<= 1;
	for(d = 0; d <= dx; d++)
	{
		Stretch(xd1,xd2,xs1,xs2,ys1,yd1, pSrc, pDest);
		while(e >= 0)
		{
            ys1++;
			e -= dx2;
		}
        yd1++;
		e += dy;
	}
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegScreen::Stretch(SIGNED x1, SIGNED x2, SIGNED y1, SIGNED y2,
    SIGNED yr, SIGNED yw, PegBitmap *pSrc, PegBitmap *pDest)
{
	SIGNED dx,dy,e,d,dx2;
    COLORVAL color;
    dx = x2 - x1;
    dy = y2 - y1;
	e = (dy << 1) - dx;
	dx2 = dx << 1;
	dy <<= 1;
	for (d = 0; d <= dx; d++)
	{
		color = GetBitmapPixel(y1, yr, pSrc); 
		PutBitmapPixel(x1, yw, pDest, color);
		while(e >= 0)
		{
            y1++;
			e -= dx2;
		}
        x1++;
		e += dy;
	}
}



#endif

/*--------------------------------------------------------------------------*/
// The following functions are used for test and debug only. They provide
// the capability of writing any portion of the PEG screen to a Microsoft
// Windows Bitmap file.
//
/*--------------------------------------------------------------------------*/

#ifdef PEG_BITMAP_WRITER


/*--------------------------------------------------------------------------*/
typedef struct
{	
	UCHAR	Blue;  
	UCHAR	Green;
	UCHAR	Red;
	UCHAR	Pad;
} WIN_BMP_PAL_ENTRY;


#define BMP_WORD(a)  WriteBitmapWord(&pWrite, a)
#define BMP_DWORD(a) WriteBitmapDword(&pWrite, a)

#define SWAP_BMP_VALS       // turn on for little-endian systems

/*--------------------------------------------------------------------------*/
void WriteBitmapWord(UCHAR **pWrite, WORD wVal)
{
    UCHAR *pPut = *pWrite;
    #ifdef SWAP_BMP_VALS
    *pPut++ = (UCHAR) wVal;
    *pPut++ = (UCHAR) (wVal >> 8);
    #else
    *pPut++ = (UCHAR) (wVal >> 8);
    *pPut++ = (UCHAR) wVal;
    #endif
    *pWrite = pPut;
}

/*--------------------------------------------------------------------------*/
void WriteBitmapDword(UCHAR **pWrite, DWORD dVal)
{
    UCHAR *pPut = *pWrite;
    #ifdef SWAP_BMP_VALS
    *pPut++ = (UCHAR) dVal;
    *pPut++ = (UCHAR) (dVal >> 8);
    *pPut++ = (UCHAR) (dVal >> 16);
    *pPut++ = (UCHAR) (dVal >> 24);
    #else
    *pPut++ = (UCHAR) (dVal >> 24);
    *pPut++ = (UCHAR) (dVal >> 16);
    *pPut++ = (UCHAR) (dVal >> 8);
    *pPut++ = (UCHAR) dVal;
    #endif
    *pWrite = pPut;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR *PegScreen::DumpWindowsBitmap(DWORD *pPutSize, PegRect &View)
{
    PegBitmap *pMap;
    UCHAR *pReturnMap;
    UCHAR *pWrite;
    UCHAR *pGet;
    DWORD dSize;
    DWORD dDataOffset;
    WORD wActualBytes;
    WORD wBytesPerLine;
    WORD wPaddingBytes;
    WORD wPalEntries;
    int wLoop;
    int wLoop1;

    *pPutSize = 0;      // default to failure

    // first, capture the area they want to make into a bitmap:

    PegCapture BlockCapture;
    Capture(&BlockCapture, View);

    pMap = BlockCapture.Bitmap();

    switch(pMap->uBitsPix)
    {
    case 1:
		wBytesPerLine = (WORD) (pMap->wWidth + 7) / 8;
        wPalEntries = 2;
        break;

    case 2:     // treat 2bpp as 4bpp, because 2bpp not supported
    case 4:
		wBytesPerLine = (WORD) (pMap->wWidth + 1) / 2;
        wPalEntries = 16;
        break;

    case 8:
		wBytesPerLine = (WORD) pMap->wWidth;
        wPalEntries = 256;
        break;

    default:
        return NULL;        // no other depths supported
    }

    wActualBytes = wBytesPerLine;
	wActualBytes += 3;
	wActualBytes &= ~3;
	wPaddingBytes = wActualBytes - wBytesPerLine;

    dDataOffset = 14 +         // the file header size in bytes
                  40 +         // the bitmap header size in bytes
                  (wPalEntries * 4);
                  
    dSize = dDataOffset + (wActualBytes * pMap->wHeight);


    // allocate space for the bitmap in memory:

    pReturnMap = new UCHAR[dSize];
    pWrite = pReturnMap;

    if (!pReturnMap)
    {
        return NULL;
    }

    *pPutSize = dSize;

    // fill in the file header:

    BMP_WORD(0x4d42);
    BMP_DWORD(dSize);
    BMP_DWORD(0);
    BMP_DWORD(dDataOffset);

    // fill in the bitmap header:

    BMP_DWORD(40);              // size of header
    BMP_DWORD(pMap->wWidth);
    BMP_DWORD(pMap->wHeight);
    BMP_WORD(1);                // color planes

    if (pMap->uBitsPix != 2)
    {
        BMP_WORD(pMap->uBitsPix);
    }
    else
    {
        BMP_WORD(4);            // 2bpp not supported, use 4-bpp
    }
    BMP_DWORD(0);               // no compression
    BMP_DWORD(dSize - dDataOffset);
    BMP_DWORD(0);               // horz res
    BMP_DWORD(0);               // vert res
    BMP_DWORD(wPalEntries);
    BMP_DWORD(wPalEntries);

    // write the palette entries:

#define FORCE_DJ_MONO_PALETTE

#ifdef FORCE_DJ_MONO_PALETTE

	*pWrite++ = 0;  
	*pWrite++ = 0xFF;
	*pWrite++ = 0x66;
	*pWrite++ = 0;


	*pWrite++ = 0x33;
	*pWrite++ = 0x33;
	*pWrite++ = 0x33;
	*pWrite++ = 0;

#else
        
      
    pGet = GetPalette(&dDataOffset);

    for (wLoop = 0; wLoop < wPalEntries; wLoop++)
    {
        if (pMap->uBitsPix != 2 || wLoop < 4)
        {
            #ifdef SWAP_BMP_VALS
            *pWrite++ = *(pGet + 2);
            *pWrite++ = *(pGet + 1);
            *pWrite++ = *pGet;
            *pWrite++ = 0;
            pGet += 3;
            #else
            *pWrite++ = *pGet++;
            *pWrite++ = *pGet++;
            *pWrite++ = *pGet++;
            *pWrite++ = 0;
            #endif
        }
        else
        {
            // for 2bpp, last 12 entries are just all 0
            BMP_DWORD(0);
        }
    }

#endif

    // OK, now we can write the bitmap data!!


    for (wLoop = pMap->wHeight - 1; wLoop >= 0; wLoop--)
    {
        pGet = pMap->pStart;
        pGet += wLoop * wBytesPerLine;

        for (wLoop1 = 0; wLoop1 < wBytesPerLine; wLoop1++)
        {
            if (pMap->uBitsPix == 2)       
            {
                UCHAR uTemp = *pGet++;
                *pWrite = (uTemp >> 2) & 0x30;
                *pWrite++ |= (uTemp >> 4) & 3;
                *pWrite = (uTemp << 2) & 0x30;
                *pWrite++ |= uTemp & 3;
                wLoop1++;
            }
            else
            {
                *pWrite++ = *pGet++;
            }
        }
        for (wLoop1 = 0; wLoop1 < wPaddingBytes; wLoop1++)
        {
            *pWrite++ = 0;
        }
    }

    // return pointer to Windows bitmap:

    return pReturnMap;
}
#endif


/*--------------------------------------------------------------------------*/
// PegCapture function implementations
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
void PegCapture::Realloc(LONG lSize)
{
    if (lSize > mlDataSize)
    {
        if (mBitmap.pStart)
        {
            delete mBitmap.pStart;
            mBitmap.pStart = NULL;
        }
        mBitmap.pStart = new UCHAR[lSize];
        mlDataSize = lSize;
    }
}

/*--------------------------------------------------------------------------*/
void PegCapture::MoveTo(SIGNED iLeft, SIGNED iTop)
{
    mRect.MoveTo(iLeft, iTop);
}


/*--------------------------------------------------------------------------*/
PegPoint PegCapture::Point(void)
{
    PegPoint Where;
    Where.x = mRect.wLeft;
    Where.y = mRect.wTop;
    return(Where);
}








