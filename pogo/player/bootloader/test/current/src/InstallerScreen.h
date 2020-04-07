//........................................................................................
//........................................................................................
//.. File Name: InstallerScreen.h															..
//.. Date: 10/03/2001																	..
//.. Author(s): Dan Bolstad																..
//.. Description of content: contains the definition of the CScreen class				..
//.. Usage:The CInstallerScreen class is an abstract base class from which more			..
//..	   specific screens are derived.												..
//.. Last Modified By: Dan Bolstad	danb@iobjects.com									..
//.. Modification date: 10/10/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#ifndef INSTALLERSCREEN_H_
#define INSTALLERSCREEN_H_

#include <gui/simplegui/common/UITypes.h>
#include <gui/simplegui/show/Show.h>
#include <gui/simplegui/screen/Screen.h>
#include <gui/simplegui/screenelem/textlabel/TextLabel.h>
#include <gui/simplegui/screenelem/bitmap/Bitmap.h>
#include "Bitmaps.h"
#include "Strings.h"
#include "Fonts.h"

#include <cyg/infra/diag.h>
#define DEBUGPRINTF diag_printf
typedef unsigned short WORD;


typedef struct
{
  bool bShowLogo;
  WORD		wString;
  int			iProgressBarStage;
} SequenceItemRec;

	typedef enum eSystemMessage { START_SEQUENCE=0, DOWNLOAD_SEQUENCE, SHUTTING_DOWN, LOW_POWER, FIRMWARE_UPDATE_FAILED, TEXT_MESSAGE };

class CInstallerScreen
{
public:

	CInstallerScreen(CShow* show);
	~CInstallerScreen();
	// Hides any visible screens.
	void HideScreen();

	// Shows warning screen with a pre-canned message.
	void ShowScreen(eSystemMessage eSysMesg = START_SEQUENCE, int iSequence = 0, const char* pszMesg = 0);

	// Sets up the icons and the messages in the screen
	void SetScreenData(eSystemMessage eSysMesg, int iSequence = 0, const char* pszMesg = 0);

	void Draw();

private:


	void GetReal(guiRect *rect);
	void BuildScreen();

	int			m_iStartSequenceNumber;
	int			m_iDownloadSequenceNumber;
	int			m_iProgressBarStage;

#if 0
       	PegRect		m_StringRect;
	CScreen*	m_pControlScreen;
	PegString*	m_pString;
	PegIcon*	m_pIcon;
#endif

	CShow *m_show;
	bool m_bAdded;
	CScreen *m_screen;
	CTextLabel *m_pString;

	CBitmap *m_pBitmap;



	eSystemMessage	m_eSysMesg;
};

#endif  // INSTALLERSCREEN_H_
