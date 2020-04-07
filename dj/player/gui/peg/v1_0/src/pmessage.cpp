/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pmessage.cpp - Peg message queue implementation.
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
// PegMessageQueue supports the posting and retrieving of messages.
// PegMessageQueue also provides timer facilities for application objects.
//
// Alternate forms of most of the PegMessageQueue functions are provided
// as part of the RTOS integration. This generic version creates a queue of
// free messages, and allocates from the free queue to pass messages to
// the input queue. This provides much lower overhead than dynamically
// allocating messages as they are needed, but also means that there are
// a limited number of free messages available. 
//
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#include <gui/peg/peg.hpp>
#include <cyg/infra/diag.h>

/*--------------------------------------------------------------------------*/
// The PEG/RTOS integrations do not use default message queue, they
// replace these functions with versions custom to the RTOS. Therefore,
// we do not compile in most of this file unless we are running
// standalone.
/*--------------------------------------------------------------------------*/

#if !defined(PEGSMX) && !defined(PEGX) && !defined(PHARLAP)
#if !defined(PEGRTXC) && !defined(PEGSUPERTASK) && !defined(PEGTRONTASK)
#if !defined(PEGNUCLEUS) && !defined(PEGMQX) && !defined(PEGX11)
#if !defined(LINUXPEG) && !defined(PEG_OSE) && !defined(LYNXPEG)

void PegIdleFunction(void);

/*--------------------------------------------------------------------------*/
PegMessageQueue::PegMessageQueue(void)
{
    WORD wLoop;

    mpFirst = NULL;
    mpLast = NULL;
    mpFree = new PegMessage();
    PegMessage *Current = mpFree;

    for (wLoop = 0; wLoop < NUM_PEG_FREE_MSGS; wLoop++)
    {
        Current->Next = new PegMessage();
        Current = Current->Next;
    }

    mpFreeEnd = Current;
    mpTimerList = NULL;
}

/*--------------------------------------------------------------------------*/
PegMessageQueue::~PegMessageQueue()
{
    PegMessage *pTemp;

    while (mpFirst)
    {
        pTemp = mpFirst->Next;
        delete mpFirst;
        mpFirst = pTemp;
    }

    while(mpFree)
    {
        pTemp = mpFree->Next;
        delete mpFree;
        mpFree = pTemp;
    }

    while(mpTimerList)
    {
        PegTimer *pTimer = mpTimerList->pNext;
        delete mpTimerList;
        mpTimerList = pTimer;
    }
}

int lTargMesg;
/*--------------------------------------------------------------------------*/
void PegMessageQueue::Push(PegMessage *In)
{
    if (In->pTarget)
    {
        lTargMesg++;
    }

    if (mpFree)
    {
        PegMessage *Current = mpFree;
        mpFree = mpFree->Next;

        if (!mpFree)
        {
            mpFreeEnd = NULL;
        }
        *Current = *In;
        Current->Next = NULL;

        if (mpLast)
        {
            mpLast->Next = Current;
        }
        else
        {
            mpFirst = Current;
        }
        mpLast = Current;
    }
    else
    {
        // we dropped a message.  maybe we should shout about this?
        diag_printf("\n\n ****** PegMessageQueue::Push dropped a message ******\n\n\n");
    }
}

