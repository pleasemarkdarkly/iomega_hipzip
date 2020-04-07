//........................................................................................
//........................................................................................
//.. File Name: ui.cpp																..
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <main/demos/ssi_neo/ui/UI.h>


//#define PM_DISPLAY_TIME_INFO  (FIRST_USER_MESSAGE + 100)

// global
CUIMaster* g_pUIMaster;
//CMainMenuScreen* g_pMainMenuScreen;

/*--------------------------------------------------------------------------*/
// PegAppInitialize- called by the PEG library during program startup.
/*--------------------------------------------------------------------------*/
void PegAppInitialize(PegPresentationManager *pPresentation)
{
	// Create the dialog and add it to PegPresentationManager:
	PegRect rtScreen;
	rtScreen.Set(0, 0, 119, 79);
	g_pUIMaster = new CUIMaster(rtScreen);
	pPresentation->Add(g_pUIMaster);
	//g_pMainMenuScreen = new CMainMenuScreen();
	g_pUIMaster->Add(CPlayerScreen::GetPlayerScreen());
	//g_pUIMaster->Draw();
}



CUIMaster::CUIMaster(const PegRect& rect, WORD wStyle)
	: CScreen(NULL)
{
	mReal = rect;
	InitClient();
	RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

	CPlayerScreen::GetPlayerScreen()->SetParent(this);
	//CMainMenuScreen::GetMainMenuScreen()->SetParent(CPlayerScreen::GetPlayerScreen());
}

CUIMaster::~CUIMaster()
{
}
