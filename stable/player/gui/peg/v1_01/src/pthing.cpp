/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pthing.cpp - Base GUI object class definition.
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
// This class identifies the base functionality inherited by all GUI objects.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gui/peg/peg.hpp>

PegScreen *PegThing::mpScreen;
PegPresentationManager *PegThing::mpPresentation;
PegMessageQueue *PegThing::mpMessageQueue;

#ifdef PEG_UNICODE
const TCHAR gsPegVersion[] = {'P','E','G',' ','L','i','b','r','a','r','y',
    ' ','V','e','r','s','i','o','n',' ','1','.','7','8','\0'};
#else
const TCHAR gsPegVersion[] = "PEG Library Version 1.78";
#endif

/*--------------------------------------------------------------------------*/
PegThing::PegThing(const PegRect &Rect, WORD wId, WORD wStyle) :
    mReal(Rect),
    mClient(Rect),
    mClip(Rect),
    muType(TYPE_THING),
    mwStyle(wStyle),
    mwId(wId),
    mwStatus(PSF_SELECTABLE|PSF_ACCEPTS_FOCUS),
    mwSignalMask(0),
    mpParent(NULL),
    mpFirst(NULL),
    mpNext(NULL),
    mpPrev(NULL)
{
#ifdef PEG_FULL_CLIPPING
    mpViewportList = NULL;
#endif
}

/*--------------------------------------------------------------------------*/
PegThing::PegThing(WORD wId, WORD wStyle) :
    muType(TYPE_THING),
    mwStyle(wStyle),
    mwId(wId),
    mwStatus(PSF_SELECTABLE|PSF_ACCEPTS_FOCUS),
    mwSignalMask(0),
    mpParent(NULL),
    mpFirst(NULL),
    mpNext(NULL),
    mpPrev(NULL)

{
   #ifdef PEG_FULL_CLIPPING
    mpViewportList = NULL;
   #endif
    mReal.Set(0, 0, 0, 0);
    mClient = mReal;
    mClip = mReal;
}

/*--------------------------------------------------------------------------*/
// Destructor- Remove and delete any children.
/*--------------------------------------------------------------------------*/
PegThing::~PegThing()
{
    while (mpFirst)
    {
        PegThing *DelPtr = mpFirst;
        mpFirst = DelPtr->mpNext;
        delete DelPtr;
    }

   #ifdef PEG_FULL_CLIPPING
    LOCK_PEG
    if (mpViewportList)
    {
        Screen()->FreeViewports(this);
    }
    UNLOCK_PEG
   #endif
}


/*----------------------- PUBLIC METHOD ------------------------------------*/
// Add- Add a child to the list
/*--------------------------------------------------------------------------*/
void PegThing::Add(PegThing *What, BOOL bDraw)
{
    PegMessage Msg;
    PegThing *pTemp;

    LOCK_PEG

    // make sure it is not already in the list:

    if (What->mpParent == this)
    {
        if (mpFirst == What)
        {
            UNLOCK_PEG
            return;
        }
        pTemp = mpFirst;

        while(pTemp)
        {
            if (pTemp->mpNext == What)
            {
                // already in list, unlink it for move to front:

                pTemp->mpNext = What->mpNext;
                if (pTemp->mpNext)
                {
                    pTemp->mpNext->mpPrev = pTemp;
                }
                break;
            }
            pTemp = pTemp->mpNext;
        }
    }

    // update my links, default to putting What in front:

    What->mpParent = this;
    What->mpNext = mpFirst;
    What->mpPrev = NULL;

    if (mpFirst)
    {
        // Make sure to put What after any ALWAYS_ON_TOP objects:

        if (mpFirst->StatusIs(PSF_ALWAYS_ON_TOP) && 
            !What->StatusIs(PSF_ALWAYS_ON_TOP))
        {
            pTemp = mpFirst;

            while(pTemp->mpNext)
            {
                if (pTemp->mpNext->StatusIs(PSF_ALWAYS_ON_TOP))
                {
                    pTemp = pTemp->mpNext;
                }
                else
                {
                    break;
                }
            }

            // What goes after the object pointed to by pTemp:

            What->mpNext = pTemp->mpNext;
            What->mpPrev = pTemp;
            pTemp->mpNext = What;

            if (What->mpNext)
            {
                What->mpNext->mpPrev = What;
            }
        }
        else
        {
           #ifdef PEG_FULL_CLIPPING

            // Make sure we keep the Viewport owners on top of any lesser
            // siblings:

            if (!What->StatusIs(PSF_VIEWPORT) && mpFirst->StatusIs(PSF_VIEWPORT))
            {
                pTemp = mpFirst;
                while(pTemp->mpNext)
                {
                    if (pTemp->mpNext->StatusIs(PSF_VIEWPORT))
                    {
                        pTemp = pTemp->mpNext;
                    }
                    else
                    {
                        break;
                    }
                }                

                // What goes after the object pointed to by pTemp:

                What->mpNext = pTemp->mpNext;
                What->mpPrev = pTemp;
                pTemp->mpNext = What;

                if (What->mpNext)
                {
                    What->mpNext->mpPrev = What;
                }
            }
            else
            {
           #endif

                // What becomes my First child:

                mpFirst->mpPrev = What;
                mpFirst = What;

           #ifdef PEG_FULL_CLIPPING
            }
           #endif
        }
    }
    else
    {
        mpFirst = What;
    }

    // tell it to create itself:

    if (mwStatus & PSF_VISIBLE)
    {
        if (!(What->mwStatus & PSF_VISIBLE))
        {
            // insure a zero clipping area:
            What->mClip.wRight = What->mClip.wLeft - 1;

            // tell the object it is now visible

            Msg.wType = PM_SHOW;
            What->Message(Msg);
            
            // now update the objects clipping area:
            What->mClip = What->mReal & mClip;

            if (!What->StatusIs(PSF_NONCLIENT))
            {
                What->mClip &= mClient;
            }
            if (What->mpFirst)
            {
                What->UpdateChildClipping();
            }
        }

       #ifdef PEG_FULL_CLIPPING
        if (What->StatusIs(PSF_VIEWPORT))
        {
            Screen()->GenerateViewportList(this);
        }
       #endif

        if (bDraw)
        {
            Screen()->Invalidate(What->mClip);
            What->Draw();
        }
    }
    UNLOCK_PEG
}


