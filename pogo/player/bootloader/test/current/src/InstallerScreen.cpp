//........................................................................................
//........................................................................................
//.. File Name: InstallerScreen.h																..
//.. Date: 10/03/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: contains the implementation of CInstallerScreen class				..
//.. Usage:The CInstallerScreen class is an abstract base class from which more					..
//..	   specific screens are derived.  It contains info that every screen			..
//..	   needs, like screen title, menu caption, and functions to hide and show		..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 10/10/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include "InstallerScreen.h"

// The full size of the screen in pixels (128x64 for pogo)
#define UI_X_SIZE 128
#define UI_Y_SIZE 64

extern CBmpFont Verdana12Font;

void cyg_thread_delay(unsigned int cnt)
{
	unsigned long i;

	i = cnt * 0x1000; 
	while(i--)
		;
}

#define TIMER_SCREEN_CHANGE 999
const int sc_iScreenChangeInterval = 30;

static SequenceItemRec s_StartupItem[] = {
	{ true, SID_INTERACTIVE_OBJECTS_C, 0 },
	{ true, SID_INSTALLER_V, 0 },
	{ true, SID_PLAYER_V, 0 },
};

static SequenceItemRec s_DownloadItem[] = {
	{ true, SID_DOWNLOADING, 1 },
	{ true, SID_PROGRAMMING, 2 },
	{ true, SID_VERIFYING, 3 },
	{ true, SID_SUCCESSFUL, 4 },
};

static SimpleBitmap *gpbLogoBitmap = &gbIobjectsLogoBitmap;

#if 0 // FIXME: re-enable singleton code
CInstallerScreen* CInstallerScreen::s_pInstallerScreen = 0;


CInstallerScreen*
CInstallerScreen::GetInstallerScreen()
{
  if (!s_pInstallerScreen)
    s_pInstallerScreen = new CInstallerScreen();
  return s_pInstallerScreen;
}
#endif


CInstallerScreen::CInstallerScreen(CShow *show)
 
{
  m_show = show;
  BuildScreen();

  m_pbLogo = new CBitmap(gpbLogoBitmap->w,gpbLogoBitmap->h,gpbLogoBitmap->bpp,gpbLogoBitmap->buff); 
}


CInstallerScreen::~CInstallerScreen()
{
}


// Shows screen
void
CInstallerScreen::ShowScreen(eSystemMessage eSysMesg, int iSequence, const char* pszMesg)
{
 

  m_iStartSequenceNumber = 0;
  m_iDownloadSequenceNumber = 0;
  SetScreenData(eSysMesg, iSequence, pszMesg);
 
  diag_printf("install: Draw\n");
  m_show->Draw();

}


// changed to be synchronous
// external factors advance UI


void
CInstallerScreen::SetScreenData(eSystemMessage eSysMesg, int iSequence, const char* pszMesg)
{

  //	PegRect IconRect, StringRect;
  //	IconRect.Set(mReal.wLeft, mReal.wTop, mReal.wLeft, mReal.wTop);


  // list of images to hide, then selectively show
  m_pbLogo->SetVisible(GUI_FALSE);

	
	m_eSysMesg = eSysMesg;
	m_iProgressBarStage = 0;

	// set up what message we want to show.
	switch(eSysMesg)
	{
		case START_SEQUENCE:
		  if(m_iStartSequenceNumber < (sizeof(s_StartupItem) / sizeof(SequenceItemRec)))
			{
			  // bNewBitmap = s_StartupItem[iSequence].pBitmap;
			  ShowBitmap(m_pbLogo);
			  m_pString->SetText(LS(s_StartupItem[iSequence].wString));
			  m_iProgressBarStage = s_StartupItem[iSequence].iProgressBarStage;

			}
			else {
			  // FIXME: invalid screen
			}
			 
			break;
		case DOWNLOAD_SEQUENCE:
			if(m_iDownloadSequenceNumber < (sizeof(s_DownloadItem) / sizeof(SequenceItemRec)))
			{
			   ShowBitmap(m_pbLogo);
			   //	bNewBitmap = s_DownloadItem[iSequence].pBitmap;
				m_pString->SetText(LS(s_DownloadItem[iSequence].wString));
				m_iProgressBarStage = s_DownloadItem[iSequence].iProgressBarStage;
			}
			else {
			  // FIXME: invalid screen    SetRect(guiRect rect);
			}

			break;
		case SHUTTING_DOWN:
			m_pString->SetText(LS(SID_SHUTTING_DOWN));
			//bNewBitmap = &gbWarningBitmap;
			break;
		case LOW_POWER:
			m_pString->SetText(LS(SID_LOW_POWER));
			//bNewBitmap = &gbWarningBitmap;
			break;
		case FIRMWARE_UPDATE_FAILED:
			m_pString->SetText(LS(SID_FIRMWARE_UPDATE_FAILED));
			//bNewBitmap = &gbWarningBitmap;
			break;  
		case TEXT_MESSAGE:
		default:
			m_pString->SetText(pszMesg);
			break;
	}
}


