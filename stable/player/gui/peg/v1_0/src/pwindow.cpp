/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pwindow.cpp - Base window class definition.
//
// Author: Kenneth G. Maxwell
//
// Copyright (c) 1997-1998 Swell Software 
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
PegWindow::PegWindow(const PegRect &Rect, WORD wStyle) :
    PegThing(Rect, 0, wStyle),
    mpIconMap(NULL),
    mbModal(FALSE),
    mbMaximized(FALSE),
    mbMoveFrame(FALSE),
    muScrollMode(0),
    muMoveMode(0),
    mbShowPointer(PPT_NORMAL)
{
    Type(TYPE_WINDOW);
    muColors[PCI_NORMAL] = PCLR_CLIENT;
    AddStatus(PSF_VIEWPORT|PSF_TAB_STOP);

    if (wStyle & FF_THICK)
    {
        AddStatus(PSF_SIZEABLE|PSF_MOVEABLE);
    }
    InitClient();
}

/*--------------------------------------------------------------------------*/
PegWindow::PegWindow(WORD wStyle) :
    PegThing(0, wStyle),
    mpIconMap(NULL),
    mbModal(FALSE),
    mbMaximized(FALSE),
    mbMoveFrame(FALSE),
    muScrollMode(0),
    muMoveMode(0),
    mbShowPointer(PPT_NORMAL)
{
    mReal.Set(0, 0, 20, 20);
    mClient = mReal;
    Type(TYPE_WINDOW);
    muColors[PCI_NORMAL] = PCLR_CLIENT;
    AddStatus(PSF_VIEWPORT|PSF_TAB_STOP);

    if (wStyle & FF_THICK)
    {
        AddStatus(PSF_SIZEABLE|PSF_MOVEABLE);
    }
    InitClient();
}

/*--------------------------------------------------------------------------*/
// Destructor- Remove and delete any children.
/*--------------------------------------------------------------------------*/
PegWindow::~PegWindow()
{

}