/*----------------------- PUBLIC METHOD ------------------------------------*/
// AddToEnd- Add a child to the end of the list
/*--------------------------------------------------------------------------*/
void PegThing::AddToEnd(PegThing *What, BOOL bDraw)
{
    PegMessage Msg;
    PegThing *pCurrent;

    LOCK_PEG

    // make sure it is not already in the list:

    if (What->mpParent == this)
    {
        UNLOCK_PEG
        return;
    }

    if (mpFirst)
    {
        pCurrent = mpFirst;

        while(pCurrent)
        {
           #ifdef PEG_FULL_CLIPPING
            if (What->StatusIs(PSF_VIEWPORT) &&
                !pCurrent->StatusIs(PSF_VIEWPORT))
            {
                // keep viewport objects in front of non-viewport objects:
                
                What->mpPrev = pCurrent->mpPrev;

                if (pCurrent->mpPrev)
                {
                    pCurrent->mpPrev->mpNext = What;
                }
                else
                {
                    mpFirst = What;
                }
                pCurrent->mpPrev = What;
                What->mpNext = pCurrent;
                break;
            }
           #endif

            if(!pCurrent->mpNext)
            {
                pCurrent->mpNext = What;
                What->mpPrev = pCurrent;
                What->mpNext = NULL;
                break;
            }
            pCurrent = pCurrent->mpNext;
        }
    }
    else
    {
        What->mpNext = NULL;
        What->mpPrev = NULL;
        mpFirst = What;
    }

    pCurrent = What;
    pCurrent->mpParent = this;

    // tell it to create itself:

    if (mwStatus & PSF_VISIBLE)
    {
        if (!(pCurrent->mwStatus & PSF_VISIBLE))
        {
            Msg.wType = PM_SHOW;
            pCurrent->Message(Msg);
            UpdateChildClipping();
        }

       #ifdef PEG_FULL_CLIPPING
        if (pCurrent->StatusIs(PSF_VIEWPORT))
        {
            Screen()->GenerateViewportList(this);
        }
       #endif

        if (bDraw)
        {
            Screen()->Invalidate(pCurrent->mClip);
            pCurrent->Draw();
        }
    }
    UNLOCK_PEG
}


