/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// ppresent.cpp - PegPresentationManager.
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

#include "string.h"
#include <gui/peg/peg.hpp>


/*--------------------------------------------------------------------------*/
PegPresentationManager::PegPresentationManager(PegRect &Total) :
    PegWindow(Total, FF_NONE)
{
    mpInputThing = NULL;
    mpLastPointerOver = NULL;
    mpScratchPad = NULL;
    RemoveStatus(0xffff);

    // I am ALLWAYS in the current branch
    AddStatus(PSF_VIEWPORT|PSF_CURRENT|PSF_VISIBLE|PSF_ACCEPTS_FOCUS);

    muColors[PCI_NORMAL] = PCLR_DESKTOP;

   #ifdef PEG_RUNTIME_COLOR_CHECK
    if (Screen()->NumColors() < 4)
    {
        muColors[PCI_NORMAL] = WHITE;
    }
   #endif

    muPointerCaptures = 0;
    mbMoveFocusFlag = FALSE;
    mpWallpaper = NULL;

   #ifdef PEG_MULTITHREAD
    mpTaskInfo = NULL;
   #endif
}


/*--------------------------------------------------------------------------*/
PegPresentationManager::~PegPresentationManager()
{
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::Draw(void)
{
    if (mpWallpaper)
    {
        BeginDraw();

        if (HAS_TRANS(mpWallpaper))
        {
            DrawFrame();
        }

        BitmapFill(mReal, mpWallpaper);
        DrawChildren();
        EndDraw();
    }
    else
    {
        PegWindow::Draw();
    }
}

/*--------------------------------------------------------------------------*/
SIGNED PegPresentationManager::Message(const PegMessage &Mesg)
{
    switch(Mesg.wType)
    {
    case PM_EXIT:
        if (Mesg.pSource == this)  // did I send this Exit message?
        {
            if (!First())           // still no top-level windows?
            {
                if (!Mesg.iData)
                {
                    PegMessage NewSend = Mesg;
                    NewSend.iData = 1;
                    MessageQueue()->Push(NewSend);
                }
                else
                {
                    return PM_EXIT;     // exit!
                }
            }
        }
        else
        {
            return PM_EXIT;
        }
        break;

    case PM_CLOSE:
        MessageChildren(Mesg);
        break;

    case PM_MOVE_FOCUS:
        if (mbMoveFocusFlag)
        {
            if (First())
            {
                mpInputThing = First();
                SetFocus(First());
            }
            mbMoveFocusFlag = FALSE;
        }
        break;

    default:
        return PegWindow::Message(Mesg);
    }
    return 0;
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::NullInput(PegThing *Current)
{
    PegThing *pTest;

    // Check to see if the thing being removed is or is a parent of my
    // private "last pointer over" thing:

    if (mpLastPointerOver)
    {
        pTest = mpLastPointerOver;

        while(pTest && pTest != Current)
        {
            pTest = pTest->Parent();
        }
        if (pTest == Current)
        {
            mpLastPointerOver = NULL;
        }
    }
    
    // Check to see if the thing being removed is or is a parent of 
    // my private "Input Thing":

    if (mpInputThing)
    {
        pTest = mpInputThing;

        while(pTest && pTest != Current)
        {
            pTest = pTest->Parent();
        }
        if (pTest == Current)
        {
            if (mpInputThing->StatusIs(PSF_OWNS_POINTER))
            {
                ReleasePointer(mpInputThing);
            }
            mpInputThing = NULL;

        }
    }
}

/*--------------------------------------------------------------------------*/
PegThing *PegPresentationManager::Remove(PegThing *What, BOOL bDraw)
{
    if (What)
    {
        if (What->Type() >= TYPE_WINDOW && What->StatusIs(PSF_CURRENT))
        {
            mbMoveFocusFlag = TRUE;
        }
    }
    PegThing *Return = PegWindow::Remove(What, bDraw);

    PegMessage NewMessage;
    NewMessage.pTarget = this;
    NewMessage.pSource = this;
    
    if (!First())        // last window removed?
    {
        NewMessage.wType = PM_EXIT;
        NewMessage.iData = 0;
        MessageQueue()->Push(NewMessage);
    }
    else
    {
        if (mbMoveFocusFlag)
        {
            NewMessage.wType = PM_MOVE_FOCUS;
            MessageQueue()->Push(NewMessage);
        }
    }
    return Return;
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::Add(PegThing *What, BOOL bDraw)
{
    // An object can only be placed
    // on top if the pointer is not captured by another object

    if (muPointerCaptures)
    {
        if (mpPointerOwners[muPointerCaptures - 1] != What &&
            What->mReal.Overlap(mpPointerOwners[muPointerCaptures - 1]->mReal))
        {
            PegThing::AddToEnd(What, bDraw);
            return;
        }
    }

    if (What->Type() >= TYPE_WINDOW)
    {
        mbMoveFocusFlag = FALSE;    
    }

    LOCK_PEG

    if (!(What->StatusIs(PSF_VISIBLE)) && What->StatusIs(PSF_ACCEPTS_FOCUS))
    {
        if (First())
        {
            KillFocus(First());
        }

        PegThing::Add(What, bDraw);
        mpInputThing = What;
        SetFocus(What);
    }
    else
    {
        PegThing::Add(What, bDraw);
    }

    UNLOCK_PEG
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegPresentationManager::CapturePointer(PegThing *Who)
{
    LOCK_PEG
    if (muPointerCaptures < MAX_POINTER_CAPTURE_NESTING)
    {
        if (!muPointerCaptures)
        {
            mpDefaultInputThing = mpInputThing;
        }
        mpInputThing = Who;
        mpPointerOwners[muPointerCaptures ] = Who;
        muPointerCaptures++;
        Who->AddStatus(PSF_OWNS_POINTER);
    }
    UNLOCK_PEG
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegPresentationManager::ReleasePointer(PegThing *Who)
{
    LOCK_PEG
    if (Who->StatusIs(PSF_OWNS_POINTER))
    {
        Who->RemoveStatus(PSF_OWNS_POINTER);

        if (muPointerCaptures > 0)
        {
            for (SIGNED uLoop = muPointerCaptures - 1; uLoop >= 0; uLoop--)
            {
                if (mpPointerOwners[uLoop] == Who)
                {
                    for (SIGNED uLoop1 = uLoop; uLoop1 < muPointerCaptures - 1; uLoop1++)
                    {
                        mpPointerOwners[uLoop1] = mpPointerOwners[uLoop1 + 1];
                    }
                    muPointerCaptures--;
                    break;
                }
            }

            if (muPointerCaptures)
            {
                mpInputThing = mpPointerOwners[muPointerCaptures - 1];
                mpInputThing->AddStatus(PSF_OWNS_POINTER);
            }
            else
            {
                mpInputThing = mpDefaultInputThing;
            }
        }
        else
        {
            mpInputThing = mpDefaultInputThing;
        }
    }
    UNLOCK_PEG
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
PegThing *PegPresentationManager::GetPointerOwner(void)
{
    if (muPointerCaptures)
    {
        return mpPointerOwners[muPointerCaptures - 1];
    }
    return NULL;
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegPresentationManager::SetScratchPad(TCHAR *pText)
{
    if (mpScratchPad)
    {
        delete mpScratchPad;
        mpScratchPad = NULL;
    }

    if (pText)
    {
        if (*pText)
        {
            mpScratchPad = new TCHAR[tstrlen(pText) + 1];
            tstrcpy(mpScratchPad, pText);
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegPresentationManager::ClearScratchPad(void)
{
    if (mpScratchPad)
    {
        delete(mpScratchPad);
        mpScratchPad = NULL;
    }
}

/*--------------------------------------------------------------------------*/
// FindLowestThingContaining-
// Find the lowest element which contains a point.
/*--------------------------------------------------------------------------*/

PegThing *PegPresentationManager::
    FindLowestThingContaining(PegThing *Start, PegPoint Point)
{
    LOCK_PEG

    PegThing *Current = Start->First();

    while(Current)
    {
        if (Current->mClip.Contains(Point))
        {
            if (Current->StatusIs(PSF_VISIBLE) &&
                Current->StatusIs(PSF_SELECTABLE))
            {
                Start = Current;
                Current = Current->First();
                continue;
            }
        }
        Current = Current->Next();
    }
    UNLOCK_PEG
    return (Start);
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::KillFocus(PegThing *Start)
{
    while (Start)
    {
        if (Start->StatusIs(PSF_CURRENT))
        {
            PegMessage Mesg;
            Mesg.wType = PM_NONCURRENT;
            Mesg.pTarget = Start;
            Start->Message(Mesg);
            return;
        }
        Start = Start->Next();
    }
}

/*--------------------------------------------------------------------------*/
void PegPresentationManager::SetFocus(PegThing *Start)
{
    if (Start)
    {
        PegMessage Mesg;
        Mesg.wType = PM_CURRENT;
        Start->Message(Mesg);
        Start->CheckSendSignal(PSF_FOCUS_RECEIVED);
    }
}


/*--------------------------------------------------------------------------*/
// This function changes which object has input focus. As part of this
// process, all window objects in the new focus tree are brought to
// the front.
/*--------------------------------------------------------------------------*/

void PegPresentationManager::MoveFocusTree(PegThing *Current)
{
    //if (muPointerCaptures || !Current->StatusIs(PSF_VISIBLE) ||
    if (!Current->StatusIs(PSF_VISIBLE) ||
        !Current->StatusIs(PSF_ACCEPTS_FOCUS))
    {
        return;
    }
    LOCK_PEG

    if (mpInputThing && mpInputThing != Current)
    {
        mpInputThing->CheckSendSignal(PSF_FOCUS_LOST);
    }
    mpInputThing = Current;

    // go up the tree to find the nearest parent with focus:

    if (Current == this)
    {
        if (First())
        {
            KillFocus(First());
        }
        UNLOCK_PEG
        return;
    }
    PegThing *NearParent = Current;

    while(!(NearParent->StatusIs(PSF_CURRENT)))
    {
        NearParent = NearParent->Parent();
    }

    // kill the old currency tree:

    KillFocus(NearParent->First());

    // Sometimes the new current object gets removed by killing focus on
    // the old tree. In that case, set focus to the near parent:

    if (!(Current->StatusIs(PSF_VISIBLE)) || Current->StatusIs(PSF_CURRENT))
    {
        UNLOCK_PEG
        return;
    }

    // put the new current object on top:

    PegThing *NewCurrent = Current;
    PegThing *pSib;
    PegThing *pOverlapper;

    while(Current != NearParent)
    {
        if (Current->Type() >= TYPE_WINDOW)
        {
            // This version makes a compromise. If no windows
            // cover the current window, do nothing. If one window covers
            // the new top window, just draw that part of the current
            // window that was covered. If more than one window overlaps
            // the new top window, draw the whole window.

            pSib = Current->Parent()->First();
            SIGNED iCount = 0; 
            pOverlapper = NULL;

            while (pSib != Current)
            {
                if (pSib->mReal.Overlap(Current->mReal))
                {
                    iCount++;
                    if (iCount > 1)
                    {
                        break;
                    }
                    pOverlapper = pSib;
                }
                pSib = pSib->Next();
            }

            if (iCount <= 1)
            {
                Current->Parent()->Add(Current, FALSE);

                if (iCount)
                {
                    PegRect Invalid = pOverlapper->mReal;
                    Invalid &= Current->mReal;
                    Invalidate(Invalid);
                    Current->Draw();
                }
            }
            else
            {
                Current->Parent()->Add(Current);
            }
        }
        Current = Current->Parent();
    }

    // now tell the new current object, and all of its parents, that it is current:

    SetFocus(NewCurrent);
    UNLOCK_PEG
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::InsureBranchHasFocus(PegThing *Current)
{
    PegThing *pParent = Current->Parent();

    while(pParent)
    {
        if (pParent->StatusIs(PSF_CURRENT))
        {
            return;
        }
        if (pParent->StatusIs(PSF_ACCEPTS_FOCUS))
        {
            MoveFocusTree(pParent);
        }
        pParent = pParent->Parent();
    }
}

#ifdef PEG_MULTITHREAD

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// the following group of functions are only provided if PEG_MULTITHREAD
// is defined.
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------*/
BOOL PegPresentationManager::RouteMessageToTask(PegMessage *pMesg)
{
    PegThing *pTarget = pMesg->pTarget;
    //void *pQueue;
    PEG_QUEUE_TYPE pQueue;

    if (!pTarget)      // no target for this message?
    {
        switch(pMesg->wType)
        {
        case PM_LBUTTONDOWN:
        case PM_LBUTTONUP:
        case PM_RBUTTONDOWN:
        case PM_RBUTTONUP:
        case PM_POINTER_MOVE:
            if (muPointerCaptures)
            {
                pTarget = mpPointerOwners[muPointerCaptures - 1];
            }
            else
            {
                pTarget = FindLowestThingContaining(this, pMesg->Point);
            }
            pQueue = GetThingMessageQueue(pTarget);
            if (pQueue)
            {
                MessageQueue()->Fold(pMesg, pQueue);
                return TRUE;
            }
            break;

        case PM_KEY:
        case PM_KEY_RELEASE:
            pTarget = mpInputThing;
            break;

        default:
            break;
        }
    }

    if (!pTarget)
    {
        return FALSE;
    }

    pQueue = GetThingMessageQueue(pTarget);

    if (pQueue)
    {
        ENQUEUE_TASK_MESSAGE(pMesg, pQueue);
        return TRUE;
    }

    return FALSE;
}


/*--------------------------------------------------------------------------*/
PEG_QUEUE_TYPE PegPresentationManager::GetThingMessageQueue(PegThing* pTarget)
{
    if (pTarget == this)
    {
        return((PEG_QUEUE_TYPE)NULL);
    }

    // find the top level thing that is the parent of the target:

    while(pTarget && pTarget->Parent() != this)
    {
        pTarget = pTarget->Parent();
    }

    if (!pTarget)           // shouldn't happen
    {
		return((PEG_QUEUE_TYPE)NULL);
    }

    // find out which task owns this top level thing:

    PegTaskInfo *pInfo = mpTaskInfo;

    while(pInfo)
    {
        if (pInfo->pThing == pTarget)
        {
            return(pInfo->pQueue);
        }
        pInfo = pInfo->pNext;
    }
    return((PEG_QUEUE_TYPE)NULL);
}

/*--------------------------------------------------------------------------*/
PEG_QUEUE_TYPE PegPresentationManager::GetCurrentMessageQueue(void)
{
    PegTaskInfo *pInfo = mpTaskInfo;

    while(pInfo)
    {
        if (pInfo->pTask == CURRENT_TASK)
        {
            return(pInfo->pQueue);
        }
        pInfo = pInfo->pNext;
    }
    return((PEG_QUEUE_TYPE)NULL);
}


/*--------------------------------------------------------------------------*/
void PegPresentationManager::BeginSubTaskExecute(PegThing *pWin)
{
    // see if this task already has an entry in the TaskInfo list:
    LOCK_PEG
    PegTaskInfo *pInfo = mpTaskInfo;
    PEG_QUEUE_TYPE pQueue = (PEG_QUEUE_TYPE)NULL;

    while(pInfo)
    {
        if (pInfo->pTask == CURRENT_TASK)
        {
            pQueue = pInfo->pQueue;
            break;
        }
        pInfo = pInfo->pNext;
    }

    // not in list, add a new list entry:

    PegTaskInfo *pNewInfo = new PegTaskInfo;
    pNewInfo->pNext = mpTaskInfo;
    pNewInfo->pThing = pWin;
    pNewInfo->pTask = CURRENT_TASK;

    if (!pQueue)
    {
        pNewInfo->pQueue = CREATE_MESG_QUEUE;
    }
    else
    {
        pNewInfo->pQueue = pQueue;
    }
    mpTaskInfo = pNewInfo;
    //Add(pWin);
    UNLOCK_PEG
}

/*--------------------------------------------------------------------------*/
void PegPresentationManager::EndSubTaskExecute(PegThing *What)
{
    // see if we need to clean up a TaskInfo:

    LOCK_PEG

    PegTaskInfo *pInfo = mpTaskInfo;
    PegTaskInfo *pSearch = mpTaskInfo;

    while(pInfo)
    {
        if (pInfo->pThing == What)
        {
            // yup, see if we are done with this task's message queue:

            while(pSearch)
            {
                if (pSearch != pInfo &&
                    pSearch->pQueue == pInfo->pQueue)
                {
                    break;
                }
                pSearch = pSearch->pNext;
            }

            if (!pSearch)       // no other entries using same queue?
            {
                DELETE_MESSAGE_QUEUE(pInfo->pQueue);
            }

            // remove this item from the list:

            if (pInfo == mpTaskInfo)
            {
                mpTaskInfo = pInfo->pNext;
            }
            else
            {
                pSearch = mpTaskInfo;
                while(pSearch->pNext != pInfo)
                {
                    pSearch = pSearch->pNext;
                }
                pSearch->pNext = pInfo->pNext;
            }
            delete pInfo;
            break;
        }
        pInfo = pInfo->pNext;
    }
    UNLOCK_PEG
}

#endif


/*--------------------------------------------------------------------------*/
SIGNED PegPresentationManager::Execute(void)
{
    PegMessage pSend;
    SIGNED iStatus;

    while(1)
    {
        MessageQueue()->Pop(&pSend);

       #ifdef PEG_MULTITHREAD

        if (!RouteMessageToTask(&pSend))
        {
            iStatus = DispatchMessage(this, &pSend);
            if (iStatus == PM_EXIT)
            {
                return iStatus;
            }
        }

       #else

        iStatus = DispatchMessage(this, &pSend);

        if (iStatus == PM_EXIT)
        {
            return iStatus;
        }

       #endif
    }
}


/*--------------------------------------------------------------------------*/
SIGNED PegPresentationManager::DispatchMessage(PegThing *From, PegMessage *pSend)
{
    PegThing *Current;

   #ifdef PEG_TOUCH_SUPPORT
    WORD wSaveType;
   #endif

    if (pSend->pTarget)
    {
        return(pSend->pTarget->Message(*pSend));
    }
    else
    {
        switch(pSend->wType)
        {
        case PM_LBUTTONDOWN:
        case PM_RBUTTONDOWN:

            if (muPointerCaptures)
            {
                return(mpPointerOwners[muPointerCaptures - 1]->Message(*pSend));
            }

            // figure out which thing it should go to based on
            // the position:

            Current = FindLowestThingContaining(From, pSend->Point);

            #if defined(PEG_TOUCH_SUPPORT)

            // For touch screens, we may not get PM_POINTER_MOVE messages.
            // In that case, make sure objects get PM_POINTER_EXIT and
            // PM_POINTER_ENTER messages.

            if (Current != mpLastPointerOver)
            {
                if (mpLastPointerOver)
                {
                    mpLastPointerOver->Message(PegMessage(PM_POINTER_EXIT));
                }
                Current->Message(PegMessage(PM_POINTER_ENTER));
            }
            #endif
           
            mpLastPointerOver = Current;

            if (Current != mpInputThing)
            {
                if (Current->StatusIs(PSF_ACCEPTS_FOCUS))
                {
                    MoveFocusTree(Current);
                }
                else
                {
                    // make sure the nearest window parent has
                    // focus:

                    InsureBranchHasFocus(Current);
                }
            }
            return(Current->Message(*pSend));

        case PM_LBUTTONUP:
        case PM_RBUTTONUP:
            if (muPointerCaptures)
            {
               #if defined(PEG_TOUCH_SUPPORT)

                // Fill in any missing PM_POINTER_MOVE messages:

                wSaveType = pSend->wType;
                pSend->wType = PM_POINTER_MOVE;
                mpPointerOwners[muPointerCaptures - 1]->Message(*pSend);
                pSend->wType = wSaveType;

               #endif

                return(mpPointerOwners[muPointerCaptures - 1]->Message(*pSend));
            }

            if (mpLastPointerOver)
            {
                if (mpLastPointerOver->mReal.Contains(pSend->Point))
                {
                   #if defined(PEG_TOUCH_SUPPORT)

                    // Fill in any missing PM_POINTER_MOVE messages:

                    wSaveType = pSend->wType;
                    pSend->wType = PM_POINTER_MOVE;
                    mpLastPointerOver->Message(*pSend);
                    pSend->wType = wSaveType;

                   #endif

                    return(mpLastPointerOver->Message(*pSend));
                }
               #if defined(PEG_TOUCH_SUPPORT)
                else
                {
                    mpLastPointerOver->Message(PegMessage(PM_POINTER_EXIT));
                    Current = FindLowestThingContaining(From, pSend->Point);
                    mpLastPointerOver = Current;
                    mpLastPointerOver->Message(PegMessage(PM_POINTER_ENTER));
                }
               #endif
            }
            break;

        case PM_POINTER_MOVE:
           #if defined(PEG_MOUSE_SUPPORT)
            Screen()->SetPointer(pSend->Point);
           #endif

            if (muPointerCaptures)
            {
                return(mpPointerOwners[muPointerCaptures - 1]->Message(*pSend));
            }

            Current = FindLowestThingContaining(From, pSend->Point);

            if (Current != mpLastPointerOver)
            {
                if (mpLastPointerOver)
                {
                    mpLastPointerOver->Message(PegMessage(PM_POINTER_EXIT));
                }
                mpLastPointerOver = Current;
                mpLastPointerOver->Message(PegMessage(PM_POINTER_ENTER));
            }
            else
            {
                if (Current != this)
                {
                    return(Current->Message(*pSend));
                }
            }
            break;

        case PM_KEY:
        case PM_KEY_RELEASE:
        case PM_CUT:
        case PM_COPY:
        case PM_PASTE:
            
            // even if PEG_KEYBOARD_SUPPORT is not defined, we leave basic
            // PM_KEY handling. This allow the application level software
            // to create and send PM_KEY messages.

            if (mpInputThing)
            {
                return(mpInputThing->Message(*pSend));
            }
            break;

        default:
            if (pSend->wType < FIRST_USER_MESSAGE)
            {
                // by default, system messages go to the top window:

                return(First()->Message(*pSend));
            }
            else
            {
                // for user messages with a NULL pTarget, the message
                // iData field should contain the target object ID. Look
                // for this object:

                if (pSend->iData)
                {
                    PegThing *pTarget = Find(pSend->iData);

                    if (pTarget)
                    {
                        return(pTarget->Message(*pSend));
                    }
                    return 0;
                }
            }
        }
    }
    return 0;
}

