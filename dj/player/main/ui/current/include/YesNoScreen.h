//
// YesNoScreen.h: definition of CYesNoScreen class
// danb@fullplaymedia.com 12/17/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef YESNOSCREEN_H_
#define YESNOSCREEN_H_

#include <main/ui/ProgressScreen.h>

class CYesNoScreen : public CProgressScreen
{
public:
	static CYesNoScreen* GetInstance();
    static void Destroy() {
		if (s_pYesNoScreen)
			delete s_pYesNoScreen;
		s_pYesNoScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

    void Draw();

    typedef void YesNoCallback(bool bYes);

    void Config(CScreen* pParent, YesNoCallback* pfnCB, bool bYes = false);

private:
	CYesNoScreen(CScreen* pParent);
	virtual ~CYesNoScreen();

	void BuildScreen();

    void DrawScreen();

	void ForceRedraw();

    void DoCallback();

	PegString*  m_pYesString;
	PegString*  m_pNoString;
    PegIcon*    m_pYesLeftArrow;
    PegIcon*    m_pYesRightArrow;
    PegIcon*    m_pNoLeftArrow;
    PegIcon*    m_pNoRightArrow;

    YesNoCallback* m_pfnCB;

    bool        m_bYes;

	static CYesNoScreen* s_pYesNoScreen;
};

#endif  // YESNOSCREEN_H_
