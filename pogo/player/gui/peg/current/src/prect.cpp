/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// prect.cpp - Basic rectangle definition.
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

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegRect::Contains(PegPoint Test) const
{
    if (Test.x >= wLeft &&
        Test.x <= wRight &&
        Test.y >= wTop &&
        Test.y <= wBottom)
    {
        return TRUE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegRect::Contains(SIGNED Testx, SIGNED Testy) const
{
    if (Testx >= wLeft &&
        Testx <= wRight &&
        Testy >= wTop &&
        Testy <= wBottom)
    {
        return TRUE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegRect::Contains(const PegRect &That) const
{
    if (That.wLeft >= wLeft &&
        That.wRight <= wRight &&
        That.wTop >= wTop &&
        That.wBottom <= wBottom)
    {
        return TRUE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegRect::Overlap(const PegRect &That) const
{
    if (That.wLeft <= wRight &&
        That.wTop <= wBottom &&
        That.wBottom >= wTop &&
        That.wRight >= wLeft)
    {
        return TRUE;
    }
    return FALSE;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegRect::MoveTo(SIGNED x, SIGNED y)
{
    SIGNED xShift = x - wLeft;
    SIGNED yShift = y - wTop;
    Shift(xShift, yShift);

}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegRect::Shift(SIGNED xShift, SIGNED yShift)
{
    wLeft += xShift;
    wRight += xShift;
    wTop += yShift;
    wBottom += yShift;
}

/*--------------------------------------------------------------------------*/
// PegRect operators
PegRect PegRect::operator &= (const PegRect &Rect)
{
    if (Rect.wLeft > wLeft)
    {
        wLeft = Rect.wLeft;
    }
    if (Rect.wRight < wRight)
    {
        wRight = Rect.wRight;
    }
    if (Rect.wTop > wTop)
    {
        wTop = Rect.wTop;
    }
    if (Rect.wBottom < wBottom)
    {
        wBottom = Rect.wBottom;
    }
    return *this;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator |= (const PegRect &Rect)
{
    if (wLeft > Rect.wLeft)
    {
        wLeft = Rect.wLeft;
    }

    if (wRight < Rect.wRight)
    {
        wRight = Rect.wRight;
    }
    if (wTop > Rect.wTop)
    {
        wTop = Rect.wTop;
    }
    if (wBottom < Rect.wBottom)
    {
        wBottom = Rect.wBottom;
    }
    return *this;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator &(const PegRect &Rect) const
{
    PegRect NewRect = *this;
    
    if (NewRect.wRight > Rect.wRight)
    {
        NewRect.wRight = Rect.wRight;
    }
    if (NewRect.wTop < Rect.wTop)
    {
        NewRect.wTop = Rect.wTop;
    }
    if (NewRect.wBottom > Rect.wBottom)
    {
        NewRect.wBottom = Rect.wBottom;
    }
    if (NewRect.wLeft < Rect.wLeft)
    {
        NewRect.wLeft = Rect.wLeft;
    }
    return NewRect;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator ^=(const PegRect &Rect)
{
    if (wRight < Rect.wLeft ||
        wLeft > Rect.wRight ||
        wTop > Rect.wBottom ||
        wBottom < Rect.wTop)
    {
        return *this;        // no overlap
    }

    LONG vArea;
    LONG hArea;

    // clip it to the biggest regular rectangle:

    if (wLeft >= Rect.wLeft &&
        wRight <= Rect.wRight)
    {
        if (wTop < Rect.wTop)
        {
            wBottom = Rect.wTop - 1;        // trip the bottom
        }
        else
        {
            if (wBottom > Rect.wBottom)
            {
                wTop = Rect.wBottom + 1;    // trim the top
            }
            else
            {
                wTop = wBottom + 1;            // return negative rect.
            }
        }
        return *this;
    }

    if (wTop >= Rect.wTop &&
        wBottom <= wBottom)
    {
        if (wLeft < Rect.wLeft)
        {
            wRight = Rect.wLeft - 1;        // trim the right
        }
        else
        {
            if (wRight > Rect.wRight)
            {
                wLeft = Rect.wRight + 1;    // trim the left
            }
            else
            {
                wLeft = wRight + 1;            // return negative rect.
            }
        }
        return *this;
    }

    if (Contains(Rect.wLeft, Rect.wTop))    // contains top left corner?
    {
        vArea = Width() * (Rect.wTop - wTop);
        hArea = Height() * (Rect.wLeft - wLeft);

        if (vArea > hArea)
        {
            wBottom    = Rect.wTop - 1;
        }
        else
        {
            wRight = Rect.wLeft - 1;
        }
        return *this;
    }

    if (Contains(Rect.wRight, Rect.wTop))    // contains top right corner?
    {
        vArea = Width() * (Rect.wTop - wTop);
        hArea = Height() * (wRight - Rect.wRight);

        if (vArea > hArea)
        {
            wBottom    = Rect.wTop - 1;
        }
        else
        {
            wLeft = Rect.wRight + 1;
        }
        return *this;
    }

    if (Contains(Rect.wLeft, Rect.wBottom))    // contains bottom left corner?
    {
        vArea = Width() * (wBottom - Rect.wBottom);
        hArea = Height() * (Rect.wLeft - wLeft);

        if (vArea > hArea)
        {
            wTop = Rect.wBottom + 1;
        }
        else
        {
            wRight = Rect.wLeft - 1;
        }
        return *this;
    }

    // contains bottom right corner:

    vArea = Width() * (wBottom - Rect.wBottom);
    hArea = Height() * (wRight - Rect.wRight);

    if (vArea > hArea)
    {
        wTop = Rect.wBottom + 1;
    }
    else
    {
        wLeft = Rect.wRight + 1;
    }
    return *this;
}


/*--------------------------------------------------------------------------*/
PegRect PegRect::operator +(const PegPoint &Point) const
{
    PegRect NewRect;
    NewRect.wLeft = wLeft + Point.x;
    NewRect.wRight = wRight + Point.x;
    NewRect.wTop = wTop + Point.y;
    NewRect.wBottom = wBottom + Point.y;
    return NewRect;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator++(int)
{
    wLeft--;
    wRight++;
    wTop--;
    wBottom++;
    return *this;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator+= (int Val)
{
    wLeft -= Val;
    wRight += Val;
    wTop -= Val;
    wBottom += Val;
    return *this;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator--(int)
{
    wLeft++;
    wRight--;
    wTop++;
    wBottom--;

    if (wLeft > wRight)
    {
        wRight = wLeft;
    }
    if (wBottom < wTop)
    {
        wBottom = wTop;
    }
    return *this;
}

/*--------------------------------------------------------------------------*/
PegRect PegRect::operator-=(int Val)
{
    wLeft += Val;
    wRight -= Val;
    wTop += Val;
    wBottom -= Val;
    if (wLeft > wRight)
    {
        wRight = wLeft;
    }
    if (wBottom < wTop)
    {
        wBottom = wTop;
    }
    return *this;
}

/*--------------------------------------------------------------------------*/
BOOL PegRect::operator != (const PegRect &Rect) const
{
    if (Rect.wTop != wTop ||
        Rect.wBottom != wBottom ||
        Rect.wLeft != wLeft ||
        Rect.wRight != wRight)
    {
        return TRUE;
    }
    return FALSE;
}


/*--------------------------------------------------------------------------*/
BOOL PegRect::operator == (const PegRect &Rect) const
{
    if (Rect.wTop == wTop &&
        Rect.wBottom == wBottom &&
        Rect.wLeft == wLeft &&
        Rect.wRight == wRight)
    {
        return TRUE;
    }
    return FALSE;
}







