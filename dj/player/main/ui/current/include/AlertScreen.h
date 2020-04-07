//
// AlertScreen.h: definition of CAlertScreen class
// danb@fullplaymedia.com 03/13/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef ALERTSCREEN_H_
#define ALERTSCREEN_H_

#include <main/ui/ProgressScreen.h>

#define AS_DEFAULT_TIMEOUT_LENGTH              200

class CAlertScreen : public CProgressScreen
{
public:
	static CAlertScreen* GetInstance();
    static void Destroy() {
		if (s_pAlertScreen)
			delete s_pAlertScreen;
		s_pAlertScreen = 0;
    }

	SIGNED Message(const PegMessage &Mesg);

	void HideScreen();

    typedef void FNTimeoutCallback();

    void Config(CScreen* pParent, int iTimeout = 0, FNTimeoutCallback* pfnCB = NULL);

    typedef void FNCancelCallback();

    void SetCancellable(FNCancelCallback* pfnCancelCB)
        { m_pfnCancelCB = pfnCancelCB; }

private:
	CAlertScreen(CScreen* pParent);
	virtual ~CAlertScreen();

	void BuildScreen();

	void ForceRedraw();

    void DoCallback();

    FNTimeoutCallback* m_pfnTimeoutCB;

    FNCancelCallback* m_pfnCancelCB;

	static CAlertScreen* s_pAlertScreen;
};

#endif  // ALERTSCREEN_H_
