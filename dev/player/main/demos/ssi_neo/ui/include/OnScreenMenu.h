//........................................................................................
//........................................................................................
//.. File Name: OnScreenMenu.h															..
//.. Date: 08/28/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The COnScreenMenu class is an abstract base class from which more			..
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

#ifndef ONSCREENMENU_H_
#define ONSCREENMENU_H_

#include <main/demos/ssi_neo/ui/Screen.h>

class COnScreenMenu : public CScreen
{
public:
	COnScreenMenu(CScreen* pParent);
	~COnScreenMenu();

	// Hides any visible screens.
	virtual void HideScreen();

	// Shows screens.
	virtual void ShowScreen();

	SIGNED Message(const PegMessage &Mesg);
	void Draw();

private:
    int m_iPlaylistEntryIndex;
    int m_iPlaylistCount;

	void BuildScreen();

	PegString*	m_pCaption;
	PegIcon*	m_pBackArrow;
	PegIcon*	m_pForwardArrow;
	PegRect		m_CaptionRect;
};

#endif  // ONSCREENMENU_H_
