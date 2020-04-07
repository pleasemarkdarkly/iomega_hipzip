//........................................................................................
//........................................................................................
//.. File Name: SplashScreen.h															..
//.. Date: 08/28/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The CSplashScreen class is an abstract base class from which more			..
//..	   specific screens are derived.												..
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..
//.. Modification date: 08/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef SPLASHSCREEN_H_
#define SPLASHSCREEN_H_

#include <main/ui/Screen.h>

typedef struct
{
	PegBitmap*	pBitmap;
	PegString*	pString;
} SplashItemRec;

class CSplashScreen : public CScreen
{
public:
	static CSplashScreen* GetSplashScreen();
    static void Destroy() {
		if (s_pSplashScreen)
			delete s_pSplashScreen;
		s_pSplashScreen = 0;
    }
	
	// Set the Screen that gets control after this splash screen
	void SetControlScreen(CScreen* pScreen) { m_pControlScreen = pScreen; }

	// Hides any visible screens.
	void HideScreen();

	// Shows screens.
	void ShowScreen();

	SIGNED Message(const PegMessage &Mesg);
	void Draw();

private:
	CSplashScreen();
	~CSplashScreen();

	void BuildScreen();

	static CSplashScreen* s_pSplashScreen;

	int			m_cItems;		// The number of screens in the list.
	int			m_iCurrentScreen;
	
	CScreen*	m_pControlScreen;
	PegIcon*	m_pScreenWindow;
	PegString*	m_pVersionString;
	PegString*	m_pCopyrightString;
};

#endif  // SPLASHSCREEN_H_
