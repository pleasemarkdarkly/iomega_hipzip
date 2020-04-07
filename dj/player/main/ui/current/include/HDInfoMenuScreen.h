//
// HDInfoMenuScreen.cpp: implementation of CHDInfoMenuScreen class
// danb@fullplaymedia.com 07/22/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef HDINFOMENUSCREEN_H_
#define HDINFOMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>

#define STR_SIZE 128

class CHDInfoMenuScreen : public CDynamicMenuScreen
{
public:
    static CScreen* GetHDInfoMenuScreen();
    
    static void Destroy() {
        if (s_pHDInfoMenuScreen)
            delete s_pHDInfoMenuScreen;
        s_pHDInfoMenuScreen = 0;
    }
    
    // Called before this screen is shown so it refreshes the info
    void GetHDInfo();
    
protected:
    
    // Called when the user hits the select button.
    // Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex) {};
    
    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};
    
    // The functions help get info about the menu items so we can draw them correctly
    // These functions must be defined by the derived class
    const TCHAR* MenuItemCaption(int iMenuIndex);
    
private:
    
    CHDInfoMenuScreen(CScreen* pParent = NULL);
    ~CHDInfoMenuScreen();
    
    // A pointer to the global instance of this class.
    static CHDInfoMenuScreen* s_pHDInfoMenuScreen;	
    
    TCHAR m_pszFreeDiskSpace[STR_SIZE];
    TCHAR m_pszFreeDiskTime[STR_SIZE];
    TCHAR m_pszNumberOfTracks[STR_SIZE];
    TCHAR m_pszNumberOfAlbums[STR_SIZE];
    TCHAR m_pszNumberOfArtists[STR_SIZE];
    TCHAR m_pszNumberOfGenres[STR_SIZE];
    TCHAR m_pszNumberOfPlaylists[STR_SIZE];
};

#endif  // HDINFOMENUSCREEN_H_