/*--------------------------------------------------------------------------*/
void PegMessageQueue::Pop(PegMessage *Put)
{
    while(1)
    {
        if (mpFirst)
        {
            *Put = *mpFirst;
            Put->Next = NULL;
            if (mpFreeEnd)
            {
                mpFreeEnd->Next = mpFirst;
            }
            else
            {
                mpFree = mpFirst;
            }        

            mpFreeEnd = mpFirst;
            mpFirst = mpFirst->Next;
            mpFreeEnd->Next = NULL;

            if (!mpFirst)
            {
                mpLast = NULL;
            }
            return;
        }
        else
        {
            PegIdleFunction();
        }
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegMessageQueue::Fold(PegMessage *In)
{
    // see if this message already exists:

    PegMessage *pTest = mpFirst;

    while(pTest)
    {
        if (pTest->wType == In->wType &&
            pTest->pTarget == In->pTarget &&
            pTest->iData == In->iData)
        {
            // these messages match, fold the old one into the new one:

            pTest->Rect = In->Rect;
            return;
        }
        pTest = pTest->Next;
    }
    Push(In);
}


/*--------------------------------------------------------------------------*/
// Purge: Removes any messages from the message queue which are targeted
//     for a deleted thing.
/*--------------------------------------------------------------------------*/
void PegMessageQueue::Purge(PegThing *Del)
{
    PegMessage *pMesg = mpFirst;
    PegMessage *pLast = NULL;
    PegMessage *pNext;

    while(pMesg)
    {
        pNext = pMesg->Next;

        if (pMesg->pTarget == Del)
        {
            if (pLast)
            {
                pLast->Next = pNext;

				if (!pNext)             // purging last message? 
				{
					mpLast = pLast;     // back up last message pointer
				}
            }
            else
            {
                mpFirst = pNext;
                if (!mpFirst)
                {
                    mpLast = NULL;
                }
            }
            if (mpFreeEnd)
            {
                mpFreeEnd->Next = pMesg;
            }
            else
            {
                mpFree = pMesg;
            }        
            mpFreeEnd = pMesg;
        }
        else
        {
            pLast = pMesg;
        }
        pMesg = pNext;
    }
}

#endif      // end fourth line of integration tests
#endif      // end third line of integration tests
#endif      // end second line of integration tests
#endif      // end first line of integration tests


/*--------------------------------------------------------------------------*/
void PegMessageQueue::SetTimer(PegThing *Who, WORD wId,
    LONG lCount, LONG lReset)
{
    LOCK_TIMER_LIST
    // see if this is a 'Reset'

    PegTimer *pTimer = mpTimerList;

    while(pTimer)           
    {
        if (pTimer->pTarget == Who && pTimer->wTimerId == wId)
        {
            pTimer->lCount = lCount;
            pTimer->lReset = lReset;
            UNLOCK_TIMER_LIST
            return;
        }
        pTimer = pTimer->pNext;
    }

    mpTimerList = new PegTimer(mpTimerList, Who, wId, lCount, lReset);
    UNLOCK_TIMER_LIST
}

/*--------------------------------------------------------------------------*/
void PegMessageQueue::KillTimer(PegThing *Who, WORD wId)
{
    LOCK_TIMER_LIST
    PegTimer *pTimer = mpTimerList;
    PegTimer *pPrevious = pTimer;

    while(pTimer)
    {
        if (pTimer->pTarget == Who)
        {
            if (!wId || pTimer->wTimerId == wId)
            {
                if (pTimer == mpTimerList)
                {
                    mpTimerList = pTimer->pNext;
                }
                else
                {
                    pPrevious->pNext = pTimer->pNext;
                }
                delete pTimer;
                UNLOCK_TIMER_LIST
                return;
            }
        }
        pPrevious = pTimer;
        pTimer = pTimer->pNext;
    }
    UNLOCK_TIMER_LIST
}


/*--------------------------------------------------------------------------*/
void PegMessageQueue::TimerTick(void)
{
    // check to see if anyone is due a timer message:

    LOCK_TIMER_LIST

    PegTimer *pTimer = mpTimerList;
    PegTimer *pPrevious = pTimer;
    PegMessage NewMessage;

    while(pTimer)
    {
        pTimer->lCount--;

        if (pTimer->lCount == 0)
        {
            // send a message to this guy:

            NewMessage.wType = PM_TIMER;
            NewMessage.pTarget = pTimer->pTarget;
            NewMessage.iData = pTimer->wTimerId;
            Fold(&NewMessage);

            if (pTimer->lReset > 0)
            {
                pTimer->lCount = pTimer->lReset;
                pTimer = pTimer->pNext;
            }
            else
            {
                if (pTimer == mpTimerList)
                {
                    mpTimerList = pTimer->pNext;
                    delete pTimer;
                    pTimer = mpTimerList;
                    
                }
                else
                {
                    pPrevious->pNext = pTimer->pNext;
                    delete pTimer;
                    pTimer = pPrevious->pNext;
                }
            }
        }
        else
        {
            pPrevious = pTimer;
            pTimer = pTimer->pNext;
        }
    }
    UNLOCK_TIMER_LIST
}


