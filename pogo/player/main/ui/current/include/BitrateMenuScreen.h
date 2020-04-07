//........................................................................................
//........................................................................................
//.. File Name: BitrateMenuScreen.h															..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: definition of CBitrateMenuScreen class							..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#ifndef BITRATEMENUSCREEN_H_
#define BITRATEMENUSCREEN_H_

#include <main/ui/MenuScreen.h>

class CBitrateMenuScreen : public CMenuScreen
{
public:
    static CScreen* GetBitrateMenuScreen();
    
    static void Destroy() {
        if (s_pBitrateMenuScreen)
            delete s_pBitrateMenuScreen;
        s_pBitrateMenuScreen = 0;
    }
    
    int GetBitrate();
    void SetBitrate(int nBitrate);
        
protected:
    
    // Called when the user hits the select button.
    // Acts based upon the currently highlighted menu item.
    void ProcessMenuOption(int iMenuIndex);
    
    // Called when the user hits the next button.
    // Acts based upon the currently highlighted menu item.
    void GotoSubMenu(int iMenuIndex) {};
    
private:
    
    CBitrateMenuScreen(CScreen* pParent = NULL);
    virtual ~CBitrateMenuScreen();
    
    // A pointer to the global instance of this class.
    static CBitrateMenuScreen* s_pBitrateMenuScreen;	
    
    int m_nBitrate;
    
};

#endif  BITRATEMENUSCREEN_H_
