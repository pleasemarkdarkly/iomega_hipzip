/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// ppresent.hpp - PegPresentationManager class defintion.
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
// 
/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/

#ifndef _PPRESENT_
#define _PPRESENT_

/*--------------------------------------------------------------------------*/
// The maximum number of things that will ever simultaneously try to capture
// the mouse pointer
/*--------------------------------------------------------------------------*/

#define MAX_POINTER_CAPTURE_NESTING 8

#ifdef PEG_MULTITHREAD

struct PegTaskInfo
{
    PegTaskInfo PEGFAR *pNext;
    PegThing PEGFAR *pThing;

	PEG_TASK_TYPE	pTask;
	PEG_QUEUE_TYPE	pQueue;
};

#endif

class PegPresentationManager : public PegWindow
{
    public:
        PegPresentationManager(PegRect &);
        ~PegPresentationManager();
        void Draw(void);
        SIGNED Message(const PegMessage &Mesg);
        void Add(PegThing *What, BOOL bDraw = TRUE);
        PegThing *Remove(PegThing *What, BOOL bDraw = TRUE);
        SIGNED Execute(void);
        virtual SIGNED DispatchMessage(PegThing *From, PegMessage *pSend);
        void MoveFocusTree(PegThing *Current);
        PegThing *FindLowestThingContaining(PegThing *Start,
            PegPoint Point);
        void NullInput(PegThing *Current);

        BOOL IsPointerCaptured(void) {
            if (muPointerCaptures)
            {
                return TRUE;
            }
            return FALSE;
        }

        PegThing *GetCurrentThing(void) {return mpInputThing;}
        PegThing *GetPointerOwner(void);

        void SetScratchPad(TCHAR *pText);
        TCHAR *GetScratchPad(void) {return mpScratchPad;}
        void ClearScratchPad(void);

       #ifdef PEG_BUILD_PRESC
        virtual void CapturePointer(PegThing *Who);
        virtual void ReleasePointer(PegThing *Who);
        virtual void SetWallpaper(PegBitmap *pBm) { mpWallpaper = pBm; }
       #else
        void CapturePointer(PegThing *Who);
        void ReleasePointer(PegThing *Who);
        void SetWallpaper(PegBitmap *pBm) { mpWallpaper = pBm;}
       #endif

        PegThing *LastPointerOver(void) {return mpLastPointerOver;}
        void LastPointerOver(PegThing *pOver) {mpLastPointerOver = pOver;}

       #ifdef PEG_MULTITHREAD
        BOOL RouteMessageToTask(PegMessage *pMesg);
        void BeginSubTaskExecute(PegThing *pWin);
        void EndSubTaskExecute(PegThing *pWin);
		PEG_QUEUE_TYPE GetThingMessageQueue(PegThing* pTarget);
		PEG_QUEUE_TYPE GetCurrentMessageQueue(void);
        PegTaskInfo *GetTaskInfo(void) {return mpTaskInfo;}
       #endif 

    protected:
        
        void KillFocus(PegThing *Start);
        void SetFocus(PegThing *Start);
        void InsureBranchHasFocus(PegThing *Current);
        
       #ifdef PEG_MULTITHREAD
        PegTaskInfo *mpTaskInfo;
       #endif 

        PegThing *mpInputThing;
        PegThing *mpDefaultInputThing;
        PegThing *mpLastPointerOver;
        PegThing *mpPointerOwners[MAX_POINTER_CAPTURE_NESTING];
        PegBitmap *mpWallpaper;
        TCHAR *mpScratchPad;
        UCHAR muPointerCaptures;
        BOOL mbMoveFocusFlag;
};


#endif