/*--------------------------------------------------------------------------*/
SIGNED PegWindow::Message(const PegMessage &Mesg)
{
    SIGNED Result = 0;
    PegMessage NewMessage;

    switch(Mesg.wType)
    {
    case PM_POINTER_MOVE:
        if (muMoveMode)
        {
            if (Mesg.Point.x != mMovePoint.x ||
                Mesg.Point.y != mMovePoint.y)
            {
                DrawMoveFrame(TRUE);
                mMovePoint = Mesg.Point;
                DrawMoveFrame(FALSE);
            }
        }
        else
        {
            if (StatusIs(PSF_SIZEABLE) && !mbMaximized)
            {
                UCHAR uTemp = BorderContains(Mesg.Point);

                if (uTemp != mbShowPointer)
                {
                    mbShowPointer = uTemp;

                    switch(uTemp)
                    {
                    case PMM_MOVELEFT:
                    case PMM_MOVERIGHT:
                        SetPointerType(PPT_HSIZE);
                        break;

                    case PMM_MOVETOP:
                    case PMM_MOVEBOTTOM:
                        SetPointerType(PPT_VSIZE);
                        break;

                    case PMM_MOVEUR:
                    case PMM_MOVELL:
                        SetPointerType(PPT_NESW_SIZE);
                        break;

                    case PMM_MOVEUL:
                    case PMM_MOVELR:
                        SetPointerType(PPT_NWSE_SIZE);
                        break;

                    default:
                        SetPointerType(PPT_NORMAL);
                        break;
                    }
                }
            }
        }
        break;

    case PM_SHOW:
        PegThing::Message(Mesg);

        if (muScrollMode & WSM_AUTOSCROLL)
        {
            CheckAutoScroll();
        }
        break;

    case PM_CURRENT:
        PegThing::Message(Mesg);
        if (Presentation()->GetCurrentThing() == this &&
            !StatusIs(PSF_KEEPS_CHILD_FOCUS))
        {
            MoveFocusToFirstClientChild();
        }
        break;

    case PM_ADDICON:
        AddIcon((PegIcon *) Mesg.pData);
        break;

    case PM_BEGIN_MOVE:
        if (!muMoveMode && StatusIs(PSF_MOVEABLE))
        {
            muMoveMode = (UCHAR) Mesg.iData;
            mbMoveFrame = FALSE;
            CapturePointer();
            mStartMove = Mesg.Point;
            mMovePoint = mStartMove;

            if (muMoveMode == PMM_MOVELR)
            {
                SetPointerType(PPT_NWSE_SIZE);
                mbShowPointer = TRUE;
            }
        }
        break;

    case PM_POINTER_EXIT:
        if (mbShowPointer)
        {
            SetPointerType(PPT_NORMAL);
            mbShowPointer = 0;
        }
        PegThing::Message(Mesg);
        break;

    case PM_LBUTTONDOWN:
        if (!muMoveMode && StatusIs(PSF_SIZEABLE) && !mbMaximized)
        {
            muMoveMode = BorderContains(Mesg.Point);

            if (muMoveMode)
            {
                mbMoveFrame = FALSE;
                CapturePointer();
                mStartMove = Mesg.Point;
                mMovePoint = mStartMove;
            }
        }
        break;

    case PM_LBUTTONUP:
        if (muMoveMode)
        {
            DrawMoveFrame(TRUE);
            SetPointerType(PPT_NORMAL);
            ReleasePointer();
            PegRect OldSize = mReal;

            if (mStartMove.x == mMovePoint.x &&
                mStartMove.y == mMovePoint.y)
            {
                muMoveMode = 0;
                break;  // didn't really move or size anything
            }

            PegRect NewSize = mReal;
            int xShift = mMovePoint.x - mStartMove.x;
            int yShift = mMovePoint.y - mStartMove.y;

            switch(muMoveMode)
            {
            case PMM_MOVEALL:
                NewSize.Shift(xShift, yShift);
                break;

            case PMM_MOVERIGHT:
                NewSize.wRight += xShift;
                break;

            case PMM_MOVELEFT:
                NewSize.wLeft += xShift;
                break;

            case PMM_MOVETOP:
                NewSize.wTop += yShift;
                break;

            case PMM_MOVEBOTTOM:
                NewSize.wBottom += yShift;
                break;

            case PMM_MOVEUL:
                NewSize.wLeft += xShift;
                NewSize.wTop += yShift;
                break;

            case PMM_MOVELL:
                NewSize.wLeft += xShift;
                NewSize.wBottom += yShift;
                break;

            case PMM_MOVEUR:
                NewSize.wRight += xShift;
                NewSize.wTop += yShift;
                break;

            case PMM_MOVELR:
                NewSize.wRight += xShift;
                NewSize.wBottom += yShift;
                break;
            }
            CheckResizeRect(NewSize);

            NewMessage.wType = PM_SIZE;
            NewMessage.Rect = NewSize;
            Message(NewMessage);

            if (StatusIs(PSF_VISIBLE))
            {
                if (mReal.Contains(OldSize))
                {
                    Draw();
                }
                else
                {
                    Parent()->Draw();
                }
            }
            muMoveMode = 0;
        }
        break;

    case PM_VSCROLL:
        MoveClientObjects(0, Mesg.iData - (SIGNED) Mesg.lData);
        break;

    case PM_HSCROLL:
        MoveClientObjects(Mesg.iData - (SIGNED) Mesg.lData, 0);
        break;

    case PM_MAXIMIZE:
        if (StatusIs(PSF_SIZEABLE))
        {
            mbMaximized = TRUE;
            mRestore = mReal;
            NewMessage.wType = PM_SIZE;
            NewMessage.Rect = Parent()->mClient;
            Message(NewMessage);
            Draw();
        }
        break;

    case PM_MINIMIZE:
        if (StatusIs(PSF_SIZEABLE) && !IsModal())
        {
            NewMessage.wType = PM_ADDICON;
            PegIcon *pIcon = new PegIcon(this, mpIconMap);
            pIcon->Id(Id());
            NewMessage.pData = pIcon;
            NewMessage.pTarget = Parent();
            MessageQueue()->Push(NewMessage);
            Parent()->Remove(this);
        }
        break;

    case PM_RESTORE:
        if (StatusIs(PSF_SIZEABLE))
        {
            if (mbMaximized)
            {
                mbMaximized = FALSE;
                NewMessage.wType = PM_SIZE;
                NewMessage.Rect = mRestore;
                Message(NewMessage);
                Parent()->Draw();
            }
        }
        break;

   #ifdef PEG_KEYBOARD_SUPPORT
    case PM_KEY:
        if (Parent() == Presentation() &&
            Mesg.iData == PK_F4 &&
            (Mesg.lData & KF_CTRL))
        {
            NewMessage.wType = PM_CLOSE;
            NewMessage.pTarget = this;
            MessageQueue()->Push(NewMessage);
        }
        else
        {
            return PegThing::Message(Mesg);
        }
        break;
   #endif

    case PM_CLOSE:
        Destroy(this);
        return(IDB_CLOSE);

    case PEG_SIGNAL(IDB_CLOSE, PSF_CLICKED):
    case PEG_SIGNAL(IDB_OK, PSF_CLICKED):
    case PEG_SIGNAL(IDB_CANCEL, PSF_CLICKED):
    case PEG_SIGNAL(IDB_YES, PSF_CLICKED):
    case PEG_SIGNAL(IDB_NO, PSF_CLICKED):
    case PEG_SIGNAL(IDB_RETRY, PSF_CLICKED):
    case PEG_SIGNAL(IDB_ABORT, PSF_CLICKED):
    case PEG_SIGNAL(IDB_APPLY, PSF_CLICKED):
        Destroy(this);
        return(Mesg.iData);

    default:
        return (PegThing::Message(Mesg));
    }
    return (Result);
}


