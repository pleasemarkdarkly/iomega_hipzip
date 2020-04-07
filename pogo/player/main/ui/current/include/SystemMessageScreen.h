//........................................................................................
//........................................................................................
//.. File Name: SystemMessageScreen.h															..
//.. Date: 10/02/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The CSystemMessageScreen class is an abstract base class from which more			..
//..	   specific screens are derived.												..
//.. Last Modified By: Dan Bolstad	danb@fullplaymedia.com									..
//.. Modification date: 10/02/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................

#ifndef SYSTEMMESSAGESCREEN_H_
#define SYSTEMMESSAGESCREEN_H_

#include <main/ui/Screen.h>

void ShowBriefTextMessage(const char* szMsg);

class CSystemMessageScreen : public CScreen
{
public:
	typedef enum eSystemMessage { SHUTTING_DOWN=0, LOW_POWER, USB_CONNECTED, PLAYER_LOCKED, INVALID_PLAYLIST, TEXT_MESSAGE, TEXT_MSG_NO_TIMEOUT, PLAYER_LOCKED_POWOFF };

	static CSystemMessageScreen* GetSystemMessageScreen();
    static void Destroy() {
		if (s_pSystemMessageScreen)
			delete s_pSystemMessageScreen;
		s_pSystemMessageScreen = 0;
    }
	
	// Hides any visible screens.
	void HideScreen();

	// Shows warning screen with a pre-canned message.
	void ShowScreen(CScreen* pScreen, eSystemMessage eSysMesg, const TCHAR* pszMesg = NULL);

	// Sets up the icons and the messages in the screen
	void SetScreenData(eSystemMessage eSysMesg, const TCHAR* pszMesg = NULL);
	void SetScreenDataByChar(eSystemMessage eSysMesg, const char* pszMesg = NULL);

	SIGNED Message(const PegMessage &Mesg);
	void Draw();

private:
	CSystemMessageScreen();
	~CSystemMessageScreen();

	void BuildScreen();

	static CSystemMessageScreen* s_pSystemMessageScreen;

	int			m_cItems;		// The number of screens in the list.
	PegRect		m_StringRect;
	CScreen*	m_pControlScreen;
	PegString*	m_pString;
	PegIcon*	m_pIcon;

	eSystemMessage	m_eSysMesg;
};

#endif  // SYSTEMMESSAGESCREEN_H_
