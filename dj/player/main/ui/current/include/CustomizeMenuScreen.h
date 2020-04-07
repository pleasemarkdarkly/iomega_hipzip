//
// CustomizeMenuScreen.cpp: implementation of CCustomizeMenuScreen class
// danb@fullplaymedia.com 07/23/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef CUSTOMIZEMENUSCREEN_H_
#define CUSTOMIZEMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

#define STR_SIZE 128

class CCustomizeMenuScreen : public CDynamicMenuScreen
{
public:
    static CScreen* GetCustomizeMenuScreen();
    
    static void Destroy() {
        if (s_pCustomizeMenuScreen)
            delete s_pCustomizeMenuScreen;
        s_pCustomizeMenuScreen = 0;
    }
    
    // Called before this screen is shown so it refreshes the info
    void RefreshInfo();
    
protected:
    
    // Called when the user hits the select button.
    // Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex);
    
    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};
    
    // The functions help get info about the menu items so we can draw them correctly
    // These functions must be defined by the derived class
    const TCHAR* MenuItemCaption(int iMenuIndex);
    
private:
    
    CCustomizeMenuScreen(CScreen* pParent = NULL);
    ~CCustomizeMenuScreen();
    
    // A pointer to the global instance of this class.
    static CCustomizeMenuScreen* s_pCustomizeMenuScreen;	
    
    TCHAR m_pszEjectCD[STR_SIZE];
    TCHAR m_pszExtendedChars[STR_SIZE];
    TCHAR m_pszWebControl[STR_SIZE];
    TCHAR m_pszTrackNumber[STR_SIZE];
    TCHAR m_pszAlbumWithArtistName[STR_SIZE];
    TCHAR m_pszPlayCDWhenInserted[STR_SIZE];
    TCHAR m_pszTextScrollSpeed[STR_SIZE];
    TCHAR m_pszEnableRecordLED[STR_SIZE];
    TCHAR m_pszLCDBrightness[STR_SIZE];
};

#endif  // CUSTOMIZEMENUSCREEN_H_