/*--------------------------------------------------------------------------*/
SIGNED PegWindow::Execute(void)
{
    PegMessage pSend;
    mbModal = TRUE;
    SIGNED iReturn;

   #ifdef PEG_MULTITHREAD

    if (CURRENT_TASK != PEG_TASK_PTR)
    {
        Presentation()->BeginSubTaskExecute(this);
    }

   #endif

    if (!StatusIs(PSF_VISIBLE))
    {
        Presentation()->Add(this);
    }

    while(1)
    {
        MessageQueue()->Pop(&pSend);

        switch (pSend.wType)
        {
        case PM_LBUTTONDOWN:
        case PM_RBUTTONDOWN:
        case PM_LBUTTONUP:
        case PM_RBUTTONUP:
           #ifdef PEG_MULTITHREAD
            if (CURRENT_TASK == PEG_TASK_PTR)
            {
                if (Presentation()->RouteMessageToTask(&pSend))
                {
                    break;
                }
            }
           #endif

            if (mReal.Contains(pSend.Point) ||
                Presentation()->IsPointerCaptured())
            {
                //Presentation()->DispatchMessage(this, &pSend);
                Presentation()->DispatchMessage(Presentation(), &pSend);
            }
            break;

        case PM_POINTER_MOVE:
           #ifdef PEG_MULTITHREAD
            if (CURRENT_TASK == PEG_TASK_PTR)
            {
                if (Presentation()->RouteMessageToTask(&pSend))
                {
                    break;
                }
            }
           #endif
            if (mReal.Contains(pSend.Point) ||
                Presentation()->IsPointerCaptured())
            {
                //Presentation()->DispatchMessage(this, &pSend);
                Presentation()->DispatchMessage(Presentation(), &pSend);
            }
            else
            {
                if (mbShowPointer)
                {
                    SetPointerType(PPT_NORMAL);
                    mbShowPointer = 0;
                }
                Screen()->SetPointer(pSend.Point);
            }
            break;

       #ifdef PEGWIN32
        // When running under Windows, the user might close the development
        // window, without first closing the modal PEG window. There seems to be
        // no way to prevent this. In order to terminate correctly, we push
        // a couple of PM_EXIT messages, to be sure that all modal windows are
        // terminated, and PegPresentationManager returns.

        case PM_EXIT:
			{
				PegMessage NewMessage;
				NewMessage.wType = PM_EXIT;
				NewMessage.pTarget = Presentation();
				NewMessage.pSource = NULL;
				MessageQueue()->Push(NewMessage);
				MessageQueue()->Push(NewMessage);
				return(PM_EXIT);
			}
       #endif

       #ifdef PEG_KEYBOARD_SUPPORT
        case PM_KEY:
            if (pSend.iData == PK_TAB && (pSend.lData & KF_CTRL))
            {
                break;
            }
            // fall throught to the default case:
       #endif
   
        default:

           #ifdef PEG_MULTITHREAD
            if (CURRENT_TASK == PEG_TASK_PTR)
            {
                if (Presentation()->RouteMessageToTask(&pSend))
                {
                    break;
                }
            }
           #endif

            iReturn = Presentation()->DispatchMessage(this, &pSend);
            if (iReturn)
            {
               #ifdef PEG_MULTITHREAD
                Presentation()->EndSubTaskExecute(this);
               #endif
                return(iReturn);
            }
        }
    }
}

