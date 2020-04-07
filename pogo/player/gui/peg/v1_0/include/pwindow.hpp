/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pwindow.hpp - Basic window class definition.
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

#ifndef _PEGWINDOW_
#define _PEGWINDOW_


class PegDecoratedWindow;    // forward reference

/*--------------------------------------------------------------------------*/
// Move Types. A re-size is treated by Peg as simply a different type
//  of move.

#define PMM_MOVEALL         0x80
#define PMM_MOVETOP         0x01
#define PMM_MOVEBOTTOM      0x02
#define PMM_MOVELEFT        0x04 
#define PMM_MOVERIGHT       0x08
#define PMM_MOVEUR          0x09
#define PMM_MOVELR          0x0a
#define PMM_MOVEUL          0x05
#define PMM_MOVELL          0x06

/*--------------------------------------------------------------------------*/
// Border Width in pixels for PegBorder Objects
// Note: Value must be >= 3 for correct operation
/*--------------------------------------------------------------------------*/
#define PEG_FRAME_WIDTH 5
#define PEG_SCROLL_WIDTH 16     // default scroll bar width

/*--------------------------------------------------------------------------*/

struct PegScrollInfo 
{
    SIGNED wMin;
    SIGNED wMax;
    SIGNED wCurrent;
    SIGNED wStep;
    SIGNED wVisible;
};

class PegWindow : public PegThing
{
    public:
        PegWindow(const PegRect &Rect, WORD wStyle = FF_THICK);
        PegWindow(WORD wStyle = FF_THICK);
        virtual ~PegWindow();
        virtual void Draw(void);
        virtual SIGNED Message(const PegMessage &Mesg);
        virtual SIGNED Execute(void);

       #ifdef PEG_MULTITHREAD
        SIGNED GlobalModalExecute(void);
       #endif

        virtual void InitClient(void);
        virtual void Add(PegThing *Who, BOOL bDraw = TRUE);
        virtual void AddIcon(PegIcon *pIcon);
        virtual void SetScrollMode(UCHAR uMode);
        virtual UCHAR GetScrollMode(void){return muScrollMode;}
        virtual void GetVScrollInfo(PegScrollInfo *Put);
        virtual void GetHScrollInfo(PegScrollInfo *Put);
        virtual void DrawFrame(BOOL bFill = TRUE);
        virtual PegBitmap *GetIcon(void) {return mpIconMap;}
        virtual void SetIcon(PegBitmap *nb) {mpIconMap = nb;}
        virtual void Resize(PegRect NewSize);
        virtual void MoveFocusToFirstClientChild(void);
        BOOL IsMaximized(void) {return mbMaximized;}
        BOOL IsModal(void) {return mbModal;}
        UCHAR CurrentMoveMode(void) {return muMoveMode;}
        BOOL CheckAutoScroll(void);

    protected:
        virtual UCHAR BorderContains(PegPoint Point);
        virtual void MoveClientObjects(SIGNED xShift, SIGNED yShift, BOOL bDraw = TRUE);
        void DrawMoveFrame(BOOL Erase);
        void FillScrollCorner(void);
        void CheckResizeRect(PegRect &Rect);

        PegBitmap *mpIconMap;
        BOOL mbModal;
        BOOL mbMaximized;
        BOOL mbMoveFrame;
        UCHAR muScrollMode;
        UCHAR muMoveMode;
        UCHAR mbShowPointer;
        PegPoint mStartMove;
        PegPoint mMovePoint;
        PegRect mRestore;

};


#endif

