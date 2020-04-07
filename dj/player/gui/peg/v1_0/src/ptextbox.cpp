/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
// ptextbox.cpp - Text box class implementation.
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

#include "stdlib.h"     // ansi
#include "string.h"     // ansi
#include "ctype.h"      // ansi

#include <gui/peg/peg.hpp>

extern TCHAR lsTEST[];

/*--------------------------------------------------------------------------*/
PegTextBox::PegTextBox(const PegRect &Rect, WORD wId, WORD wStyle, TCHAR *Text,
    WORD wMaxChars) : PegWindow(Rect, wStyle),
    PegTextThing(Text, wStyle & (EF_EDIT|TT_COPY), PEG_TEXTBOX_FONT)
{
    Id(wId);
    muColors[PCI_NORMAL] = PCLR_CLIENT;
    muColors[PCI_SELECTED] = PCLR_HIGH_TEXT_BACK;
    muColors[PCI_NTEXT] = PCLR_NORMAL_TEXT;
    muColors[PCI_STEXT] = PCLR_HIGH_TEXT;
    Type(TYPE_TEXTBOX);

    miTopLine = 0;
    miClipCount = 0;
    mwMaxChars = wMaxChars;
    miWidestLine = 0;
    miLeftOffset = 0;
    miMarkLine = -1;
    miLineHeight = TextHeight(lsTEST, mpFont);
    mpBuf = NULL;
    miBufLen = 0;

    if (wStyle & (EF_EDIT|AF_ENABLED))
    {
        AddStatus(PSF_TAB_STOP);
    }
    else
    {
        RemoveStatus(PSF_TAB_STOP|PSF_ACCEPTS_FOCUS);
    }
    
    miTotalLines = 0;
    
    // configure the start of line indexes:

    mwLineStarts = new WORD[MAX_LINE_OFFSETS];
    UpdateLineStarts();
}


/*--------------------------------------------------------------------------*/
// Destructor- Remove and delete any children.
/*--------------------------------------------------------------------------*/
PegTextBox::~PegTextBox()
{
    delete mwLineStarts;

    if (mpBuf)
    {
        delete mpBuf;
    }
}

/*--------------------------------------------------------------------------*/
SIGNED PegTextBox::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_VSCROLL:
        if (Mesg.lData != miTopLine)
        {
            Invalidate();
            SIGNED iCount = miTopLine - (SIGNED) Mesg.lData;
            SIGNED iShift = iCount * miLineHeight;
            MoveClientObjects(0, iShift, FALSE);
            miTopLine = (SIGNED) Mesg.lData;

            // If FAST_BLIT is not defined, just redraw the lines of text
            // in my client area:

           #ifndef FAST_BLIT
            Draw();
           #else

            // If we have scrolled more than the height of the client area,
            // just redraw myself

           #ifdef PEG_FULL_CLIPPING
            if (abs(iCount) >= miVisible || !ViewportList())
            {
                Draw();
                break;
            }

            if (!ViewportList()->mView.Contains(mClient))
            {
                Draw();
                break;
            }
           #else
            if (abs(iCount) >= miVisible)
            {
                Draw();
                break;
            }
           #endif

            // Here for scrolling just a line or so. To improve speed,
            // we use RectMove to move most of the text, and just draw
            // the newly exposed lines.

            SIGNED iLine;
            PegRect ShiftRect = mClient;
            ShiftRect -= 2;
            ShiftRect.wBottom = ShiftRect.wTop + (miVisible * miLineHeight) - 1;
            PegPoint Put;
            Put.x = ShiftRect.wLeft;
            ClipToClient();
            BeginDraw();

            // Scroll as much as possible using RectMove()
            // if iCount < 0, we are advancing miTopLine and the lines
            // need to move up. Otherwise the lines need to move down.


            if (iCount < 0)     // scroll lines up
            {
                Put.y = ShiftRect.wTop;
                ShiftRect.wTop -= iShift;
                Screen()->RectMove(this, ShiftRect, Put);
                iLine = miTopLine + miVisible + iCount;
                Put.y += (iLine - miTopLine) * miLineHeight;

                while (iLine < miTopLine + miVisible) 
                {
                    DrawTextLine(iLine, Put, TRUE);
                    Put.y += miLineHeight;
                    iLine++;
                }
            }
            else                // scroll lines down
            {
                ShiftRect.wBottom -= iShift;
                Put.y = ShiftRect.wTop + iShift;
                Screen()->RectMove(this, ShiftRect, Put);

                // Draw top line(s)
                Put.y = mClient.wTop + 2;
                iLine = miTopLine;

                while (iLine < miTopLine + iCount)
                {
                    DrawTextLine(iLine, Put, TRUE);
                    Put.y += miLineHeight;
                    iLine++;
                }
            }
            EndDraw();
            EndClipToClient();

           #endif
        }
        break;

    case PM_HSCROLL:
        if (Mesg.lData != miLeftOffset)
        {
            SIGNED iShift = miLeftOffset - (SIGNED) Mesg.lData;
            miLeftOffset = (SIGNED) Mesg.lData;

            // Let PegWindow do the shifting:

            ClipToClient();
            MoveClientObjects(iShift, 0);
            EndClipToClient();
        }
        break;


   #ifdef PEG_KEYBOARD_SUPPORT
    case PM_KEY:
        switch(Mesg.iData)
        {
        case PK_PGUP:
            SetTopLine(miTopLine - miVisible);
            break;

        case PK_PGDN:
            SetTopLine(miTopLine + miVisible);
            break;

        default:
            return PegWindow::Message(Mesg);
        }
        break;
   #endif

    default:
        return(PegWindow::Message(Mesg));
    }
    return 0;
}

