//
// EditScreen.h: contains the definition of the CEditScreen class
// Usage: This class is derived from the CScreen class, and will be overlayed
//		on the calling screen and give the user the abliity to edit a value and
//      choose to save or ignore the changes.
// chuckf@fullplaymedia.com 02/12/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#ifndef EDITSCREEN_H_
#define EDITSCREEN_H_

#include <gui/peg/peg.hpp>
#include <main/ui/ScrollingListScreen.h>
#include <main/ui/SystemMessageString.h>
#include <util/datastructures/SimpleVector.h>
#include <main/ui/Keys.h>

typedef struct Key_s
{
	unsigned int Length;
	char* KeyValues[2];
}Key_t;

class CKeyTable
{
public:
	CKeyTable(const Key_t* Keys) : pKeys(Keys) {}
	~CKeyTable() {}
	char GetKey(unsigned int KeyNum, unsigned int KeyRepeated, TCHAR PreviousChar);
    const char * GetKeyValues(unsigned int KeyNum, TCHAR PreviousChar) 
    {
        TCHAR tcSpace = ' ';
        bool bCaps = (PreviousChar == 0) || (PreviousChar == tcSpace);
        KeyNum -= IR_KEY_0_space;
        return pKeys[KeyNum].KeyValues[(bCaps)?1:0];
    }
    unsigned int GetKeyValuesLength(unsigned int KeyNum)
    {
        KeyNum -= IR_KEY_0_space;
        return pKeys[KeyNum].Length;
    }

private:
	const Key_t* pKeys;
};

class CEditString : public PegString
{
public:
	CEditString(const PegRect &Rect, const TCHAR *Text = NULL, WORD wld = 0, WORD wStyle =
		FF_RECESSED|AF_ENABLED|EF_EDIT, SIGNED iLen = -1);
	void SetFirstVisible(SIGNED position) {miFirstVisibleChar = position;}
	SIGNED GetFirstVisible() const {return miFirstVisibleChar;}
};


class CEditScreen : public CScreen
{
public:
	static CEditScreen* GetInstance();
    static void Destroy() {
		if (s_pEditScreen)
			delete s_pEditScreen;
		s_pEditScreen = 0;
    }
	SIGNED       Message(const PegMessage &Mesg);
    void         Draw();
    typedef void EditCallback(bool bSave);
    void         Config(CScreen* pParent, EditCallback* pfnCB, const TCHAR* szText, bool bSave = false, bool bNumericOnly = false, int iMaxLen = 80);
	const TCHAR* GetDataString() {return m_pCurrentString->DataGet();}
	void         SetViewMode(CDJPlayerState::EUIViewMode eViewMode);
    void         SetTitleText(const TCHAR* szText);
    void         SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);
    void         SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType = CSystemMessageString::INFO);

    void SetHideScreen(bool bHideScreen) { m_bHideScreen = bHideScreen; }
    
protected:
	virtual BOOL InsertKey(SIGNED iKey);
	bool m_bCaps;
	bool m_bShowCursor;
	CKeyTable* m_pKeyTable;
	unsigned int m_Repeat;
	unsigned int m_CurrentKey;
    unsigned int m_iMaxLen;

private:
	CEditScreen(CScreen* pParent);
	virtual ~CEditScreen();
	void BuildScreen();
	void SynchWithViewMode();
	void ForceRedraw();
	void SetIcon();
	void SetEditText(const TCHAR* szText);
    void DoCallback();
	void AdvanceCursor(char advance);
	void RetardCursor(char retard);
    void ShowSubKey(SIGNED iKey, unsigned int Repeat, TCHAR PreviousChar);
    
    const TCHAR * m_pDefaultSystemMessage;
    
	CEditString*            m_pEditString;
	CEditString*            m_pZoomTextString;
    PegString*              m_pScreenTitle;
    CSystemMessageString*   m_pMessageTextString;
	SIGNED                  m_iCursor;
	PegPoint                m_CursorPos;
	CEditString*            m_pCurrentString;
	PegFont*                m_pCurrentFont;
    PegIcon*                m_pTopScreenHorizontalDottedBarIcon;
    PegIcon*                m_pScreenHorizontalDottedBarIcon;
    
    EditCallback* m_pfnCB;

    bool        m_bSave;

	static CEditScreen* s_pEditScreen;

        bool m_bHideScreen;
};

#endif  // EDITSCREEN_H_