/*----------------------- PUBLIC METHOD ------------------------------------*/
// Remove- Remove a child from the list
/*--------------------------------------------------------------------------*/
PegThing *PegThing::Remove(PegThing *What, BOOL bDraw)
{
    LOCK_PEG

    PegThing *pParent = What->mpParent;

    if (!pParent)
    {
        UNLOCK_PEG
        return NULL;
    }
    if (pParent != this)
    {
        pParent = pParent->Remove(What, bDraw);
        UNLOCK_PEG
        return pParent;
    }

    PegThing *Current = mpFirst;
    PegThing *Last = Current;

    while(Current)
    {
        if (Current == What)
        {
            if (Current == mpFirst)
            {
                mpFirst = Current->mpNext;
                if (mpFirst)
                {
                    mpFirst->mpPrev = NULL;
                }
            }
            else
            {
                Last->mpNext = Current->mpNext;
                if (Last->mpNext)
                {
                    Last->mpNext->mpPrev = Last;
                }
            }
            Current->mpParent = NULL;
            Current->mpNext = NULL;
            Current->mpPrev = NULL;

            if (Current->StatusIs(PSF_VISIBLE))
            {
                Presentation()->NullInput(Current);
                Current->mwStatus &= ~PSF_VISIBLE;
                Screen()->Invalidate(Current->mClip);

               #ifdef PEG_FULL_CLIPPING
                if (Current->StatusIs(PSF_VIEWPORT))
                {
                    Screen()->FreeViewports(Current);
                    Screen()->GenerateViewportList(this);
                }
               #endif
            
                PegMessage Msg(PM_HIDE);
                Current->Message(Msg);
            
                if (bDraw)
                {
                    Draw();
                }
            }
            UNLOCK_PEG
            return(Current);
        }
        Last = Current;
        Current = Last->mpNext;
    }
    UNLOCK_PEG
    return NULL;
}


/*----------------------- PUBLIC METHOD ------------------------------------*/
// Destroy- Delete a child from its parent and from memory
/*--------------------------------------------------------------------------*/
void PegThing::Destroy(PegThing *What)
{
    PegMessage NewMessage;

   #ifdef PEG_MULTITHREAD

    if (What->Type() >= TYPE_WINDOW)
    {
        PEG_QUEUE_TYPE pQueue = Presentation()->GetThingMessageQueue(What);
        
        if (pQueue && pQueue != Presentation()->GetCurrentMessageQueue())
        {
            // If we get to here, then the window is being destroyed by
            // the current task, but executed modally by some other task.
            // Send the window a close message so that it exits the Execute() loop
            // and we return to the calling task:
            
            NewMessage.wType = PM_CLOSE;
            NewMessage.pTarget = What;
            NewMessage.pSource = this;
            MessageQueue()->Push(NewMessage);
            return;
        }
    }
   #endif

    if (What->mpParent)
    {
        Remove(What);
    }
    MessageQueue()->KillTimer(What, 0);
    MessageQueue()->Purge(What);    // no more messages for this guy!

    if (What != this)
    {
        delete What;
    }
    else
    {
        NewMessage.wType = PM_DESTROY;
        NewMessage.pSource = this;
        NewMessage.pTarget = Presentation();
        MessageQueue()->Push(NewMessage);
    }
}


/*--------------------------------------------------------------------------*/
SIGNED PegThing::Message(const PegMessage &Mesg)
{
    switch(Mesg.wType)
    {
    case PM_POINTER_ENTER:
    case PM_POINTER_EXIT:
    case PM_POINTER_MOVE:
        break;

    case PM_LBUTTONDOWN:
        if (!CheckSendSignal(PSF_CLICKED))
        {
            if (Parent())
            {
                return Parent()->Message(Mesg);
            }
        }
        break;

    case PM_DRAW:
        Invalidate(mReal);
        Draw();
        break;

    case PM_SHOW:
        mwStatus |= PSF_VISIBLE;
        MessageChildren(Mesg);
        break;

    case PM_HIDE:
        if (mwStatus & PSF_CURRENT)
        {
            Presentation()->NullInput(this);
        }
        mwStatus &= ~(PSF_VISIBLE | PSF_CURRENT);
        MessageChildren(Mesg);
        break;

    case PM_ADD:
        if (Mesg.pSource && Mesg.pTarget == this)
        {
            Add(Mesg.pSource);
        }
        break;

    case PM_REMOVETHING:
        if (Mesg.pSource && Mesg.pTarget == this)
        {
            Remove(Mesg.pSource);
        }
        break;

    case PM_NONCURRENT:
        if (mwStatus & PSF_CURRENT)
        {
            mwStatus &= ~PSF_CURRENT;
            PegThing *Current = mpFirst;

            while (Current)
            {
                if (Current->mwStatus & PSF_CURRENT)
                {
                    Current->Message(Mesg);
                    break;
                }
                Current = Current->mpNext;
            }
        }
        break;

    case PM_CURRENT:
        if (!(mwStatus & PSF_CURRENT))
        {
            mwStatus |= PSF_CURRENT;
            if (mpParent)
            {
                if (!(mpParent->mwStatus & PSF_CURRENT))
                {
                    mpParent->Message(Mesg);
                }
            }
        }
        break;

    case PM_DESTROY:
        if (Mesg.pSource)
        {
            PegThing *pKill = Mesg.pSource;
            MessageQueue()->Purge(pKill);    // no more messages for this guy!
            delete pKill;
        }
        break;

    case PM_SIZE:
        Resize(Mesg.Rect);
        CheckSendSignal(PSF_SIZED);
        break;

    case PM_PARENTSIZED:
        break;

   #ifdef PEG_KEYBOARD_SUPPORT
    case PM_KEY:
        DefaultKeyHandler(Mesg);
        break;
   #endif

    default:
        if (mpParent)
        {
            if (mpParent != Presentation())
            {
                return(mpParent->Message(Mesg));
            }
        }
        break;
    }
    return(0);
}