void CInstallerScreen::ShowBitmap(CBitmap *pbit)
{

  guiRect real;
  GetReal(&real);
  pbit->SetVisible(GUI_TRUE);
  pbit->m_clip.ul.x = ((real.lr.x - real.ul.x) - pbit->m_nWidth) / 2;
  pbit->m_clip.lr.x = real.lr.x;
  pbit->m_clip.ul.y = (((real.lr.y - 20) - real.ul.y) - pbit->m_nHeight) / 2;
  pbit->m_clip.lr.y = real.lr.y - 20;
  pbit->m_start.x = pbit->m_clip.ul.x;
  pbit->m_start.y = pbit->m_clip.ul.y;

}
    
	
// Shows screens.
void
CInstallerScreen::HideScreen()
{

  // FIXME: hide all elements?
	
}


void
CInstallerScreen::Draw()
{

  // m_show->Clear();
  m_show->Draw();

  // FIXME: draw the progress bar now

#if 0
	PegRect ProgressBarRect;
	ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 6, mReal.wRight, mReal.wBottom);
	Invalidate(ProgressBarRect);

	BeginDraw();
	CScreen::Draw();

	// Draw the progress bar
	if(m_iProgressBarStage > 0)
	{
		Rectangle(ProgressBarRect, BLACK);

		// we don't want to divide by zero, do we?
		if (m_iProgressBarStage > 0)
		{
			int iProgress = (int)((double)m_iProgressBarStage * ((double)(mReal.wRight - mReal.wLeft - 1) / (double)(sizeof(s_DownloadItem) / sizeof(SequenceItemRec))));
			Line(ProgressBarRect.wLeft, ProgressBarRect.wTop, ProgressBarRect.wLeft + iProgress, ProgressBarRect.wTop, BLACK, ProgressBarRect.Height());
		}
	}
	EndDraw();
#endif


}


//
// Message removed, elements will be handled externally
//


void CInstallerScreen::GetReal(guiRect *rect)
{

  rect->ul.x = 0;
  rect->lr.x = UI_X_SIZE - 1;
  rect->ul.y = 0;
  rect->lr.y = UI_Y_SIZE - 1;

  DEBUGPRINTF("X: %d Y: %d\n",rect->lr.x,rect->lr.y);

}

// Creates all the icons used by the menu screen.
void
CInstallerScreen::BuildScreen()
{
  guiRect real;
  // create a screen, add it to the root screen manager
  m_screen = new CScreen();
  m_show->AddScreen(m_screen);

  GetReal(&real);

  m_pString = new CTextLabel();
  m_pString->SetFont(&FONT_PLAYSCREEN);  
  m_pString->SetText("");
  m_pString->m_clip.ul.x = real.ul.x;
  m_pString->m_clip.lr.x = real.lr.x;
  m_pString->m_clip.ul.y = real.lr.y - 16;
  m_pString->m_clip.lr.y = real.lr.y - 6;
  m_pString->m_start.x = real.ul.x;
  m_pString->m_start.y = real.lr.y - 16; 
  m_screen->AddScreenElem(m_pString);

  m_screen->AddScreenElem(m_pbLogo);
  m_pbLogo->SetVisible(FALSE);

  // can't remove screens right now
 

  DEBUGPRINTF("Screen Added\n");
  m_screen->SetVisible(GUI_TRUE);
  m_show->Clear();
  m_show->Draw();
}
