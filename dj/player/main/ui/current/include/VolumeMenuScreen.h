//
// VolumeMenuScreen.h: definition of CVolumeMenuScreen class
// danb@fullplaymedia.com 02/14/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef VOLUMEMENUSCREEN_H_
#define VOLUMEMENUSCREEN_H_

#ifndef DISABLE_VOLUME_CONTROL

#include <main/ui/DynamicMenuScreen.h>

class CVolumeMenuScreen : public CDynamicMenuScreen
{
public:
    static CScreen* GetVolumeMenuScreen();

    static void Destroy() {
		if (s_pVolumeMenuScreen)
			delete s_pVolumeMenuScreen;
		s_pVolumeMenuScreen = 0;
    }

    void Draw();
    
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

    // Notification from the scrolling list that the list has scrolled up.
    void NotifyScrollUp();
    
    // Notification from the scrolling list that the list has scrolled down.
    void NotifyScrollDown();
    
private:

	CVolumeMenuScreen();
	virtual ~CVolumeMenuScreen();

    TCHAR   m_pszVolumeValue[16];

	// A pointer to the global instance of this class.
	static CVolumeMenuScreen* s_pVolumeMenuScreen;	

};

#endif // DISABLE_VOLUME_CONTROL

#endif  // VOLUMEMENUSCREEN_H_
