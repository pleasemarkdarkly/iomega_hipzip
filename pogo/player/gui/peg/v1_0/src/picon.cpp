/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// picon.cpp - PegIcon and PegWindowIcon class definitions.
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

#include <gui/peg/peg.hpp>
extern PegBitmap gbWinIconBitmap;

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegIcon::PegIcon(PegThing *proxy, PegBitmap *pbitmap, WORD wId, WORD wStyle) :
    PegThing(wId, wStyle)
{
    mpBitmap = pbitmap;
    Type(TYPE_ICON);
    if (!mpBitmap)
    {
        mpBitmap = &gbWinIconBitmap;
    }
    mpProxy = proxy;

    mReal.wLeft = mReal.wTop = 0;
    mReal.wRight = mReal.wLeft + mpBitmap->wWidth - 1;
    mReal.wBottom = mReal.wTop + mpBitmap->wHeight - 1;
    InitClient();
    muColors[PCI_NORMAL] = PCLR_BUTTON_FACE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegIcon::PegIcon(const PegRect &Where, PegBitmap *pbitmap, WORD wId, WORD wStyle) :
    PegThing(Where, wId, wStyle)
{
    mpBitmap = pbitmap;
    Type(TYPE_ICON);
    if (!mpBitmap)
    {
        mpBitmap = &gbWinIconBitmap;
    }
    mpProxy = NULL;
    InitClient();
    muColors[PCI_NORMAL] = PCLR_BUTTON_FACE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegIcon::PegIcon(PegBitmap *pbitmap, WORD wId, WORD wStyle) :
    PegThing(wId, wStyle)
{
    mpBitmap = pbitmap;
    Type(TYPE_ICON);
    if (!mpBitmap)
    {
        mpBitmap = &gbWinIconBitmap;
    }
    mpProxy = NULL;
    mReal.wLeft = mReal.wTop = 0;
    mReal.wRight = mReal.wLeft + mpBitmap->wWidth - 1;
    mReal.wBottom = mReal.wTop + mpBitmap->wHeight - 1;
    InitClient();
    muColors[PCI_NORMAL] = PCLR_BUTTON_FACE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegIcon::Draw(void)
{
    BeginDraw();

    if (mpBitmap)
    {
        PegPoint Put;
        Put.x = mReal.wLeft;
        Put.y = mReal.wTop;
        Bitmap(Put, mpBitmap);
    }
    else
    {
        PegColor Color(muColors[PCI_NORMAL], BLACK, CF_NONE);
        Rectangle(mReal, Color, 2);
    }

    EndDraw();
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
SIGNED PegIcon::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_LBUTTONUP:
        if (mpProxy)
        {
            Parent()->Add(mpProxy);
            Presentation()->NullInput(this);
            Destroy(this);
        }
        break;

    default:
        PegThing::Message(Mesg);
    }
    return 0;    
}


