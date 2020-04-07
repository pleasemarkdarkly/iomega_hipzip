/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pmessage.hpp - PegMessage and PegMessageQueue type definition.
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

#ifndef _PEGMESSAGE_
#define _PEGMESSAGE_

/*--------------------------------------------------------------------------*/
// Standard PEG generated message types:

enum PegSystemMessage {
    PM_ADD = 1,
    PM_REMOVETHING,
    PM_DESTROY,
    PM_SIZE,
    PM_MOVE,
    PM_CLOSE,
    PM_HIDE,
    PM_SHOW,
    PM_POINTER_MOVE,
    PM_LBUTTONDOWN,
    PM_LBUTTONUP,
    PM_RBUTTONDOWN,
    PM_RBUTTONUP,
    PM_DRAW,
    PM_CURRENT,
    PM_NONCURRENT,
    PM_POINTER_ENTER,
    PM_POINTER_EXIT,
    PM_EXIT,
    PM_ADDICON,
    PM_BEGIN_MOVE,
    PM_PARENTSIZED,
    PM_VSCROLL,
    PM_HSCROLL,
    PM_MAXIMIZE,
    PM_MINIMIZE,
    PM_RESTORE,
    PM_CLOSE_SIBLINGS,
    PM_TIMER,
    PM_KEY,
    PM_KEY_HOLD,
    PM_KEY_RELEASE,
    PM_CUT,
    PM_COPY,
    PM_PASTE,
    PM_SLIDER_DRAG,
    PM_MWCOMPLETE,
    PM_DIALOG_NOTIFY,
    PM_DIALOG_APPLY,
    PM_MOVE_FOCUS

};

const WORD FIRST_SIGNAL        = 128;
const WORD FIRST_USER_MESSAGE    = 0x4000;

class PegThing;             // forward reference

/*--------------------------------------------------------------------------*/
// PegMessage definition:
// New fields may be added as needed by the user.


struct PegMessage
{
	public:
    PegMessage() {Next = NULL; pTarget = NULL; pSource = NULL;}
    PegMessage(WORD wVal) {Next = NULL; pTarget = NULL; pSource = NULL; wType = wVal;}
    PegMessage(PegThing *pTo, WORD wVal) {Next = NULL; pTarget = pTo; pSource = NULL; wType = wVal;}
    WORD wType;
    SIGNED iData;
    PegThing *pTarget;
    PegThing *pSource;
    PegMessage *Next;
 
    union
    {
        void *pData;
        LONG lData;
        PegRect Rect;
        PegPoint Point;
        LONG    lUserData[2];
        DWORD   dUserData[2];
        SIGNED  iUserData[4];
        WORD    wUserData[4];
        UCHAR   uUserData[8];
    };
};


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// PegMessageQueue definition:
// New fields may be added as needed by the user.

class PegMessageQueue
{
    struct PegTimer
    {
        PegTimer() {pNext = NULL; pTarget = NULL;}
        PegTimer(LONG lCnt, LONG lRes)
        {
            pNext = NULL;
            pTarget = NULL;
            lCount = lCnt;
            lReset = lRes;
        }
        PegTimer(PegTimer *Next, PegThing *Who, WORD wId, LONG lCnt, LONG lRes)
        {
            pNext = Next;
            pTarget = Who;
            lCount = lCnt;
            lReset = lRes;
            wTimerId = wId;
        }
    
        PegTimer *pNext;
        PegThing *pTarget;
        LONG    lCount;
        LONG    lReset;
        WORD    wTimerId;
    };

    public:

            PegMessageQueue(void);
            ~PegMessageQueue();
            void Push(PegMessage *);
            void Push(PegMessage &In) {Push(&In);}
            void Pop(PegMessage *);
            void Fold(PegMessage *In);
           #ifdef PEG_MULTITHREAD
            void Fold(PegMessage *In, PEG_QUEUE_TYPE pQueue);
           #endif
            void Purge(PegThing *);
            void SetTimer(PegThing *Who, WORD wId, LONG lCount, LONG lReset);
            void KillTimer(PegThing *Who, WORD wId);
            void TimerTick();

    private:

       #if !defined(PEGSMX) && !defined(PEGX) && !defined(PHARLAP) && !defined(PEGRTXC) && !defined(PEGNUCLEUS) && !defined(PEGMQX) && !defined(PEGX11) && !defined(LINUXPEG)

        // The above RTOS integrations replace the standalone
        // PEG message queue implementation, so we don't need these
        // member variables.

        PegMessage *mpFirst;
        PegMessage *mpLast;
        PegMessage *mpFree;
        PegMessage *mpFreeEnd;
       #endif

        PegTimer *mpTimerList;

};

#endif






