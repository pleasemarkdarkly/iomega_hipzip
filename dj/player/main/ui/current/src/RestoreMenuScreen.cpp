//
// RestoreMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 05/10/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/RestoreMenuScreen.h>
#include <main/ui/RestoreScreen.h>
#include <main/ui/RestoreOptionsMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <main/ui/YesNoScreen.h>
#include <main/ui/AlertScreen.h>

#include <main/main/MiniCDMgr.h>
#include <main/main/AppSettings.h>

#include <cyg/fileio/fileio.h>
#include <datastream/isofile/IsoFileInputStream.h>
#include <util/debug/debug.h>


DEBUG_MODULE_S( DBG_RESTORE_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE( DBG_RESTORE_MENU_SCREEN );

// the global reference for this class
CRestoreMenuScreen* CRestoreMenuScreen::s_pRestoreMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
    { false, false, false, SID_CHECK_HARD_DISK, NULL },
    { false, false, false, SID_FACTORY_RESTORE, NULL },
    { false, false, true, SID_SELECT_RESTORE_OPTIONS, &CRestoreOptionsMenuScreen::GetRestoreOptionsMenuScreen },
};


// This is a singleton class.
CScreen*
CRestoreMenuScreen::GetRestoreMenuScreen()
{
	if (!s_pRestoreMenuScreen) {
		s_pRestoreMenuScreen = new CRestoreMenuScreen(NULL);
	}
	return s_pRestoreMenuScreen;
}


CRestoreMenuScreen::CRestoreMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_RESTORE_MENU, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
    DEBUGP( DBG_RESTORE_MENU_SCREEN, DBGLEV_TRACE, "CRestoreMenuScreen::Ctor\n");
}

CRestoreMenuScreen::~CRestoreMenuScreen()
{
    DEBUGP( DBG_RESTORE_MENU_SCREEN, DBGLEV_TRACE, "CRestoreMenuScreen::Dtor\n");
}


SIGNED
CRestoreMenuScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_RESTORE_MENU_SCREEN, DBGLEV_TRACE, "CRestoreMenuScreen::Message\n");
	switch (Mesg.wType)
	{
	case PM_KEY:

		switch (Mesg.iData)
		{
        case IR_KEY_POWER:
        case KEY_POWER:
        case IR_KEY_EXIT:
        case KEY_EXIT:
            {
                CRestoreScreen* pRS = CRestoreScreen::GetInstance();
                Add(pRS);
                pRS->DoReset(LS(SID_NO_UPDATES_PERFORMED));
            }
        case IR_KEY_INFO:
        case IR_KEY_MENU:
        case KEY_MENU:
        case IR_KEY_PREV:
			return 0;
        default:
            break;
        }
        break;

	default:
		break;
	}
	return CMenuScreen::Message(Mesg);
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CRestoreMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    DEBUGP( DBG_RESTORE_MENU_SCREEN, DBGLEV_TRACE, "CRestoreMenuScreen::ProcessMenuOption(%d)\n", iMenuIndex);

    CRestoreScreen* pRS = CRestoreScreen::GetInstance();
	switch (iMenuIndex)
	{
	case 0:		// check disk

        pRS->SetRestoreOption(CHECK_DISK, true);
        pRS->SetTitleText(LS(SID_CHECK_HARD_DISK));
        Parent()->Add(pRS);
        Presentation()->MoveFocusTree(pRS);
        pRS->DoSelectedOptions();
		break;

    case 1:		// restore everything
		pRS->SetRestoreOption(EVERYTHING, true);

		if(!pRS->DoCDCheck(this, LS(SID_FACTORY_RESTORE)))
        {
			pRS->SetRestoreOption(EVERYTHING, false);
            break;
        }
        
		CYesNoScreen::GetInstance()->Config(this, EverythingConfirmCB);
        CYesNoScreen::GetInstance()->SetTitleText(LS(SID_FACTORY_RESTORE));
        CYesNoScreen::GetInstance()->SetActionText(LS(SID_ERASING_MUSIC_CONTINUE_QM));
		Add(CYesNoScreen::GetInstance());

		break;

    case 2:		// select the restore options
        GotoSubMenu(iMenuIndex);
		break;
	}
}

void
CRestoreMenuScreen::EverythingConfirm(bool bConfirm)
{
	CRestoreScreen* pRS = CRestoreScreen::GetInstance();

	if(bConfirm)
	{        
        Parent()->Add(pRS);
        Presentation()->MoveFocusTree(pRS);
        pRS->DoSelectedOptions();
	}
    else
    {
        pRS->SetRestoreOption(EVERYTHING, false);
    }
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CRestoreMenuScreen::GotoSubMenu(int iMenuIndex)
{
    DEBUGP( DBG_RESTORE_MENU_SCREEN, DBGLEV_TRACE, "CRestoreMenuScreen::GotoSubMenu(%d)\n", iMenuIndex);
	switch (iMenuIndex)
	{
	case 0:		// restore everything
    case 1:		// check disk
		break;

    case 2:		// select the restore options
        Parent()->Add(CRestoreOptionsMenuScreen::GetRestoreOptionsMenuScreen());
        //m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CRestoreOptionsMenuScreen::GetRestoreOptionsMenuScreen());
		break;
	}
}

