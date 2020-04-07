//........................................................................................
//........................................................................................
//.. File Name: ui.cpp                                                                  ..
//.. Date: 09/17/2001                                                                   ..
//.. Author(s): Daniel Bolstad                                                          ..
//.. Usage: The CUIMaster class is derived from the CScreen class, and is the only      ..
//..        class that should be directly added to the presentation manager. It is      ..
//..        the main 'container' window that is the parent of all windows in this ui.   ..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com                                ..
//.. Modification date: 09/24/2001                                                      ..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.                                   ..
//..     All rights reserved. This code may not be redistributed in source or linkable  ..
//..     object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................

#include <main/ui/UI.h>
#include <main/ui/SplashScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/RestoreMenuScreen.h>

// global
static CUIMaster* g_pUIMaster;
static CRestoreUIMaster* g_pRestoreUIMaster;

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_UI, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE( DBG_UI );

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
}

// reverse PegAppInit
void PegAppUninitialize(PegPresentationManager *pPresentation)
{
    g_pUIMaster->Remove(CPlayerScreen::GetPlayerScreen());
    pPresentation->Remove(g_pUIMaster);
    delete g_pUIMaster;
    g_pUIMaster = NULL;
}

void PegRestoreAppInitialize(PegPresentationManager *pPresentation, bool bUseSplashScreen)
{
    // Create the dialog and add it to PegPresentationManager:
    PegRect rtScreen;
    rtScreen.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);

    g_pRestoreUIMaster = new CRestoreUIMaster(rtScreen, FF_NONE, bUseSplashScreen);
    pPresentation->Add(g_pRestoreUIMaster);
    pPresentation->SetColor(PCI_NORMAL, BLACK);

    if (bUseSplashScreen)
        g_pRestoreUIMaster->Add(CSplashScreen::GetSplashScreen());
    else
        g_pRestoreUIMaster->Add(CRestoreMenuScreen::GetRestoreMenuScreen());
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

SIGNED
CUIMaster::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        DEBUGP( DBG_UI, DBGLEV_TRACE, ">>>> ui: uncaught key: %d\n", Mesg.iData);
        break;
    case PM_KEY_RELEASE:
        DEBUGP( DBG_UI, DBGLEV_TRACE, ">>>> ui: uncaught key release: %d\n", Mesg.iData);
        break;
        
    default:
        break;
    }
    
    return CScreen::Message(Mesg);
}



CRestoreUIMaster::CRestoreUIMaster(const PegRect& rect, WORD wStyle, bool bUseSplashScreen)
    : CScreen(NULL)
{
    mReal = rect;
    InitClient();
    RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);

    if (bUseSplashScreen)
    {
        CSplashScreen::GetSplashScreen()->SetParent(this);
        CSplashScreen::GetSplashScreen()->SetControlScreen(CRestoreMenuScreen::GetRestoreMenuScreen());
        CSplashScreen::GetSplashScreen()->ShowScreen();
        CRestoreMenuScreen::GetRestoreMenuScreen()->SetParent(this);
    }
}

CRestoreUIMaster::~CRestoreUIMaster()
{
}