#ifdef PEG_MULTITHREAD
/*--------------------------------------------------------------------------*/
SIGNED PegWindow::GlobalModalExecute(void)
{
    PegMessage pSend;
    PegThing *Current;
    mbModal = TRUE;
    SIGNED iReturn = 0;

    if (CURRENT_TASK != PEG_TASK_PTR)
    {
        Presentation()->BeginSubTaskExecute(this);
    }

    CapturePointer();
    Presentation()->Add(this);

    while(1)
    {
        MessageQueue()->Pop(&pSend);

        switch (pSend.wType)
        {
        case PM_LBUTTONDOWN:
        case PM_RBUTTONDOWN:
        case PM_LBUTTONUP:
        case PM_RBUTTONUP:
        case PM_POINTER_MOVE:

           #if defined(PEG_MOUSE_SUPPORT)
            if (pSend.wType == PM_POINTER_MOVE)
            {
                Screen()->SetPointer(pSend.Point);
            }
           #endif

            if (muMoveMode)
            {
                Message(pSend);
                break;
            }

            if (Presentation()->First() == this)
            {
                Current = Presentation()->GetPointerOwner();
                    
                if (Current != this)
                {
                    // One of my children has captured the pointer,
                    // let him have the pointer messages:

                    iReturn = Current->Message(pSend);
                }
                else
                {
                    if (mReal.Contains(pSend.Point))
                    {
                        Current = Presentation()->FindLowestThingContaining(this, pSend.Point);

                        if (Current)
                        {
                            if (Current != Presentation()->LastPointerOver())
                            {
                                if (Presentation()->LastPointerOver())
                                {
                                    Presentation()->LastPointerOver()->Message(PegMessage(PM_POINTER_EXIT));
                                }
                                Presentation()->LastPointerOver(Current);
                                Current->Message(PegMessage(PM_POINTER_ENTER));
                            }

                            if ((pSend.wType == PM_LBUTTONDOWN || pSend.wType == PM_RBUTTONDOWN) &&
                                Current != Presentation()->GetCurrentThing())
                            {
                                if (Current->StatusIs(PSF_ACCEPTS_FOCUS))
                                {
                                    Presentation()->MoveFocusTree(Current);
                                }
                            }
                            iReturn = Current->Message(pSend);
                        }
                    }
                }
            }
            else
            {
                // If we get here, it means another task is modally executing
                // a window, and it is now on top. Try to pass mouse messages
                // to that task:

                if (CURRENT_TASK == PEG_TASK_PTR)
                {
                    Presentation()->RouteMessageToTask(&pSend);
                }
            }
            break;

       #ifdef PEG_KEYBOARD_SUPPORT
        case PM_KEY:
            if (pSend.iData == PK_TAB && (pSend.lData & KF_CTRL))
            {
                break;
            }
            // fall throught to the default case:
       #endif
   
        default:
            iReturn = Presentation()->DispatchMessage(this, &pSend);
            break;
        }
        if (iReturn)
        {
            Presentation()->EndSubTaskExecute(this);
            ReleasePointer();
            return(iReturn);
        }
    }
}
#endif

/*--------------------------------------------------------------------------*/
void PegWindow::Resize(PegRect NewSize)
{
    PegThing *Child;
    PegMessage NewMessage(PM_PARENTSIZED);
    PegThing::Resize(NewSize);

    if (muScrollMode & WSM_AUTOSCROLL)
    {
        if (CheckAutoScroll())
        {
            UpdateChildClipping();
            Screen()->GenerateViewportList(this);
            Invalidate(mReal);
        }
    }

    Child = First();
    while(Child)
    {
        Child->Message(NewMessage);
        Child = Child->Next();
    }
}


/*--------------------------------------------------------------------------*/
void PegWindow::Draw(void)
{
    BeginDraw();
    DrawFrame();
    DrawChildren();

    EndDraw();
}


/*--------------------------------------------------------------------------*/
void PegWindow::InitClient(void)
{
    PegThing::InitClient();
}


/*--------------------------------------------------------------------------*/
void PegWindow::FillScrollCorner(void)
{
}

#ifndef PEG_RUNTIME_COLOR_CHECK