/*--------------------------------------------------------------------------*/
void PegTextBox::Draw(void)
{
    BeginDraw();
    PegWindow::Draw();
    ClipToClient();
    DrawAllText();
    EndClipToClient();
    EndDraw();
}

/*--------------------------------------------------------------------------*/
void PegTextBox::Resize(PegRect NewSize)
{
    PegWindow::Resize(NewSize);

    UpdateLineStarts();

    if (muScrollMode & WSM_AUTOSCROLL)
    {
        if (CheckAutoScroll())
        {
            UpdateLineStarts();
        }
    }
}

/*--------------------------------------------------------------------------*/
void PegTextBox::SetFont(PegFont *Font)
{
    PegTextThing::SetFont(Font);
    miLineHeight = TextHeight(lsTEST, mpFont);
    UpdateLineStarts();

    if (muScrollMode & WSM_AUTOSCROLL)
    {
        if (CheckAutoScroll())
        {
            UpdateLineStarts();
        }
    }
    Invalidate();
}


/*--------------------------------------------------------------------------*/
void PegTextBox::RewindDataSet(const TCHAR *Text)
{
    miTopLine = 0;
    DataSet(Text);
}


/*--------------------------------------------------------------------------*/
void PegTextBox::SetTopLineToEnd(void)
{
    SetTopLine(miTotalLines);
}

/*--------------------------------------------------------------------------*/
void PegTextBox::SetTopLine(SIGNED iLine)
{
    miTopLine = iLine;

    if (miTopLine + miVisible >= miTotalLines)
    {
        //miTopLine = miTotalLines - miVisible + 1;
        miTopLine = miTotalLines - miVisible;
    }
    if (miTopLine < 0)
    {
        miTopLine = 0;
    }
    UpdateLineStarts();

    if (muScrollMode & WSM_AUTOSCROLL)
    {
        if (CheckAutoScroll())
        {
            UpdateLineStarts();
        }
    }

    if (StatusIs(PSF_VISIBLE))
    {
        Invalidate();
        Draw();
    }
}

/*--------------------------------------------------------------------------*/
void PegTextBox::DataSet(const TCHAR *Text)
{
    PegTextThing::DataSet(Text);

    UpdateLineStarts();

    if (miTopLine &&
        miTopLine + miVisible >= miTotalLines)
    {
        SetTopLineToEnd();
    }

    if (muScrollMode)
    {
        if (CheckAutoScroll())
        {
            UpdateLineStarts();
            CheckAutoScroll();
        }
    }
    Invalidate();
}


/*--------------------------------------------------------------------------*/
void PegTextBox::ClipToClient(void)
{
    if (!miClipCount)
    {
        mOldClip = mClip;
        PegRect TrueClient = mClient;
        TrueClient -= 2;
        mClip &= TrueClient;
    }
    miClipCount++;
}

/*--------------------------------------------------------------------------*/
void PegTextBox::EndClipToClient(void)
{
    miClipCount--;

    if (!miClipCount)
    {
        mClip = mOldClip;
    }
}

