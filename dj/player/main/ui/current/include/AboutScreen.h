//
// AboutScreen.h: definition of CAboutScreen class
// danb@fullplaymedia.com 07/22/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef ABOUTSCREEN_H_
#define ABOUTSCREEN_H_

#include <main/ui/Screen.h>

class CAboutScreen : public CScreen
{
public:
	static CAboutScreen* GetAboutScreen();
    static void Destroy() {
		if (s_pAboutScreen)
			delete s_pAboutScreen;
		s_pAboutScreen = 0;
    }
	
	SIGNED Message(const PegMessage &Mesg);

private:
	CAboutScreen();
	~CAboutScreen();

	void BuildScreen();

    void GotoPreviousMenu();

	static CAboutScreen* s_pAboutScreen;
	
	PegIcon*	m_pScreenWindow;
};

#endif  // ABOUTSCREEN_H_