/*--------------------------------------------------------------------------*/
void PegWindow::DrawFrame(BOOL bFill)
{
    PegColor Color(PCLR_BORDER, muColors[PCI_NORMAL], CF_FILL);

    if (bFill)
    {
        switch(mwStyle & FF_MASK)
        {
        case FF_THICK:
            Rectangle(mReal, Color, PEG_FRAME_WIDTH);
            break;

        case FF_THIN:
            Color.uForeground = PCLR_SHADOW;
            Rectangle(mReal, Color, 1);
            break;

        case FF_NONE:
        default:
            Rectangle(mReal, Color, 0);
        }
    }

   #if (PEG_NUM_COLORS >= 4)
    if (mwStyle & (FF_THICK|FF_RAISED))
   #else
    if (mwStyle & FF_THICK)
   #endif
    {
        Color.uForeground = PCLR_HIGHLIGHT;

        // add highlights:

        Line(mReal.wLeft + 1, mReal.wTop + 1,
            mReal.wLeft + 1, mReal.wBottom, Color);
        Line(mReal.wLeft + 2, mReal.wTop + 1, mReal.wRight - 1,
            mReal.wTop + 1, Color);

        Color.uForeground = PCLR_LOWLIGHT;

        // add edge
        Line(mReal.wRight - 1, mReal.wTop + 1, mReal.wRight - 1,
            mReal.wBottom - 1, Color);
        Line(mReal.wLeft + 1, mReal.wBottom - 1, mReal.wRight - 1,
            mReal.wBottom - 1, Color);

        // add shadows:

        Color.uForeground = PCLR_SHADOW;

        Line(mReal.wRight, mReal.wTop, mReal.wRight,
            mReal.wBottom, Color);
        Line(mReal.wLeft, mReal.wBottom, mReal.wRight,
            mReal.wBottom, Color);

    }

   #if (PEG_NUM_COLORS >= 4)

    if (mwStyle & FF_RECESSED)
    {
        Color.uForeground = PCLR_HIGHLIGHT;

        // add highlights:

        Line(mReal.wLeft, mReal.wBottom,
            mReal.wRight, mReal.wBottom, Color);
        Line(mReal.wRight, mReal.wTop, mReal.wRight,
            mReal.wBottom, Color);

        Color.uForeground = PCLR_LOWLIGHT;

        // add edge
        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
            mReal.wTop, Color);
        Line(mReal.wLeft, mReal.wTop + 1, mReal.wLeft,
            mReal.wBottom, Color);

        Color.uForeground = PCLR_SHADOW;

        // add edge
        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wRight - 2,
            mReal.wTop + 1, Color);
        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wLeft + 1,
            mReal.wBottom - 1, Color);

        // add shadows:

        Color.uForeground = PCLR_DIALOG;

        Line(mReal.wLeft + 1, mReal.wBottom - 1,
            mReal.wRight - 1, mReal.wBottom - 1, Color);
        Line(mReal.wRight - 1, mReal.wTop + 1,
            mReal.wRight - 1, mReal.wBottom - 1, Color);
    }
   #endif

    if (mwStyle & FF_THICK)
    {
        Color.uForeground = PCLR_HIGHLIGHT;
        Line(mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wTop + PEG_FRAME_WIDTH - 1,
             mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wBottom - PEG_FRAME_WIDTH + 2, Color);
        
        Line(mReal.wLeft + PEG_FRAME_WIDTH - 1,
             mReal.wBottom - PEG_FRAME_WIDTH + 2,
             mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wBottom - PEG_FRAME_WIDTH + 2, Color);


        Color.uForeground = PCLR_LOWLIGHT;

        Line(mReal.wLeft + PEG_FRAME_WIDTH - 1, mClient.wTop - 1,
            mReal.wRight - PEG_FRAME_WIDTH, mClient.wTop - 1, Color);
        Line(mClient.wLeft - 1, mClient.wTop,
            mClient.wLeft - 1, mReal.wBottom - PEG_FRAME_WIDTH + 1, Color);
    }
}

#else

// here for run-time color depth determination:

