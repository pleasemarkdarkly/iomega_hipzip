/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// ptextbox.hpp - Text box class definition.
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

#ifndef _PEGTEXTBOX_
#define _PEGTEXTBOX_

#define MAX_LINE_OFFSETS 100

class PegTextBox : public PegWindow, public PegTextThing
{
    public:
        PegTextBox(const PegRect &Rect, WORD wId = 0,
                   WORD wStyle = FF_RECESSED|EF_WRAP|TJ_LEFT,
                   TCHAR *Text = NULL, WORD wMaxChars = 1000);

        virtual ~PegTextBox();
        virtual void Draw(void);
        virtual SIGNED Message(const PegMessage &Mesg);
        virtual void GetVScrollInfo(PegScrollInfo *Put);
        virtual void GetHScrollInfo(PegScrollInfo *Put);
        virtual void DataSet(const TCHAR *Text);
        virtual void Append(const TCHAR *pText, BOOL bDraw = TRUE);
        virtual void SetFont(PegFont *Font);
        virtual void Resize(PegRect NewSize);

        void RewindDataSet(const TCHAR *Text);
        SIGNED LineCount(void) {return miTotalLines;}
        WORD GetTopLine(void) {return miTopLine;}
        void SetTopLineToEnd(void);
        void SetTopLine(SIGNED iLine);
        TCHAR *GetLineStart(SIGNED iLine, SIGNED *pPutLen);
        WORD GetLineIndex(SIGNED iLine);
        SIGNED GetWidestLine(void) {return miWidestLine;}
        void CheckBufLen(SIGNED iLen);

        WORD GetMaxChars(void) {return mwMaxChars;}
        void SetMaxChars(WORD wMax) {mwMaxChars = wMax;}

        TCHAR *FindLinePointer(SIGNED iLine);
        BOOL LineDown(void);
        BOOL LineUp(void);

        void MarkLine(SIGNED iMarkLine) {miMarkLine = iMarkLine;}
        //void VScrollText(SIGNED iLine);
        //void HScrollText(SIGNED iOffset);

    protected:

        void ClipToClient(void);
        void EndClipToClient(void);

        void UpdateLineStarts(void);
        void SlideStartWindowUp(SIGNED iCount);
        void SlideStartWindowDown(SIGNED iCount);

        void FillLineStarts(SIGNED iStartLine, BOOL bFull);
        
        TCHAR *FindNextLine(TCHAR *Text, SIGNED sMaxWidth, SIGNED iDir = 1);
        void DrawAllText(void);
        virtual void DrawTextLine(SIGNED iLine, PegPoint PutPoint,
            BOOL bFill = FALSE);
        void DrawTextLine(SIGNED iLine);

        TCHAR *mpBuf;
        PegRect mOldClip;
        SIGNED miClipCount;

        SIGNED miWidestLine;
        SIGNED miLeftOffset;
        SIGNED miTotalLines;
        SIGNED miVisible;
        SIGNED miLineStartTop;
        SIGNED miLineStartEnd;
        SIGNED miLineHeight;
        SIGNED miBufLen;
        
        WORD   mwMaxChars;
        WORD *mwLineStarts;
        SIGNED miTopLine;
        SIGNED miMarkLine;
};


#endif