/*--------------------------------------------------------------------------*/
// The base draw item simply instructs all children to draw themselves.
/*--------------------------------------------------------------------------*/
void PegThing::Draw(void)
{
    if (mwStatus & PSF_VISIBLE)
    {
        BeginDraw();
        DrawChildren();
        EndDraw();
    }
}


/*--------------------------------------------------------------------------*/
// The default key handler, only included if PEG_KEYBOARD_SUPPORT is defined.
// This handler checks for TAB and Up/Down/Left/Right keys to move focus,
// all other keys are passed on to the parent.
/*--------------------------------------------------------------------------*/

#ifdef PEG_KEYBOARD_SUPPORT
void PegThing::DefaultKeyHandler(const PegMessage &InMesg)
{
    PegThing *pTest;

    switch(InMesg.iData)
    {
    case PK_TAB:
        if (mpParent)
        {
            // If CTRL+TAB, move to the next top level window by
            // passing the message up to the top level window.

            if (InMesg.lData & KF_CTRL)
            {
                if (mpParent != Presentation()) // if I am not top-level
                {
                    pTest = mpParent;

                    while(pTest->mpParent != Presentation())
                    {
                        pTest = pTest->mpParent;
                    }
                    pTest->Message(InMesg);
                    return;
                }
                // else I am a top level object, fall through to
                // normal tab-key handling
            }
            else
            {
                if (mpParent == Presentation())
                {
                    // Here if I am a top level object and received
                    // a TAB message. If I am a window, this means that focus
                    // has been lost by operating with a mouse and
                    // with a keyboard. The mouse was clicked on the
                    // window after the window had focus, causing
                    // input focus to leave the default control.

                    if (Type() >= TYPE_WINDOW)
                    {
                        PegWindow *pw = (PegWindow *) this;
                        pw->MoveFocusToFirstClientChild();
                    }
                    return;
                }
            }
            if (mpParent->Type() == TYPE_GROUP || 
                mpParent->Type() == TYPE_VLIST ||
                mpParent->Type() == TYPE_HLIST ||
                mpParent->Type() == TYPE_COMBO)
            {
                mpParent->Message(InMesg);
                return;
            }
        }
        if (Type() >= TYPE_WINDOW || InMesg.lData & KF_SHIFT)
        {
            pTest = mpPrev;
        }
        else
        {
            pTest = mpNext;
        }
        while(pTest)
        {
            if ((pTest->mwStatus & PSF_TAB_STOP) &&
                !(pTest->mwStatus & PSF_NONCLIENT))
            {
                Presentation()->MoveFocusTree(pTest);
                break;
            }
            if (InMesg.lData & KF_SHIFT)
            {
                pTest = pTest->mpPrev;
            }
            else
            {
                pTest = pTest->mpNext;
            }
        }
        if (!pTest && mpParent)
        {
            pTest = mpParent->mpFirst;
            if (Type() >= TYPE_WINDOW || InMesg.lData & KF_SHIFT)
            {
                while(pTest->mpNext)
                {
                    pTest = pTest->mpNext;
                }
            }
            while(pTest)
            {
                if (pTest == this)
                {
                    break;
                }
                if ((pTest->mwStatus & PSF_TAB_STOP) &&
                    !(pTest->mwStatus & PSF_NONCLIENT))
                {
                    Presentation()->MoveFocusTree(pTest);
                    break;
                }
                if (Type() >= TYPE_WINDOW || InMesg.lData & KF_SHIFT)
                {
                    pTest = pTest->mpPrev;
                }
                else
                {
                    pTest = pTest->mpNext;
                }
            }
        }
        break;

    case PK_LNUP:
    case PK_LNDN:
    case PK_RIGHT:
    case PK_LEFT:
        if (!(InMesg.lData & (KF_SHIFT|KF_CTRL)))
        {
            if (CheckDirectionalMove(InMesg.iData))
            {
                return;
            }
        }

        // fall through to default case:

    default:
        if (!CheckSendSignal(PSF_KEY_RECEIVED))
        {
            if (Parent())
            {
                Parent()->Message(InMesg);
            }
        }
        break;
    }
}