/*--------------------------------------------------------------------------*/
void PegWindow::DrawFrame(BOOL bFill)
{
    PegColor Color(PCLR_BORDER, muColors[PCI_NORMAL], CF_FILL);

    DWORD dColors = Screen()->NumColors();

    if (dColors < 4)
    {
        Color.uForeground = PCLR_SHADOW;
    }

    if (bFill)
    {
        switch(mwStyle & FF_MASK)
        {
        case FF_THICK:
            Rectangle(mReal, Color, PEG_FRAME_WIDTH);
            break;

        case FF_RAISED:
        case FF_RECESSED:
            if (dColors >= 4)
            {
                Rectangle(mReal, Color, 0);
                break;
                // else for monochrome, fall through to FF_THIN code:
            }

        case FF_THIN:
            Color.uForeground = PCLR_SHADOW;
            Rectangle(mReal, Color, 1);
            break;


        case FF_NONE:
        default:
            Rectangle(mReal, Color, 0);
        }
    }

    if ((dColors >= 4 && (mwStyle & (FF_THICK|FF_RAISED))) ||
        (dColors < 4 && (mwStyle & FF_THICK)))
    {
        Color.uForeground = PCLR_HIGHLIGHT;

        // add highlights:

        Line(mReal.wLeft + 1, mReal.wTop + 1,
            mReal.wLeft + 1, mReal.wBottom, Color);
        Line(mReal.wLeft + 2, mReal.wTop + 1, mReal.wRight - 1,
            mReal.wTop + 1, Color);

        Color.uForeground = PCLR_LOWLIGHT;

        // add edge
        Line(mReal.wRight - 1, mReal.wTop + 1, mReal.wRight - 1,
            mReal.wBottom - 1, Color);
        Line(mReal.wLeft + 1, mReal.wBottom - 1, mReal.wRight - 1,
            mReal.wBottom - 1, Color);

        // add shadows:

        Color.uForeground = PCLR_SHADOW;

        Line(mReal.wRight, mReal.wTop, mReal.wRight,
            mReal.wBottom, Color);
        Line(mReal.wLeft, mReal.wBottom, mReal.wRight,
            mReal.wBottom, Color);

    }

    if (Screen()->NumColors() >= 4)
    {
	    if (mwStyle & FF_RECESSED)
	    {
	        Color.uForeground = PCLR_HIGHLIGHT;
	
	        // add highlights:
	
	        Line(mReal.wLeft, mReal.wBottom,
	            mReal.wRight, mReal.wBottom, Color);
	        Line(mReal.wRight, mReal.wTop, mReal.wRight,
	            mReal.wBottom, Color);
	
	        Color.uForeground = PCLR_LOWLIGHT;
	
	        // add edge
	        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
	            mReal.wTop, Color);
	        Line(mReal.wLeft, mReal.wTop + 1, mReal.wLeft,
	            mReal.wBottom, Color);
	
	        Color.uForeground = PCLR_SHADOW;
	
	        // add edge
	        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wRight - 2,
	            mReal.wTop + 1, Color);
	        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wLeft + 1,
	            mReal.wBottom - 1, Color);
	
	        // add shadows:
	
	        Color.uForeground = PCLR_DIALOG;
	
	        Line(mReal.wLeft + 1, mReal.wBottom - 1,
	            mReal.wRight - 1, mReal.wBottom - 1, Color);
	        Line(mReal.wRight - 1, mReal.wTop + 1,
	            mReal.wRight - 1, mReal.wBottom - 1, Color);
	    }
    }

    if (mwStyle & FF_THICK)
    {
        Color.uForeground = PCLR_HIGHLIGHT;

        Line(mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wTop + PEG_FRAME_WIDTH - 1,
             mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wBottom - PEG_FRAME_WIDTH + 2, Color);
        
        Line(mReal.wLeft + PEG_FRAME_WIDTH - 1,
             mReal.wBottom - PEG_FRAME_WIDTH + 2,
             mReal.wRight - PEG_FRAME_WIDTH + 2,
             mReal.wBottom - PEG_FRAME_WIDTH + 2, Color);

        Color.uForeground = PCLR_LOWLIGHT;

        Line(mReal.wLeft + PEG_FRAME_WIDTH - 1, mClient.wTop - 1,
            mReal.wRight - PEG_FRAME_WIDTH, mClient.wTop - 1, Color);

        Line(mClient.wLeft - 1, mClient.wTop,
            mClient.wLeft - 1, mReal.wBottom - PEG_FRAME_WIDTH + 1, Color);
    }
}

#endif // runtime color checking if

/*--------------------------------------------------------------------------*/
void PegWindow::GetVScrollInfo(PegScrollInfo *Put)
{
    Put->wVisible = mClient.Height();
    SIGNED wTop = mClient.wTop;
    SIGNED wBottom = 0;
    PegThing *pTest = First();

    while(pTest)
    {
        if (!(pTest->StatusIs(PSF_NONCLIENT)))
        {
            if (pTest->mReal.wBottom > wBottom)
            {
                wBottom = pTest->mReal.wBottom;
            }
            if (pTest->mReal.wTop < wTop)
            {
                wTop = pTest->mReal.wTop;
            }
        }
        pTest = pTest->Next();
    }

    Put->wMin = wTop;
    Put->wCurrent = mClient.wTop;
    Put->wMax = wBottom;
    Put->wStep = mClient.Height() / 10;
}


/*--------------------------------------------------------------------------*/
void PegWindow::GetHScrollInfo(PegScrollInfo *Put)
{
    Put->wVisible = mClient.Width();
    SIGNED wLeft = mClient.wLeft;
    SIGNED wRight = 0;
    PegThing *pTest = First();

    while(pTest)
    {
        if (!(pTest->StatusIs(PSF_NONCLIENT)))
        {
            if (pTest->mReal.wRight > wRight)
            {
                wRight = pTest->mReal.wRight;
            }
            if (pTest->mReal.wLeft < wLeft)
            {
                wLeft = pTest->mReal.wLeft;
            }
        }
        pTest = pTest->Next();
    }

    Put->wMin = wLeft;
    Put->wMax = wRight;
    Put->wCurrent = mClient.wLeft;
    Put->wStep = mClient.Width() / 10;
}


/*--------------------------------------------------------------------------*/
BOOL PegWindow::CheckAutoScroll(void)
{
    return FALSE;
}


