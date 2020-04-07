//
// RestoreOptionsMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 05/14/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/RestoreOptionsMenuScreen.h>
#include <main/ui/RestoreScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

#include <main/ui/YesNoScreen.h>
#include <cyg/fileio/fileio.h>
#include <datastream/isofile/IsoFileInputStream.h>
#include <main/main/MiniCDMgr.h>
#include <main/main/AppSettings.h>
#include <util/debug/debug.h>
#include <main/ui/AlertScreen.h>

DEBUG_MODULE_S( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE( DBG_RESTORE_OPTIONS_MENU_SCREEN );

// the global reference for this class
CRestoreOptionsMenuScreen* CRestoreOptionsMenuScreen::s_pRestoreOptionsMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
    { false, true, false, SID_UPDATE_FIRMWARE, NULL },
    { false, true, false, SID_UPDATE_CDDB, NULL },
    { false, true, false, SID_RESET_CDDB, NULL },
    { false, true, false, SID_CLEAR_CD_CACHE, NULL },
    { false, true, false, SID_RESTORE_DEFAULT_USER_SETTINGS, NULL },
    { false, true, false, SID_START_SYSTEM_RESTORE, NULL },
};


// This is a singleton class.
CScreen*
CRestoreOptionsMenuScreen::GetRestoreOptionsMenuScreen()
{
	if (!s_pRestoreOptionsMenuScreen) {
		s_pRestoreOptionsMenuScreen = new CRestoreOptionsMenuScreen(NULL);
	}
	return s_pRestoreOptionsMenuScreen;
}


CRestoreOptionsMenuScreen::CRestoreOptionsMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SELECT_RESTORE_OPTIONS, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
    DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::Ctor\n");
}

CRestoreOptionsMenuScreen::~CRestoreOptionsMenuScreen()
{
    DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::Dtor\n");
}

SIGNED
CRestoreOptionsMenuScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::Message\n");
	switch (Mesg.wType)
	{
	case PM_KEY:
        DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::PM_KEY %d\n", Mesg.iData);
		switch (Mesg.iData)
		{
        case IR_KEY_INFO:
            return 0;
        case IR_KEY_POWER:
        case KEY_POWER:
            {
                CRestoreScreen* pRS = CRestoreScreen::GetInstance();
                Add(pRS);
                pRS->DoReset(LS(SID_NO_UPDATES_PERFORMED));
            }
        case IR_KEY_MENU:
        case KEY_MENU:
        case IR_KEY_EXIT:
        case KEY_EXIT:
            {
                CRestoreScreen* pRS = CRestoreScreen::GetInstance();
                pRS->SetRestoreOption(EVERYTHING, false);
                s_menuItems[0].bSelected = pRS->GetRestoreOption(UPDATE_SOFTWARE);
                s_menuItems[1].bSelected = pRS->GetRestoreOption(UPDATE_CDDB);
                s_menuItems[2].bSelected = pRS->GetRestoreOption(RESET_CDDB);
                s_menuItems[3].bSelected = pRS->GetRestoreOption(CLEAR_CD_CACHE);
                s_menuItems[4].bSelected = pRS->GetRestoreOption(CLEAR_SAVED_SETTINGS);
                
                m_pParent->Remove(this);
                Presentation()->MoveFocusTree(m_pParent);
            }
			return 0;
        default:
            break;
        }
        break;
	case PM_KEY_RELEASE:
        DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::PM_KEY_RELEASE %d\n", Mesg.iData);
        return 0;

	default:
		break;
	}
	return CMenuScreen::Message(Mesg);
}

// Called when the user hits the play/pause button.
// Acts based upon the currently highlighted menu item.
void
CRestoreOptionsMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    DEBUGP( DBG_RESTORE_OPTIONS_MENU_SCREEN, DBGLEV_TRACE, "CRestoreOptionsMenuScreen::ProcessMenuOption\n");
    CRestoreScreen* pRS = CRestoreScreen::GetInstance();
	switch (iMenuIndex)
	{
    case 0:
        s_menuItems[iMenuIndex].bSelected = pRS->SetRestoreOption(UPDATE_SOFTWARE, !pRS->GetRestoreOption(UPDATE_SOFTWARE));
		break;

    case 1:
        s_menuItems[iMenuIndex].bSelected = pRS->SetRestoreOption(UPDATE_CDDB, !pRS->GetRestoreOption(UPDATE_CDDB));
		break;

    case 2:
        s_menuItems[iMenuIndex].bSelected = pRS->SetRestoreOption(RESET_CDDB, !pRS->GetRestoreOption(RESET_CDDB));
		break;

    case 3:
        s_menuItems[iMenuIndex].bSelected = pRS->SetRestoreOption(CLEAR_CD_CACHE, !pRS->GetRestoreOption(CLEAR_CD_CACHE));
		break;

    case 4:
        s_menuItems[iMenuIndex].bSelected = pRS->SetRestoreOption(CLEAR_SAVED_SETTINGS, !pRS->GetRestoreOption(CLEAR_SAVED_SETTINGS));
		break;

    case 5:		// start the restore process (tcl - now with magic confirmation!!)

		if(!pRS->DoCDCheck(this, LS(SID_FACTORY_RESTORE)))
			break;

		if(pRS->GetRestoreOption(FORMAT_HD))
		{
			CYesNoScreen::GetInstance()->Config(this, FormatConfirmCB);
            CYesNoScreen::GetInstance()->SetTitleText(LS(SID_RESTORE_PROCEDURE));
            CYesNoScreen::GetInstance()->SetActionText(LS(SID_FORMAT_HARD_DISK_QM));
			Add(CYesNoScreen::GetInstance());
		}
		else
		{
			// don't need confirmation for non-destructive options
			Parent()->Add(pRS);
			m_pParent->Remove(this);
			Presentation()->MoveFocusTree(pRS);
			pRS->DoSelectedOptions();
		}

		break;
	}

    Draw();
}


void
CRestoreOptionsMenuScreen::FormatConfirm(bool bConfirm)
{

	CRestoreScreen* pRS = CRestoreScreen::GetInstance();

	if(bConfirm)
	{        
   		Parent()->Add(pRS);
		m_pParent->Remove(this);
		Presentation()->MoveFocusTree(pRS);
		pRS->DoSelectedOptions();
	}
}