/*--------------------------------------------------------------------------*/
// DrawAllText: Called from Draw() to write every visible line of text.
/*--------------------------------------------------------------------------*/
void PegTextBox::DrawAllText(void)
{
    PegPoint PutPoint;
    PegRect LineRect;

    SIGNED iLine  = miTopLine;

    if (!DataGet())
    {
        return;
    }

    PutPoint.x = mClient.wLeft + 2 - miLeftOffset;
    PutPoint.y = mClient.wTop + 2;

    LineRect.wLeft = mClient.wLeft;
    LineRect.wRight = mClient.wRight;
    LineRect.wTop = PutPoint.y;
    LineRect.wBottom = LineRect.wTop + miLineHeight - 1;

    SIGNED iStopy = mClient.wBottom - 2;
    iStopy -= miLineHeight - 1;

    ClipToClient();

    while(PutPoint.y <= iStopy)
    {
        if (Screen()->InvalidOverlap(LineRect))
        {
            DrawTextLine(iLine, PutPoint);
        }
        PutPoint.y += miLineHeight;
        LineRect.wTop += miLineHeight;
        LineRect.wBottom += miLineHeight;

        iLine++;

        if (iLine >= miTotalLines)
        {
            break;
        }
    }
    EndClipToClient();
}

/*--------------------------------------------------------------------------*/
void PegTextBox::CheckBufLen(SIGNED iLen)
{
    if (miBufLen < iLen + 1)
    {
        if (mpBuf)
        {
            delete mpBuf;
        }
        miBufLen = iLen + 8;
        mpBuf = new TCHAR[miBufLen];
    }
}


