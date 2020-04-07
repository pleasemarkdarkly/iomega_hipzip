//
// InputGainMenuScreen.h: definition of CInputGainMenuScreen class
// danb@fullplaymedia.com 02/14/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef INPUTGAINMENUSCREEN_H_
#define INPUTGAINMENUSCREEN_H_

#include <main/ui/DynamicMenuScreen.h>
#include <main/main/LineRecorder.h>

class CInputGainMenuScreen : public CDynamicMenuScreen
{
public:
    static CScreen* GetInputGainMenuScreen();

    static void Destroy() {
		if (s_pInputGainMenuScreen)
			delete s_pInputGainMenuScreen;
		s_pInputGainMenuScreen = 0;
    }

    void Draw();

    int GetSelectedItem() { return ((m_cItems - 1) - CLineRecorder::GetInstance()->GetGain()); }
            
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
    bool MenuItemSelected(int iMenuIndex);
    bool MenuItemSelectable(int iMenuIndex) { return true; };
    
private:

	CInputGainMenuScreen();
	virtual ~CInputGainMenuScreen();

    TCHAR   m_pszVolumeValue[16];

    // A pointer to the global instance of this class.
	static CInputGainMenuScreen* s_pInputGainMenuScreen;	

};

#endif  // INPUTGAINMENUSCREEN_H_
