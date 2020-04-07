//........................................................................................
//........................................................................................
//.. File Name: ui.cpp																	..
//.. Date: 09/17/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Usage: The CUIMaster class is derived from the CScreen class, and is the only		..
//..	    class that should be directly added to the presentation manager. It is		..
//..		the main 'container' window that is the parent of all windows in this ui.	..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/24/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#include <main/ui/UI.h>

// global
CUIMaster* g_pUIMaster;

/*--------------------------------------------------------------------------*/
// PegAppInitialize- called by the PEG library during program startup.
/*--------------------------------------------------------------------------*/
void PegAppInitialize(PegPresentationManager *pPresentation)
{
	// Create the dialog and add it to PegPresentationManager:
	PegRect rtScreen;
	rtScreen.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);

	g_pUIMaster = new CUIMaster(rtScreen);
	pPresentation->Add(g_pUIMaster);
	pPresentation->SetColor(PCI_NORMAL, BLACK);
	g_pUIMaster->Add(CSplashScreen::GetSplashScreen());
}



CUIMaster::CUIMaster(const PegRect& rect, WORD wStyle)
	: CScreen(NULL)
{
	mReal = rect;
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	CSplashScreen::GetSplashScreen()->SetParent(this);
	CSplashScreen::GetSplashScreen()->SetControlScreen(CPlayerScreen::GetPlayerScreen());
	CSplashScreen::GetSplashScreen()->ShowScreen();
	CPlayerScreen::GetPlayerScreen()->SetParent(this);
}

CUIMaster::~CUIMaster()
{
}
