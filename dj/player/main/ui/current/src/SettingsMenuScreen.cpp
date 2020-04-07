//
// SettingsMenuScreen.cpp: implementation of CSettingsMenuScreen class
// danb@fullplaymedia.com 09/26/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/SettingsMenuScreen.h>
#include <main/ui/PlayOrderMenuScreen.h>
#include <main/ui/TimeMenuScreen.h>
#include <main/ui/BitrateMenuScreen.h>
#include <main/ui/NetSettingsMenuScreen.h>
#include <main/ui/InputGainMenuScreen.h>
#include <main/ui/CustomizeMenuScreen.h>
#ifndef DISABLE_VOLUME_CONTROL
#include <main/ui/VolumeMenuScreen.h>
#endif // DISABLE_VOLUME_CONTROL

#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/Keys.h>

// the global reference for this class
CSettingsMenuScreen* CSettingsMenuScreen::s_pSettingsMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
    { false, false, true, SID_PLAY_MODE, &CPlayOrderMenuScreen::GetPlayOrderMenuScreen },
    { false, false, true, SID_RECORDING_BITRATE, &CBitrateMenuScreen::GetBitrateMenuScreen },
    { false, false, true, SID_NETWORK_SETTINGS, &CNetSettingsMenuScreen::GetNetSettingsMenuScreen },
    { false, false, true, SID_LINE_INPUT_GAIN, &CInputGainMenuScreen::GetInputGainMenuScreen },
    { false, false, true, SID_CUSTOMIZATIONS, &CCustomizeMenuScreen::GetCustomizeMenuScreen },
#ifndef DISABLE_VOLUME_CONTROL
    { false, false, true, SID_VOLUME, &CVolumeMenuScreen::GetVolumeMenuScreen },
#endif // DISABLE_VOLUME_CONTROL
};


// This is a singleton class.
CScreen*
CSettingsMenuScreen::GetSettingsMenuScreen()
{
	if (!s_pSettingsMenuScreen) {
		s_pSettingsMenuScreen = new CSettingsMenuScreen(NULL);
	}
	return s_pSettingsMenuScreen;
}


CSettingsMenuScreen::CSettingsMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_JUKEBOX_SETTINGS, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
}

CSettingsMenuScreen::~CSettingsMenuScreen()
{
}

SIGNED
CSettingsMenuScreen::Message(const PegMessage &Mesg)
{
	switch (Mesg.wType)
	{
	case PM_KEY:

		switch (Mesg.iData)
		{
            case IR_KEY_SELECT:
			case KEY_SELECT:
				GotoSubMenu(GetHighlightedIndex());
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

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CSettingsMenuScreen::GotoSubMenu(int iMenuIndex)
{
    switch (iMenuIndex)
    {
    case 0:		        // Play Order
    {
        CPlayOrderMenuScreen* pScreen = (CPlayOrderMenuScreen*)CPlayOrderMenuScreen::GetPlayOrderMenuScreen();
        pScreen->SetHighlightedIndex(pScreen->GetSelectedItem());
        Parent()->Add(pScreen);
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(pScreen);
        break;
    }
    case 1:		        // Bitrate
    {
        CBitrateMenuScreen* pScreen = (CBitrateMenuScreen*)CBitrateMenuScreen::GetBitrateMenuScreen();
        pScreen->SetHighlightedIndex(pScreen->GetSelectedItem());
        Parent()->Add(pScreen);
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(pScreen);
        break;
    }
    case 2:		        //Network Settings
    {
        CNetSettingsMenuScreen* pScreen = (CNetSettingsMenuScreen*)CNetSettingsMenuScreen::GetNetSettingsMenuScreen();
        pScreen->SetHighlightedIndex(pScreen->GetSelectedItem());
        Parent()->Add(pScreen);
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(pScreen);
        break;
    }
    case 3:		        // Input Gain
    {
        CInputGainMenuScreen* pScreen = (CInputGainMenuScreen*)CInputGainMenuScreen::GetInputGainMenuScreen();
        pScreen->SetHighlightedIndex(pScreen->GetSelectedItem());
        Parent()->Add(pScreen);
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(pScreen);
        break;
    }
    case 4:		// Customizations
        ((CCustomizeMenuScreen*)CCustomizeMenuScreen::GetCustomizeMenuScreen())->RefreshInfo();
        Parent()->Add(CCustomizeMenuScreen::GetCustomizeMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CCustomizeMenuScreen::GetCustomizeMenuScreen());
        break;
#ifndef DISABLE_VOLUME_CONTROL
    case 5:		// Volume
        Parent()->Add(CVolumeMenuScreen::GetVolumeMenuScreen());
        m_pParent->Remove(this);
        Presentation()->MoveFocusTree(CVolumeMenuScreen::GetVolumeMenuScreen());
        break;
#endif // DISABLE_VOLUME_CONTROL
    };
}
