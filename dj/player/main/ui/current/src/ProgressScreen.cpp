//
// ProgressScreen.cpp: contains the implementation of the CProgressScreen class
// danb@fullplaymedia.com 07/10/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/ProgressScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/UI.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_PROGRESS_SCREEN, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( DBG_PROGRESS_SCREEN );

CProgressScreen::CProgressScreen(CScreen* pParent)
  : CScreen(pParent)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::Ctor\n");
	BuildScreen();
    ResetProgressBar();
}

CProgressScreen::~CProgressScreen()
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::Dtor\n");
}

void
CProgressScreen::Draw()
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::Draw\n");
    BeginDraw();
    CScreen::Draw();
    DrawProgressBar();
    EndDraw();
}

void
CProgressScreen::SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::SetMessageText [%s]\n", szText);
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
    Draw();
}

void
CProgressScreen::SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::SetMessageText [%w]\n", szText);
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
    Draw();
}

void
CProgressScreen::ResetProgressBar(int iProgress, int iTotal)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::ResetProgressBar\n");
    m_iProgressBarTotal = iTotal;
    UpdateProgressBar(iProgress);
}

void
CProgressScreen::UpdateProgressBar(int iProgress)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::UpdateProgressBar\n");
    // invalidate that part of the screen
    m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom);
    Invalidate(m_ProgressBarRect);
    
    if (iProgress <= 0 || m_iProgressBarTotal <= 0)
        // we don't want to show the progress bar
        m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wLeft - 1, mReal.wBottom);
    else
    {
		int iBarWidth = (int)((double)iProgress * ((double)(mReal.wRight - mReal.wLeft) / (double)m_iProgressBarTotal));
		m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, iBarWidth, mReal.wBottom);
    }
    Draw();
}

void
CProgressScreen::DrawProgressBar()
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::SetMessageText\n");
    if ((Presentation()->GetCurrentThing() == this) && (m_ProgressBarRect.Width() > 0))
        Screen()->InvertRect(this, m_ProgressBarRect);
}

void
CProgressScreen::SetTitleText(const TCHAR* szText)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::SetTitleText [%w]\n", szText);
	PegRect ChildRect;
	int iTextLength = 0, iCaptionLength = 0;

	m_pScreenTitle->DataSet(szText);
	// center the string
	iTextLength = Screen()->TextWidth(m_pScreenTitle->DataGet(), m_pScreenTitle->GetFont());
	iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    m_pScreenTitle->Resize(ChildRect);

    Screen()->Invalidate(m_pScreenTitle->mReal);
    Draw();
}

void
CProgressScreen::SetActionText(const TCHAR* szText)
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::SetActionText [%w]\n", szText);
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pActionTextString->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pActionTextString->DataGet(), m_pActionTextString->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pActionTextString->mReal.wTop, m_pActionTextString->mReal.wRight, m_pActionTextString->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pActionTextString->mReal.wTop, m_pActionTextString->mReal.wRight, m_pActionTextString->mReal.wBottom);
    m_pActionTextString->Resize(ChildRect);

    Screen()->Invalidate(m_pActionTextString->mReal);
    Draw();
}

void
CProgressScreen::BuildScreen()
{
    DEBUGP( DBG_PROGRESS_SCREEN, DBGLEV_TRACE, "CProgressScreen::BuildScreen\n");
	PegRect ChildRect;
	mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	// track text area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 24, mReal.wRight, mReal.wTop + 40);
	m_pActionTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pActionTextString->SetFont(&FONT_PLAYSCREENBIG);
	m_pActionTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pActionTextString);

	// screen title
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, mReal.wTop + 13);
	m_pScreenTitle = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY);
	m_pScreenTitle->SetFont(&FONT_MENUSCREENTITLE);
	m_pScreenTitle->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenTitle);

	// the horizontal bar on the top of the screen
	ChildRect.Set(mReal.wLeft, mReal.wTop + 16, mReal.wRight, mReal.wTop + 17);
	m_pTopScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pTopScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pTopScreenHorizontalDottedBarIcon);

	// the horizontal bar on the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom - 13);
	m_pBottomScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pBottomScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pBottomScreenHorizontalDottedBarIcon);

	// the message region of the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 13, mReal.wRight, mReal.wBottom);
	m_pMessageTextString = new CSystemMessageString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pMessageTextString->SetFont(&FONT_PLAYSCREEN);
	m_pMessageTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pMessageTextString);
}