/*--------------------------------------------------------------------------*/
void PegWindow::SetScrollMode(UCHAR uMode)
{
}

/*--------------------------------------------------------------------------*/
void PegWindow::MoveClientObjects(SIGNED xShift, SIGNED yShift, BOOL bDraw)
{
    PegThing *Child = First();
    Invalidate();

    while(Child)
    {
        if (!(Child->StatusIs(PSF_NONCLIENT)))
        {
            PegRect MoveRect = Child->mReal;
            MoveRect.Shift(xShift, yShift);
            Child->Resize(MoveRect);
        }
        Child = Child->Next();
    }

    if (!bDraw)
    {
        return;
    }

    #ifdef FAST_BLIT
     if (StatusIs(PSF_CURRENT))
     //if (Presentation()->First() == this)
     {
        // scroll as much as possible using RectMove()

        PegPoint Put;
        PegRect Invalid, SaveClip;

        // figure out what needs to be re-drawn

        Invalid = mClient & mClip;
        SaveClip = mClient;

        if (xShift)
        {
            Put.y = mClient.wTop;

            if (xShift > 0)
            {
                Invalid.wRight = Invalid.wLeft + xShift;
                SaveClip.wRight -= xShift;
                Put.x = mClient.wLeft + xShift;
            }
            else
            {
                Invalid.wLeft = Invalid.wRight + xShift;
                SaveClip.wLeft -= xShift;
                Put.x = mClient.wLeft;
            }
        }
        else
        {
            Put.x = mClient.wLeft;
            if (yShift > 0)
            {
                Invalid.wBottom = Invalid.wTop + yShift;
                SaveClip.wBottom -= yShift;
                Put.y = mClient.wTop + yShift;
            }
            else
            {
                Invalid.wTop = Invalid.wBottom + yShift;
                SaveClip.wTop -= yShift;
                Put.y = mClient.wTop;
            }
        }
        BeginDraw();
        Screen()->RectMove(this, SaveClip, Put);
        EndDraw();

        SaveClip = mClip;
        Invalidate(Invalid);
        mClip = Invalid;
        Draw();
        mClip = SaveClip;
        return;
    }
    #endif

     // for non-accelerated hardware,
     // forced to re-draw the client area:

     Draw();
}

/*--------------------------------------------------------------------------*/
void PegWindow::Add(PegThing *Who, BOOL bDraw)
{
    BOOL bSetFocus = TRUE;
    if (Who->StatusIs(PSF_VISIBLE))
    {
        bSetFocus = FALSE;
    }
    if (Who != First())
    {
        PegThing::Add(Who, bDraw);

        if (bSetFocus)
        {
            if (StatusIs(PSF_VISIBLE) && StatusIs(PSF_CURRENT) &&
                Who->StatusIs(PSF_ACCEPTS_FOCUS) &&
                !Who->StatusIs(PSF_NONCLIENT))
            {
                Presentation()->MoveFocusTree(Who);
            }
        }
    }
}