/*--------------------------------------------------------------------------*/
// CheckDirectionalMove- default arrow key handling. Returns TRUE if
// the key was processes, otherwise false. If FALSE is returned, the
// key is passed to the parent object.
/*--------------------------------------------------------------------------*/
BOOL PegThing::CheckDirectionalMove(SIGNED iKey, BOOL bLoose)
{
    if (!Parent())
    {
        return TRUE;
    }

    if (Parent() == Presentation())
    {
        return TRUE;
    }

    WORD wType = Parent()->Type();

    if (wType == TYPE_VLIST || wType == TYPE_HLIST ||
        wType == TYPE_COMBO)
    {
        return FALSE;
    }

    PegPoint CenterThat;
    PegThing *pWinner = NULL;
    PegPoint CenterThis = CenterOf(this);
    PegThing *pSib = Parent()->First();
    LONG lDist = 0x7fffffffL;
    LONG lTest;

    switch(iKey)
    {
    case PK_LNUP:
        while(pSib)
        {
            if (pSib != this && pSib->StatusIs(PSF_TAB_STOP))
            {
                if (!bLoose)
                {
                    if (pSib->mReal.wLeft > mReal.wRight ||
                        pSib->mReal.wRight < mReal.wLeft)
                    {
                        pSib = pSib->Next();
                        continue;
                    }
                }

                CenterThat = CenterOf(pSib);
                if (CenterThat.y < CenterThis.y)
                {
                    lTest = Distance(CenterThis, CenterThat);
                    if (lTest < lDist)
                    {
                        lDist = lTest;
                        pWinner = pSib;
                    }
                }
            }
            pSib = pSib->Next();                
        }
        break;

    case PK_LNDN:
        while(pSib)
        {
            if (pSib != this && pSib->StatusIs(PSF_TAB_STOP))
            {
                if (!bLoose)
                {
                    if (pSib->mReal.wLeft > mReal.wRight ||
                        pSib->mReal.wRight < mReal.wLeft)
                    {
                        pSib = pSib->Next();
                        continue;
                    }
                }

                CenterThat = CenterOf(pSib);
                if (CenterThat.y > CenterThis.y)
                {
                    lTest = Distance(CenterThis, CenterThat);
                    if (lTest < lDist)
                    {
                        lDist = lTest;
                        pWinner = pSib;
                    }
                }
            }
            pSib = pSib->Next();                
        }
        break;

    case PK_RIGHT:
        while(pSib)
        {
            if (pSib != this && pSib->StatusIs(PSF_TAB_STOP))
            {
                if (!bLoose)
                {
                    if (pSib->mReal.wTop > mReal.wBottom ||
                        pSib->mReal.wBottom < mReal.wTop)
                    {
                        pSib = pSib->Next();
                        continue;
                    }
                }

                CenterThat = CenterOf(pSib);
                if (CenterThat.x > CenterThis.x)
                {
                    lTest = Distance(CenterThis, CenterThat);
                    if (lTest < lDist)
                    {
                        lDist = lTest;
                        pWinner = pSib;
                    }
                }
            }
            pSib = pSib->Next();                
        }
        break;

    case PK_LEFT:
        while(pSib)
        {
            if (pSib != this && pSib->StatusIs(PSF_TAB_STOP))
            {
                if (!bLoose)
                {
                    if (pSib->mReal.wTop > mReal.wBottom ||
                        pSib->mReal.wBottom < mReal.wTop)
                    {
                        pSib = pSib->Next();
                        continue;
                    }
                }

                CenterThat = CenterOf(pSib);
                if (CenterThat.x < CenterThis.x)
                {
                    lTest = Distance(CenterThis, CenterThat);
                    if (lTest < lDist)
                    {
                        lDist = lTest;
                        pWinner = pSib;
                    }
                }
            }
            pSib = pSib->Next();                
        }
        break;
    }

    if (pWinner)
    {
        Presentation()->MoveFocusTree(pWinner);
        return TRUE;
    }
    else
    {
        if (!bLoose)
        {
            return (CheckDirectionalMove(iKey, TRUE));
        }
        else
        {
            return FALSE;
        }
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
LONG PegThing::Distance(PegPoint p1, PegPoint p2)
{
    LONG l1 = p1.x - p2.x;
    l1 *= l1;
    LONG l2 = p1.y - p2.y;
    l2 *= l2;
    return(l1 + l2);        // no need for square root, as we are looking
                            // at relative values
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegPoint PegThing::CenterOf(PegThing *Who)
{
    PegPoint Center;
    Center.x = (Who->mReal.wLeft + Who->mReal.wRight) >> 1;
    Center.y = (Who->mReal.wTop + Who->mReal.wBottom) >> 1;
    return Center;
}

#endif      // PEG_KEYBOARD_SUPPORT if

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
const TCHAR *PegThing::Version(void)
{
    return gsPegVersion;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::MessageChildren(const PegMessage &Mesg)
{
    PegThing *pThing = mpFirst;

    while(pThing)
    {
        pThing->Message(Mesg);
        pThing = pThing->mpNext;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::DrawChildren(void)
{
    // draw the first last, so that it comes out on top on top:

    if (!mpFirst)
    {
        return;
    }

    PegThing *pCurrent = mpFirst;

    while(pCurrent->mpNext)
    {
        pCurrent = pCurrent->mpNext;
    }

    while(pCurrent)
    {
       #ifdef PEG_AWT_SUPPORT
        if (mReal.Overlap(pCurrent->mClip))
        {
            pCurrent->Draw();
        }
       #else
        if (Screen()->InvalidOverlap(pCurrent->mClip))
        {
            pCurrent->Draw();
        }
       #endif
        pCurrent = pCurrent->mpPrev;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegThing *PegThing::Find(WORD wId, BOOL bRecursive)
{
    LOCK_PEG
    PegThing *pTest = First();

    while(pTest)
    {
        if (pTest->Id() == wId)
        {
            UNLOCK_PEG
            return (pTest);
        }

        if (bRecursive)
        {
            PegThing *pReturn = pTest->Find(wId);
            if (pReturn)
            {
                UNLOCK_PEG
                return(pReturn);
            }
        }
        pTest = pTest->Next();
    }
    UNLOCK_PEG
    return NULL;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::SendSignal(UCHAR uSignal)
{
    PegMessage NewMessage(mpParent, PEG_SIGNAL(mwId, uSignal));
    NewMessage.pSource = this;
    NewMessage.iData = mwId;
    MessageQueue()->Push(NewMessage);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::KillFocus(PegThing *Start)
{
    if (Start)
    {
        PegMessage Mesg;
        Mesg.wType = PM_NONCURRENT;
        Mesg.pTarget = Start;
        Start->Message(Mesg);
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::ParentShift(SIGNED xOffset, SIGNED yOffset)
{
    mReal.Shift(xOffset, yOffset);
    mClient.Shift(xOffset, yOffset);
    mClip.Shift(xOffset, yOffset);

    PegThing *pTemp = mpFirst;

    while (pTemp)
    {
        pTemp->ParentShift(xOffset, yOffset);
        pTemp = pTemp->mpNext;
    }
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::Resize(PegRect NewSize)
{
    LOCK_PEG

    if (mwStatus & PSF_VISIBLE)
    {
        Invalidate(mClip);
    }
    SIGNED xShift = NewSize.wLeft - mReal.wLeft;
    SIGNED yShift = NewSize.wTop - mReal.wTop;
    mReal.Shift(xShift, yShift);
    mClient.Shift(xShift, yShift);

    SIGNED xSize = NewSize.Width() - mReal.Width();
    SIGNED ySize = NewSize.Height() - mReal.Height();
    mReal.wBottom += ySize;
    mReal.wRight += xSize;
    mClient.wRight += xSize;
    mClient.wBottom += ySize;

    if (xShift || yShift)
    {
        PegThing *pTemp = mpFirst;

        while(pTemp)
        {
            pTemp->ParentShift(xShift, yShift);
            pTemp = pTemp->mpNext;
        }
    }

    if (mwStatus & PSF_VISIBLE)
    {
        mClip = mReal & mpParent->mClip;
        if (!StatusIs(PSF_NONCLIENT))
        {
            mClip &= mpParent->mClient;
        }
        if (mpFirst)
        {
            UpdateChildClipping();
        }
        //mpParent->UpdateChildClipping();

       #ifdef PEG_FULL_CLIPPING

        if (StatusIs(PSF_VIEWPORT))
        {
            Screen()->GenerateViewportList(Parent());
        }

       #endif

        Invalidate(mClip);
    }
    UNLOCK_PEG
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::UpdateChildClipping(void)
{
    PegThing *Current = mpFirst;
    while (Current)
    {
        // set my first childs clipping rectangle:

        Current->mClip = Current->mReal & mClip;
        if (!(Current->mwStatus & PSF_NONCLIENT))
        {
            Current->mClip &= mClient;
        }
        if (Current->First())
        {
            Current->UpdateChildClipping();
        }
        Current = Current->mpNext;
    }
}

/*--------------------------------------------------------------------------*/
void PegThing::Center(PegThing *Who)
{
    SIGNED ixSpare = (mClient.Width() - Who->mReal.Width()) / 2;
    SIGNED iySpare = (mClient.Height() - Who->mReal.Height()) / 2;

    PegRect NewSize = Who->mReal;
    NewSize.MoveTo(mClient.wLeft + ixSpare, mClient.wTop + iySpare);
    Who->Resize(NewSize);
}

/*--------------------------------------------------------------------------*/
void PegThing::SetColor(const UCHAR uIndex, const COLORVAL uColor)
{
    if (uIndex < THING_COLOR_LIST_SIZE)
    {
        muColors[uIndex] = uColor;
        Invalidate(mReal);
    }
}

/*--------------------------------------------------------------------------*/
COLORVAL PegThing::GetColor(const UCHAR uIndex)
{
    if (uIndex < THING_COLOR_LIST_SIZE)
    {
        return muColors[uIndex];
    }
    return 255;
}



/*--------------------------------------------------------------------------*/
void PegThing::InitClient(void)
{
    mClient = mReal;

    switch(mwStyle & FF_MASK)
    {
  #ifdef PEG_RUNTIME_COLOR_CHECK
    case FF_RECESSED:
    case FF_RAISED:
        if (Screen()->NumColors() >= 4)
        {
            mClient -= 2;
        }
        else
        {
            mClient--;
        }
        break;
  #else
   #if (PEG_NUM_COLORS != 2)
    case FF_RECESSED:
    case FF_RAISED:
        mClient -= 2;
        break;
   #endif
  #endif

    case FF_THIN:
        mClient--;
        break;

    case FF_THICK:
        mClient -= PEG_FRAME_WIDTH;
        break;

    case FF_NONE:
    default:
        break;
    }
}

#ifndef PEG_RUNTIME_COLOR_CHECK

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::StandardBorder(COLORVAL bFillColor)
{
    PegColor Color(PCLR_BORDER, bFillColor, CF_FILL);

   #if (PEG_NUM_COLORS != 2)    
    PegRect Rect;
   #endif

    switch(mwStyle & FF_MASK)
    {
   #if (PEG_NUM_COLORS != 2)

    case  FF_RAISED:
        Rect = mReal;
        Rect -= 2;
        Rectangle(Rect, Color, 0);

        Line(mReal.wLeft, mReal.wTop + 1,
            mReal.wLeft, mReal.wBottom, Color);
        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
            mReal.wTop, Color);

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
        break;

    case FF_RECESSED:
        Rect = mReal;
        Rect -= 2;
        Rectangle(Rect, Color, 0);

        if (StatusIs(PSF_VISIBLE) &&
            Parent()->muColors[PCI_NORMAL] == PCLR_CLIENT)
        {
            Line(mReal.wRight, mReal.wTop + 1,
                mReal.wRight, mReal.wBottom, Color);
            Line(mReal.wLeft, mReal.wBottom, mReal.wRight,
                mReal.wBottom, Color);

            // add highlights:

            Color.uForeground = PCLR_HIGHLIGHT;
            Line(mReal.wLeft, mReal.wBottom - 1,
                mReal.wRight, mReal.wBottom - 1, Color);
            Line(mReal.wRight - 1, mReal.wTop, mReal.wRight - 1,
                mReal.wBottom, Color);
        }
        else
        {
            Line(mReal.wLeft, mReal.wTop + 1,
                mReal.wLeft, mReal.wBottom, Color);
            Line(mReal.wLeft, mReal.wTop, mReal.wRight,
                mReal.wTop, Color);

            // add highlights:

            Color.uForeground = PCLR_HIGHLIGHT;
            Line(mReal.wLeft, mReal.wBottom,
                mReal.wRight, mReal.wBottom, Color);
            Line(mReal.wRight, mReal.wTop, mReal.wRight,
                mReal.wBottom, Color);
        }

        Color.uForeground = PCLR_LOWLIGHT;

        // add edge
        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
            mReal.wTop, Color);
        Line(mReal.wLeft, mReal.wTop + 1, mReal.wLeft,
            mReal.wBottom, Color);

        Color.uForeground = PCLR_SHADOW;

        // add edge
        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wRight - 1,
            mReal.wTop + 1, Color);
        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wLeft + 1,
            mReal.wBottom - 1, Color);

        // add shadows:

        Color.uForeground = PCLR_DIALOG;

        if (StatusIs(PSF_VISIBLE) &&
            Parent()->muColors[PCI_NORMAL] == PCLR_CLIENT)
        {
            Line(mReal.wLeft, mReal.wBottom,
                mReal.wRight, mReal.wBottom, Color);
            Line(mReal.wRight, mReal.wTop,
                mReal.wRight, mReal.wBottom - 1, Color);
        }
        else
        {
            // normal
            Line(mReal.wLeft + 1, mReal.wBottom - 1,
                mReal.wRight - 1, mReal.wBottom - 1, Color);
            Line(mReal.wRight - 1, mReal.wTop + 1,
                mReal.wRight - 1, mReal.wBottom - 1, Color);
        }
        break;

   #endif       // NUM_COLORS != 2 check

    case FF_THIN:
        Color.uForeground = PCLR_SHADOW;
        Rectangle(mReal, Color, 1);
        break;

    case FF_NONE:
        Rectangle(mReal, Color, 0);
        break;

    default:
        break;
    }
}

#else // here for RUNTIME_COLOR_CHECK enabled

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::StandardBorder(COLORVAL bFillColor)
{
    PegColor Color(PCLR_BORDER, bFillColor, CF_FILL);
    PegRect Rect;

    switch(mwStyle & FF_MASK)
    {
    case  FF_RAISED:
        if (Screen()->NumColors() >= 4)
        {
	        Rect = mReal;
	        Rect -= 2;
	        Rectangle(Rect, Color, 0);
	
	        Line(mReal.wLeft, mReal.wTop + 1,
	            mReal.wLeft, mReal.wBottom, Color);
	        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
	            mReal.wTop, Color);
	
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
        else
        {
            Color.uForeground = PCLR_SHADOW;
            Rectangle(mReal, Color, 1);
        }
        break;

    case FF_RECESSED:
        if (Screen()->NumColors() >= 4)
        {
	        Rect = mReal;
	        Rect -= 2;
	        Rectangle(Rect, Color, 0);
	
	        if (StatusIs(PSF_VISIBLE) &&
	            Parent()->muColors[PCI_NORMAL] == PCLR_CLIENT)
	        {
	            Line(mReal.wRight, mReal.wTop + 1,
	                mReal.wRight, mReal.wBottom, Color);
	            Line(mReal.wLeft, mReal.wBottom, mReal.wRight,
	                mReal.wBottom, Color);
	
	            // add highlights:
	
	            Color.uForeground = PCLR_HIGHLIGHT;
	            Line(mReal.wLeft, mReal.wBottom - 1,
	                mReal.wRight, mReal.wBottom - 1, Color);
	            Line(mReal.wRight - 1, mReal.wTop, mReal.wRight - 1,
	                mReal.wBottom, Color);
	        }
	        else
	        {
	            Line(mReal.wLeft, mReal.wTop + 1,
	                mReal.wLeft, mReal.wBottom, Color);
	            Line(mReal.wLeft, mReal.wTop, mReal.wRight,
	                mReal.wTop, Color);
	
	            // add highlights:
	
	            Color.uForeground = PCLR_HIGHLIGHT;
	            Line(mReal.wLeft, mReal.wBottom,
	                mReal.wRight, mReal.wBottom, Color);
	            Line(mReal.wRight, mReal.wTop, mReal.wRight,
	                mReal.wBottom, Color);
	        }
	
	        Color.uForeground = PCLR_LOWLIGHT;
	
	        // add edge
	        Line(mReal.wLeft, mReal.wTop, mReal.wRight,
	            mReal.wTop, Color);
	        Line(mReal.wLeft, mReal.wTop + 1, mReal.wLeft,
	            mReal.wBottom, Color);
	
	        Color.uForeground = PCLR_SHADOW;
	
	        // add edge
	        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wRight - 1,
	            mReal.wTop + 1, Color);
	        Line(mReal.wLeft + 1, mReal.wTop + 1, mReal.wLeft + 1,
	            mReal.wBottom - 1, Color);
	
	        // add shadows:
	
	        Color.uForeground = PCLR_DIALOG;
	
	        if (StatusIs(PSF_VISIBLE) &&
	            Parent()->muColors[PCI_NORMAL] == PCLR_CLIENT)
	        {
	            Line(mReal.wLeft, mReal.wBottom,
	                mReal.wRight, mReal.wBottom, Color);
	            Line(mReal.wRight, mReal.wTop,
	                mReal.wRight, mReal.wBottom - 1, Color);
	        }
	        else
	        {
	            // normal
	            Line(mReal.wLeft + 1, mReal.wBottom - 1,
	                mReal.wRight - 1, mReal.wBottom - 1, Color);
	            Line(mReal.wRight - 1, mReal.wTop + 1,
	                mReal.wRight - 1, mReal.wBottom - 1, Color);
	        }
	    }
        else
        {
            Color.uForeground = PCLR_SHADOW;
            Rectangle(mReal, Color, 1);
        }
        break;

    case FF_THIN:
        Color.uForeground = PCLR_SHADOW;
        Rectangle(mReal, Color, 1);
        break;

    case FF_NONE:
        Rectangle(mReal, Color, 0);
        break;

    default:
        break;
    }
}

#endif        // runtime color checking if

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::CapturePointer(void)
{
    Presentation()->CapturePointer(this);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegThing::ReleasePointer(void)
{
    Presentation()->ReleasePointer(this);
}


// End of file