/*--------------------------------------------------------------------------*/
// DrawTextLine: Called from ScrollText and DrawAllText to draw an
// individual line of text. It is assumed that the background has been
// initialized to the correct color unless bFill is set to TRUE.
/*--------------------------------------------------------------------------*/
void PegTextBox::DrawTextLine(SIGNED iLine, PegPoint Put, BOOL bFill)
{
    SIGNED iLineLength;
    PegRect FillRect;
    TCHAR *pGet = GetLineStart(iLine, &iLineLength);
    PegColor Color(muColors[PCI_NTEXT], muColors[PCI_NORMAL], CF_FILL);

    CheckBufLen(iLineLength);

    if (iLine == miMarkLine)
    {
        Color.Set(muColors[PCI_STEXT], muColors[PCI_SELECTED], CF_FILL);
        bFill = TRUE;
    }
    if (bFill)
    {
        FillRect.Set(mClient.wLeft + 2, Put.y, mClient.wRight - 2,
            Put.y + miLineHeight - 1);
        Rectangle(FillRect, Color, 0);
    }

    if (!pGet)
    {
        return;
    }

    Color.uFlags = CF_NONE;

    if (iLineLength > 0)
    {
        tstrncpy(mpBuf, pGet, iLineLength);
        mpBuf[iLineLength] = '\0';

        switch(Style() & TJ_MASK)
        {
        case TJ_RIGHT:
            iLineLength = TextWidth(mpBuf, mpFont);
            Put.x = mClient.wRight - iLineLength;
            break;

        case TJ_CENTER:
            iLineLength = TextWidth(mpBuf, mpFont);
            Put.x = (mClient.Width() - iLineLength) / 2;
            Put.x = mClient.wLeft;
            break;

        case TJ_LEFT:
        default:
            break;
        }
        DrawText(Put, mpBuf, Color, mpFont, -1);
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegTextBox::LineDown(void)
{
#define NO_SCROLL
#ifndef NO_SCROLL
    if (mpVScroll)
    {
        PegScrollInfo *psi = mpVScroll->GetScrollInfo();

        if (psi->wCurrent + psi->wVisible < psi->wMax)
        {
            // send myself a scroll down message:
            PegMessage NewMessage(mpVScroll, PEG_SIGNAL(PegVScroll::IDB_DOWNBUTTON, PSF_CLICKED));
            mpVScroll->Message(NewMessage);
            return(TRUE);
        }
    }
#endif	// NO_SCROLL
    return(FALSE);
}

/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
BOOL PegTextBox::LineUp(void)
{
#ifndef NO_SCROLL
    if (mpVScroll)
    {
        if (miTopLine)
        {
            // send myself a scroll up message:
            PegMessage NewMessage(PEG_SIGNAL(PegVScroll::IDB_UPBUTTON, PSF_CLICKED));
            mpVScroll->Message(NewMessage);
            return(TRUE);
        }
    }
#endif	// NO_SCROLL
    return(FALSE);
}


/*--------------------------------------------------------------------------*/
void PegTextBox::UpdateLineStarts(void)
{
/*
    if (miLineHeight)
    {
        miVisible = (mClient.Height() - 4) / miLineHeight;
    }
    else
    {
        miVisible = 1;
    }
    */
    SIGNED iNewStart = miTopLine - (MAX_LINE_OFFSETS / 4);

    FillLineStarts(iNewStart, TRUE);

    // make sure that the top line does not put us past the end of the text:

    PegScrollInfo si;
    GetVScrollInfo(&si);

    if (miTopLine + si.wVisible > miTotalLines)
    {
        miTopLine = miTotalLines - si.wVisible;

        if (miTopLine < 0)
        {
            miTopLine = 0;
        }
    }
}


/*--------------------------------------------------------------------------*/
TCHAR *PegTextBox::GetLineStart(SIGNED iLine, SIGNED *pPutLen)
{
    SIGNED iNewStart, iLen;

    if (pPutLen)
    {
        *pPutLen = 0;   // default to 0 length in case of early exit
    }

    if (!DataGet())
    {
        return NULL;
    }

    if (iLine < miLineStartTop)
    {
        iNewStart = iLine - (MAX_LINE_OFFSETS / 4);
        FillLineStarts(iNewStart, FALSE);
    }
    else
    {
        if (iLine > miLineStartEnd)
        {
            iNewStart = iLine - (MAX_LINE_OFFSETS / 2);
            FillLineStarts(iNewStart, FALSE);
        }
    }

    if (iLine < miLineStartTop ||
        iLine > miLineStartEnd)
    {
        return NULL;
    }
    iNewStart = iLine - miLineStartTop;

    if (mwLineStarts[iNewStart] == 0xffff)
    {
        return NULL;
    }
    TCHAR *pGet = DataGet() + mwLineStarts[iNewStart];

    if (pPutLen)
    {
        if (mwLineStarts[iNewStart + 1] != 0xffff)
        {
            iLen = mwLineStarts[iNewStart + 1] - mwLineStarts[iNewStart];
        }
        else
        {
            iLen = tstrlen(pGet);
        }
        if (iLen)
        {
            if (*(pGet + iLen - 1) == 0x0a)
            {
                iLen--;
            }
            if (*(pGet + iLen - 1) == 0x0d)
            {
                iLen--;
            }
        }
        *pPutLen = iLen;
    }
    return pGet;
}

/*--------------------------------------------------------------------------*/
WORD PegTextBox::GetLineIndex(SIGNED iLine)
{
    TCHAR *pStart = GetLineStart(iLine, NULL);
    if (pStart)
    {
        return((WORD) (pStart - DataGet()));
    }
    return 0xffff;
}




/*--------------------------------------------------------------------------*/
void PegTextBox::FillLineStarts(SIGNED iStart, BOOL bFull)
{
    TCHAR *pCurrent, *pStart;
    SIGNED iLoop;
    SIGNED sMax = mClient.Width() - 5;

    if (iStart < 0)
    {
        iStart = 0;
    }
    miLineStartTop = iStart;
    miLineStartEnd = miLineStartTop;

    miWidestLine = 0;

    for (iLoop = 0; iLoop < MAX_LINE_OFFSETS; iLoop++)
    {
        mwLineStarts[iLoop] = 0xffff;
    }

    // skip down to the first line in the start window:

    pCurrent = FindLinePointer(iStart);

    if (pCurrent)
    {
        if (bFull)
        {
            miTotalLines = iStart;
        }
    }
    else
    {
        return;
    }
    pStart = DataGet();

    iLoop = 0;

    // Fill in the line start offsets array:

    while(pCurrent && iLoop < MAX_LINE_OFFSETS)
    {
        mwLineStarts[iLoop] = (WORD) (pCurrent - pStart);
        miLineStartEnd++;
        if (bFull)
        {
            miTotalLines++;
        }
        iLoop++;
        pCurrent = FindNextLine(pCurrent, sMax);
    }
    miLineStartEnd--;

    // If the text has more lines than we can keep in the line starts
    // array, keep going so we know how many total lines there are:

    if (pCurrent && bFull)
    {
        pStart = FindNextLine(pCurrent, sMax);

        while(pStart)
        {
            miTotalLines++;
            pStart = FindNextLine(pStart, sMax);
        }
    }
}


/*--------------------------------------------------------------------------*/
TCHAR *PegTextBox::FindLinePointer(SIGNED iLine)
{
    TCHAR *pCurrent = DataGet();
    SIGNED iLoop = 0;
    SIGNED sMax = mClient.Width() - 5;

    while(iLoop < iLine && pCurrent)
    {
        pCurrent = FindNextLine(pCurrent, sMax);
        iLoop++;
    }
    return pCurrent;
}


/*--------------------------------------------------------------------------*/
TCHAR *PegTextBox::FindNextLine(TCHAR *Start, SIGNED sMaxWidth, SIGNED iDir)
{
    // find the longest line that will fit, or a hard return:

    TCHAR cTemp[2];
    TCHAR ch;
    TCHAR *Get = Start;
    TCHAR *LastBreak = Get;

    SIGNED iTotWidth = 0;
    SIGNED iThisWidth;


    if (!*Get)
    {
        return NULL;
    }

    cTemp[1] = '\0';

    while (1)
    {
        ch = *Get;
        if (!ch)
        {
            return NULL;
        }
        if (ch == ' ' || ch == ',' || ch == '-')
        {
            LastBreak = Get;
        }
        else
        {
            if (ch == 0x0d)
            {
                if (*(Get + 1) == 0x0a)
                {
                    return (Get + 2);
                }
                else
                {
                    return (Get + 1);
                }
            }
            else
            {
                if (ch == 0x0a)
                {
                    if (*(Get + 1))
                    {
                        return (Get + 1);
                    }
                    else
                    {
                        return NULL;
                    }
                }
                else
                {
                    if (iDir < 0)
                    {
                        if (Get <= DataGet())
                        {
                            return DataGet();
                        }
                    }
                }
            }
        }

        if ((Style() & EF_WRAP) ||
            (muScrollMode & (WSM_AUTOHSCROLL|WSM_HSCROLL)))
        {
            cTemp[0] = ch;
            iThisWidth = TextWidth(cTemp, mpFont);
            iTotWidth += iThisWidth;

            if (iTotWidth > miWidestLine)
            {
                miWidestLine = iTotWidth;
            }

            if (Style() & EF_WRAP)
            {
                if (iTotWidth >= sMaxWidth)
                {
                    if (LastBreak != Start)
                    {
                      return (LastBreak + iDir);
                    }
                    else
                    {
                        return (Get);
                    }
                }
            }
        }
        Get += iDir;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegTextBox::GetVScrollInfo(PegScrollInfo *Put)
{
    Put->wMin = 0;
    Put->wMax = 1;
    Put->wStep = 1;
    Put->wCurrent = miTopLine;
    Put->wVisible = miVisible;

    TCHAR *Text = DataGet();

    if (!Text)
    {
        return;
    }

    if (miTotalLines > Put->wMax)
    {
        Put->wMax = miTotalLines;
    }
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegTextBox::GetHScrollInfo(PegScrollInfo *Put)
{
    Put->wMin = 0;
    Put->wMax = miWidestLine + PEG_SCROLL_WIDTH + 2;
    Put->wStep = (mClient.Width() >> 4) + 1;
    Put->wCurrent = miLeftOffset;
    Put->wVisible = mClient.Width() - 4;
}


/*--------------------------------------------------------------------------*/
/*--------------------------------------------------------------------------*/
void PegTextBox::Append(const TCHAR *Text, BOOL bDraw)
{
    if (!Text)
    {
        return;
    }
    if (!(*Text))
    {
        return;
    }

    TCHAR *pGet = DataGet();

    if (!pGet)
    {
        DataSet(Text);
        return;
    }

    WORD wStartLen = (WORD) tstrlen(pGet);
    WORD wLen = tstrlen(Text) + wStartLen;

    if (wLen >= mwMaxChars)
    {
        SIGNED iAdjust = wLen - mwMaxChars;

        if (wStartLen >= iAdjust)
        {
            pGet += iAdjust;
            wLen -= iAdjust;
        }
        else
        {
            pGet = NULL;
            wLen -= wStartLen;
            if (wLen > mwMaxChars)
            {
                iAdjust = wLen - mwMaxChars;
                Text += iAdjust;
                wLen -= iAdjust;
            }
        }
    }

    wLen += 2;

    TCHAR *pTemp = new TCHAR[wLen];
    if (pGet)
    {
        tstrcpy(pTemp, pGet);
    }
    else
    {
        *pTemp = '\0';
    }
    tstrcat(pTemp, Text);
    PegTextThing::DataSet(pTemp);

    delete (pTemp);

    UpdateLineStarts();
    if (muScrollMode & WSM_AUTOSCROLL)
    {
        if (CheckAutoScroll())
        {
            UpdateLineStarts();
        }
    }

    SetTopLineToEnd();

    if (bDraw)
    {
        Draw();
    }
}

// End of file