/*--------------------------------------------------------------------------*/
void PegWindow::AddIcon(PegIcon *pIcon)
{
    PegPoint PutIcon;
    PutIcon.x = mClient.wLeft + 10;
    PutIcon.y = mClient.wBottom - pIcon->GetIcon()->wHeight - 10;;

    LOCK_PEG

    PegThing *pTemp = First();

    while(pTemp)
    {
        if (pTemp->Type() == TYPE_ICON)
        {
            if (pTemp->mReal.wRight - mClient.wLeft > PutIcon.x)
            {
                PutIcon.x = pTemp->mReal.wRight + 10;
            }
        }
        pTemp = pTemp->Next();
    }

    PegMessage NewMessage;
    NewMessage.wType = PM_SIZE;
    NewMessage.Rect.Set(PutIcon.x, PutIcon.y,
        PutIcon.x + pIcon->GetIcon()->wWidth, PutIcon.y + pIcon->GetIcon()->wHeight);

    pIcon->Message(NewMessage);
    Add(pIcon);

    UNLOCK_PEG
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegWindow::DrawMoveFrame(BOOL bErase)
{
    if (bErase)
    {
        if (!mbMoveFrame)
        {
            return;
        }
        mbMoveFrame = FALSE;
    }
    else
    {
        if (mbMoveFrame)
        {
            return;
        }
        mbMoveFrame = TRUE;
    }

    PegRect Invalid = mReal;

    if (mMovePoint.x > Parent()->mClient.wRight)
    {
        mMovePoint.x = Parent()->mClient.wRight;
    }
    else
    {
        if (mMovePoint.x < Parent()->mClient.wLeft)
        {
            mMovePoint.x = Parent()->mClient.wLeft;
        }
    }

    if (mMovePoint.y > Parent()->mClient.wBottom)
    {
        mMovePoint.y = Parent()->mClient.wBottom;
    }
    else
    {
        if (mMovePoint.y < Parent()->mClient.wTop)
        {
            mMovePoint.y = Parent()->mClient.wTop;
        }
    }
    int xShift = mMovePoint.x - mStartMove.x;
    int yShift = mMovePoint.y - mStartMove.y;

    switch(muMoveMode)
    {
    case PMM_MOVEALL:
        Invalid.Shift(xShift, yShift);
        break;

    case PMM_MOVERIGHT:
        Invalid.wRight += xShift;
        break;

    case PMM_MOVELEFT:
        Invalid.wLeft += xShift;
        break;

    case PMM_MOVETOP:
        Invalid.wTop += yShift;
        break;

    case PMM_MOVEBOTTOM:
        Invalid.wBottom += yShift;
        break;

    case PMM_MOVEUL:
        Invalid.wLeft += xShift;
        Invalid.wTop += yShift;
        break;

    case PMM_MOVELL:
        Invalid.wLeft += xShift;
        Invalid.wBottom += yShift;
        break;

    case PMM_MOVEUR:
        Invalid.wRight += xShift;
        Invalid.wTop += yShift;
        break;

    case PMM_MOVELR:
        Invalid.wRight += xShift;
        Invalid.wBottom += yShift;
        break;
    }

    CheckResizeRect(Invalid);

    PegRect OldClip = mClip;
    mClip = Parent()->mClip & Parent()->mClient;
    Invalidate(Invalid);
    BeginDraw();
    Screen()->RectangleXOR(this, Invalid);
    EndDraw();
    mClip = OldClip;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegWindow::CheckResizeRect(PegRect &NewSize)
{
    if (muMoveMode == PMM_MOVEALL)
    {
        return;
    }
    SIGNED iChange = mReal.Width() - NewSize.Width();
    SIGNED iMin;

    if (Type() == TYPE_WINDOW)
    {
        iMin = mClient.Width() - 4;
    }
    else
    {
        iMin = mReal.Width() - PEG_SCROLL_WIDTH * 5;
    }

    if (iChange > iMin)
    {
        switch(muMoveMode)
        {
        case PMM_MOVELEFT:
        case PMM_MOVEUL:
        case PMM_MOVELL:
            NewSize.wLeft -= iChange - iMin;
            break;

        default:
            NewSize.wRight += iChange - iMin;
        }
    }

    iChange = mReal.Height() - NewSize.Height();
    iMin = mClient.Height();

    if (iChange > iMin)
    {
        switch(muMoveMode)
        {
        case PMM_MOVETOP:
        case PMM_MOVEUL:
        case PMM_MOVEUR:
            NewSize.wTop -= iChange - iMin;
            break;

        default:
            NewSize.wBottom += iChange - iMin;
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
UCHAR PegWindow::BorderContains(PegPoint Point)
{
    if (!StatusIs(PSF_SIZEABLE))
    {
        return 0;
    }

    UCHAR iMoveMode = 0;
    
    if (Point.x >= mReal.wLeft && Point.x <= mReal.wLeft + PEG_FRAME_WIDTH)
    {
        iMoveMode = PMM_MOVELEFT;
    }
    else
    {
        if (Point.x <= mReal.wRight && Point.x >= mReal.wRight - PEG_FRAME_WIDTH)
        {
            iMoveMode = PMM_MOVERIGHT;
        }
    }
    
    if (Point.y >= mReal.wTop && Point.y <= mReal.wTop + PEG_FRAME_WIDTH)
    {
        iMoveMode |= PMM_MOVETOP;
    }
    else
    {
        if (Point.y <= mReal.wBottom && Point.y >= mReal.wBottom - PEG_FRAME_WIDTH)
        {
            iMoveMode |= PMM_MOVEBOTTOM;
        }
    }
    
    return iMoveMode;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegWindow::MoveFocusToFirstClientChild(void)
{
    PegThing *pTest = First();
    PegThing *pTarget = NULL;

    while(pTest)
    {
        if (pTest->StatusIs(PSF_ACCEPTS_FOCUS) &&
            !pTest->StatusIs(PSF_NONCLIENT))
        {
            pTarget = pTest;

            if (pTarget->StatusIs(PSF_KEEPS_CHILD_FOCUS))
            {
                break;
            }
            pTest = pTarget->First();
        }
        else
        {
            pTest = pTest->Next();
        }
    }
    if (pTarget && !pTarget->StatusIs(PSF_CURRENT))
    {
        Presentation()->MoveFocusTree(pTarget);
    }
}



// End of file
