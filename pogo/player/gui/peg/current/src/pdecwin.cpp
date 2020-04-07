/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pdecwin.cpp - Base window class definition.
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
// This is the basic window class. It supports the addition of decorations
// such as a border, title, scroll bar, status bar, and menu, in addition
// to any other derived PegThings.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gui/peg/peg.hpp>

/*--------------------------------------------------------------------------*/
PegDecoratedWindow::PegDecoratedWindow(const PegRect &Rect, WORD wFlags) :
    PegWindow(Rect, wFlags)
{
    Type(TYPE_DECORATED_WIN);
}

/*--------------------------------------------------------------------------*/
PegDecoratedWindow::PegDecoratedWindow(WORD wStyle) :
    PegWindow(wStyle)
{
    Type(TYPE_DECORATED_WIN);
}

/*--------------------------------------------------------------------------*/
// Destructor- Remove and delete any children.
/*--------------------------------------------------------------------------*/
PegDecoratedWindow::~PegDecoratedWindow()
{

}

/*--------------------------------------------------------------------------*/
SIGNED PegDecoratedWindow::Message(const PegMessage &Mesg)
{
   #ifdef PEG_KEYBOARD_SUPPORT
    PegMessage NewMessage;
   #endif

    switch(Mesg.wType)
    {
    case PM_CURRENT:
        PegWindow::Message(Mesg);
        break;

    case PM_NONCURRENT:
        PegWindow::Message(Mesg);
        break;

   #ifdef PEG_KEYBOARD_SUPPORT
    case PM_KEY_RELEASE:
            return (PegWindow::Message(Mesg));
        break;

   #endif

    default:
        return(PegWindow::Message(Mesg));
    }
    return (0);
}

/*--------------------------------------------------------------------------*/
void PegDecoratedWindow::Add(PegThing *What, BOOL bDraw)
{
    PegWindow::Add(What, bDraw);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegThing *PegDecoratedWindow::Remove(PegThing *What, BOOL bDraw)
{
    return(PegWindow::Remove(What, bDraw));
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegDecoratedWindow::InitClient(void)
{
    PegWindow::InitClient();
}


/*--------------------------------------------------------------------------*/

// End of file
