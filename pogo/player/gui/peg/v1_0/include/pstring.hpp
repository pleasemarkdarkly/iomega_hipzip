/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// pstring.hpp - PegString class definition.
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

#ifndef _PEGSTRING_
#define _PEGSTRING_

class PegString : public PegThing, public PegTextThing
{
    public:
        PegString(const PegRect &Rect, const TCHAR *Text = NULL,
            WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
            SIGNED iLen = -1);
        
        PegString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth,
            const TCHAR *Text = NULL, WORD wId = 0, 
            WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT, SIGNED iLen = -1);

        PegString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text = NULL,
            WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
            SIGNED iLen = -1);

        virtual ~PegString();

        virtual SIGNED Message(const PegMessage &Mesg);

        virtual void Draw(void);
        virtual void DataSet(const TCHAR *Text);
        virtual void SetFont(PegFont *Font);
        virtual void Style(WORD wStyle);
        virtual WORD Style(void) {return PegThing::Style();}
        
        virtual void SetMark(SIGNED iStart, SIGNED iEnd);
        virtual void SetMark(TCHAR *MarkStart, TCHAR *MarkEnd);
        virtual SIGNED GetMarkStart(void) {return miMarkStart;}
        virtual SIGNED GetMarkEnd(void) {return miMarkEnd;}
        void DeleteMarkedText(void);
        void CopyToScratchPad(void);
        void PasteFromScratchPad(void);


    protected:
        virtual void RetardCursor(SIGNED iNew);
        virtual void AdvanceCursor(SIGNED iNew);
        virtual void DrawMarked(void);
        virtual BOOL InsertKey(SIGNED iKey);
        virtual void SetCursorPos(PegPoint PickPoint);

        void ReplaceMarkedText(SIGNED iKey);
        void ExitEditMode(void);

       #ifdef PEG_KEYBOARD_SUPPORT
        BOOL CheckControlKey(SIGNED iKey, SIGNED iFlags);
        TCHAR *mpBackup;
       #endif

        SIGNED miCursor;
        SIGNED miMarkAnchor;
        SIGNED miMarkStart;
        SIGNED miMarkEnd;
        SIGNED miFirstVisibleChar;
        SIGNED miMaxLen;

        PegPoint mCursorPos;

        union
        {
            struct {
            SIGNED mbEditMode:1;
            SIGNED mbMarked:1;
            SIGNED mbFullSelect:1;
            SIGNED mbMarkMode:1;
            SIGNED mbChanged:1;
            } State;

            SIGNED mwFullState;
        };
};


#endif

