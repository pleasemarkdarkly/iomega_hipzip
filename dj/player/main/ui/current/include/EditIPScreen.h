//
// EditIPScreen.h: contains the definition of the CEditIPScreen class
// Usage: This class is derived from the CScreen class, and will be overlayed
//		on the calling screen and give the user the abliity to edit a value and
//      choose to save or ignore the changes.
// chuckf@fullplaymedia.com 02/12/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef EDITIPSCREEN_H_
#define EDITIPSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/ui/ScrollingListScreen.h>
#include <main/ui/SystemMessageString.h>
#include <util/datastructures/SimpleVector.h>
#include <main/ui/IPAddressString.h>
#include <main/ui/Keys.h>
#include <main/ui/EditScreen.h>

typedef struct IPKey_s
{
	unsigned int Length;
	char* KeyValues;
}IPKey_t;

class CIPKeyTable
{
public:
	CIPKeyTable();
	~CIPKeyTable() {}
	char GetKey(unsigned int KeyNum, unsigned int KeyRepeated, bool bCaps);
    const char * GetKeyValues(unsigned int KeyNum) 
    {
        KeyNum -= IR_KEY_0_space;
        return Keys[KeyNum]->KeyValues;
    }
    unsigned int GetKeyValuesLength(unsigned int KeyNum)
    {
        KeyNum -= IR_KEY_0_space;
        return Keys[KeyNum]->Length;
    }

private:
	const IPKey_t* Keys[10];
};

class CEditIPScreen : public CScreen
{
public:
	static CEditIPScreen* GetInstance();
    static void Destroy() {
		if (s_pEditIPScreen)
			delete s_pEditIPScreen;
		s_pEditIPScreen = 0;
    }
	SIGNED Message(const PegMessage &Mesg);
    void Draw();
    typedef void EditCallback(bool bSave);
    void Config(CScreen* pParent, EditCallback* pfnCB, const TCHAR* szText, bool bSave = false);
	const TCHAR* GetDataString() {return m_pCurrentString->DataGet();}
	unsigned int GetIPAddress() {return m_pIPString->GetIPAddress();}
	void SetViewMode(CDJPlayerState::EUIViewMode eViewMode);
    void SetTitleText(const TCHAR* szText);
    void SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);

protected:
	bool InsertKey(SIGNED iKey);
	bool m_bCaps;
	CIPKeyTable m_IPKeyTable;
	unsigned int m_Repeat;
	unsigned int m_CurrentKey;

private:
	CEditIPScreen(CScreen* pParent);
	virtual ~CEditIPScreen();
	void BuildScreen();
	void SynchWithViewMode();
	void ForceRedraw();
	void SetIcon();
	void SetEditText(const TCHAR* szText);
    void DoCallback();
	void AdvanceCursor(char advance);
	void RetardCursor(char retard);
    void ShowSubKey(SIGNED iKey, unsigned int Repeat);
    void AdvanceQuad();
    void RetardQuad();
    int  GetQuadSize();
    
    const TCHAR * m_pDefaultSystemMessage;

	CIPAddressString*     m_pIPString;
	CEditString*          m_pEditIPString;
	CEditString*          m_pZoomTextString;
    CEditString*          m_pCurrentString;
    PegString*            m_pScreenTitle;
    CSystemMessageString* m_pMessageTextString;
	SIGNED                m_iCursor;
	PegPoint              m_CursorPos;
	PegFont*              m_pCurrentFont;
    PegIcon*              m_pTopScreenHorizontalDottedBarIcon;
    PegIcon*              m_pScreenHorizontalDottedBarIcon;
    SIGNED                m_iQuadPos;
    bool                  m_bClearQuad;
    
    EditCallback* m_pfnCB;

    bool m_bSave;

	static CEditIPScreen* s_pEditIPScreen;
};

#endif  // EDITIPSCREEN_H_
