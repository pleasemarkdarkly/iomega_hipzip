//
// EditScreen.cpp: implementation of CEditScreen class
// Usage: This class is derived from the CScreen class, and will be overlayed
//		on the calling screen and give the user the abliity to edit a value and
//      choose to save or ignore the changes.
// chuckf@fullplaymedia.com 02/12/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/EditScreen.h>

#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_EDIT_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_EDIT_SCREEN );

#define TIMEOUT_LENGTH                 4500
#define CURSOR_ADVANCE_TIMEOUT          150

static const Key_t sExtAlphaKeys[10] = 
{
    {2, {" 0", " 0"}},
    {66, {"1!\"#$%&'()*+,-.:;<=>?@[]^_`{|}~¡¢£¤¥¦§¨©ª«¬-®¯°±²³´µ¶·¸¹º»¼½¾¿×÷",
          "1!\"#$%&'()*+,-.:;<=>?@[]^_`{|}~¡¢£¤¥¦§¨©ª«¬-®¯°±²³´µ¶·¸¹º»¼½¾¿×÷"}},
    {23, {"aàáâãäåæbcçAÀÁÂÃÄÅÆBCÇ2", "AÀÁÂÃÄÅÆBCÇaàáâãäåæbcç2"}},
    {17, {"dğeèéêëfDĞEÈÉÊËF3", "DĞEÈÉÊËFdğeèéêëf3"}},
    {15, {"ghiìíîïGHIÌÍÎÏ4", "GHIÌÍÎÏghiìíîï4"}},
    {7, {"jklJKL5", "JKLjkl5"}},
    {21, {"mnñoòóôõöøMNÑOÒÓÔÕÖØ6", "MNÑOÒÓÔÕÖØmnñoòóôõöø6"}},
    {10, {"pqrsPQRSß7", "PQRSßpqrs7"}},
    {17, {"tŞuùúûüvTşUÙÚÛÜV8", "TşUÙÚÛÜVtŞuùúûüv8"}},
    {12, {"wxyÿızWXYİZ9", "WXYİZwxyÿız9"}},
};
static const Key_t sAlphaKeys[10] = 
{
    {2, {" 0", " 0"}},
    {31, {"1!\"#$%&\'()*+,-.:;<=>?@[]^_`{|}~", "1!\"#$%&\'()*+,-.:;<=>?@[]^_`{|}~"}},
    {7, {"abcABC2", "ABCabc2"}},
    {7, {"defDEF3", "DEFdef3"}},
    {7, {"ghiGHI4", "GHIghi4"}},
    {7, {"jklJKL5", "JKLjkl5"}},
    {7, {"mnoMNO6", "MNOmno6"}},
    {9, {"pqrsPQRS7", "PQRSpqrs7"}},
    {7, {"tuvTUV8", "TUVtuv8"}},
    {9, {"wxyzWXYZ9", "WXYZwxyz9"}},
};
static const Key_t sNumericKeys[10] =
{
    {1, {"0", "0"}},
    {1, {"1", "1"}},
    {1, {"2", "2"}},
    {1, {"3", "3"}},
    {1, {"4", "4"}},
    {1, {"5", "5"}},
    {1, {"6", "6"}},
    {1, {"7", "7"}},
    {1, {"8", "8"}},
    {1, {"9", "9"}},
};

static CKeyTable sExtAlphaTable( &(sExtAlphaKeys[0]) );
static CKeyTable sAlphaTable( &(sAlphaKeys[0]) );
static CKeyTable sNumericTable( &(sNumericKeys[0]) );

char CKeyTable::GetKey(unsigned int KeyNum, unsigned int KeyRepeated, TCHAR PreviousChar)
{
	char ReturnChar;
	KeyNum-= IR_KEY_0_space;
	if(KeyRepeated)
		KeyRepeated %= pKeys[KeyNum].Length;
    TCHAR tcSpace = ' ';
    bool bCaps = (PreviousChar == 0) || (PreviousChar == tcSpace);
    ReturnChar = pKeys[KeyNum].KeyValues[(bCaps)?1:0][KeyRepeated];
	return ReturnChar;
}

CEditString::CEditString(const PegRect &Rect, const TCHAR *Text, WORD wld, WORD wStyle, SIGNED iLen):
	PegString(Rect, Text, wld, wStyle, iLen) {}

CEditScreen* CEditScreen::s_pEditScreen = 0;

// This is a singleton class.
CEditScreen*
CEditScreen::GetInstance()
{
	if (!s_pEditScreen) {
		s_pEditScreen = new CEditScreen(NULL);
	}
	return s_pEditScreen;
}

CEditScreen::CEditScreen(CScreen* pParent)
  : CScreen(pParent),
  m_bCaps(true),
  m_bShowCursor(true),
  m_Repeat(0),
  m_CurrentKey(0),
  m_pDefaultSystemMessage(LS(SID_PUSH_SAVE_TO_KEEP_CHANGES)),
  m_iCursor(0),
  m_pfnCB(NULL),
    m_bSave(false),
    m_bHideScreen(true)
{
    DEBUGP( DBG_EDIT_SCREEN, DBGLEV_TRACE, "EditScreen:Ctor\n");
	BuildScreen();
}

CEditScreen::~CEditScreen()
{
    DEBUGP( DBG_EDIT_SCREEN, DBGLEV_TRACE, "EditScreen:Dtor\n");
    if( m_eViewMode == CDJPlayerState::ZOOM ) {
        Add(m_pEditString);
        Add(m_pScreenTitle);
    } else {
        Add(m_pZoomTextString);
    }
}

void
CEditScreen::Draw()
{
    BeginDraw();
    CScreen::Draw();
	if(m_bShowCursor)
	{
		Line(m_CursorPos.x, m_pCurrentString->mClient.wTop + 1, m_CursorPos.x, m_pCurrentString->mClient.wBottom - 1, BLACK);
	}
    EndDraw();
}

void CEditScreen::AdvanceCursor(char advance)
{
	TCHAR Temp[2]  = {advance, '\0'};
	m_CursorPos.x += TextWidth(Temp, m_pCurrentFont);
}

void CEditScreen::RetardCursor(char retard)
{
	TCHAR Temp[2]  = {retard, '\0'};
	m_CursorPos.x -= TextWidth(Temp, m_pCurrentFont);
}

SIGNED
CEditScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        // restart the screen timeout timer
        KillTimer(ES_TIMER_TIMEOUT);
        SetTimer(ES_TIMER_TIMEOUT, TIMEOUT_LENGTH, 0);
    
        if (InsertKey(Mesg.iData))
        {
            Draw();
            CheckSendSignal(PSF_TEXT_EDIT);
            return 0;
        }
        else
        {
            switch(Mesg.iData)
            {
            case IR_KEY_SAVE:
            case IR_KEY_SELECT:
            case KEY_SELECT:
                m_bSave = true;
                DoCallback();
                return 0;
            case IR_KEY_EXIT:
            case KEY_EXIT:
                m_bSave = false;
                DoCallback();
                CPlayerScreen::GetPlayerScreen()->HideMenus();
                return 0;  
            case IR_KEY_SHIFT:
                m_bCaps = !m_bCaps;
                return 0;
            case IR_KEY_MENU:
            case KEY_MENU:
            case IR_KEY_INFO:
                m_bSave = false;
                DoCallback();
                return 0;
            // let these keys fall through and get handled elsewhere
            case KEY_POWER:
            case IR_KEY_POWER:
            case IR_KEY_ZOOM:
                break;
            // ignore all other these keys
            default:
                return 0;
            }
        }
        break;

    case PM_KEY_RELEASE:
        return 0;

    case PM_TIMER:
        switch (Mesg.iData)
        {
        case ES_TIMER_TIMEOUT:
            m_bSave = false;
            DoCallback();
            return 0;
        case ES_TIMER_CURSOR_ADVANCE:
            m_bShowCursor = true;
            SetMessageText(m_pDefaultSystemMessage, CSystemMessageString::STATUS);
            Invalidate();
            Draw();
            return 0;
        default:
            break;
        }
        break;

    default:
        break;
    }
    
    //ForceRedraw();
    return CScreen::Message(Mesg);
}


void
CEditScreen::Config(CScreen* pParent, EditCallback* pfnCB, const TCHAR* szText, bool bSave, bool bNumericOnly, int iMaxLen = 80)
{
    m_pfnCB = pfnCB;
    SetEditText(szText);
    SetMessageText(m_pDefaultSystemMessage, CSystemMessageString::STATUS);
    m_bSave = bSave;
    m_bShowCursor = true;
    m_bCaps = true;
    m_iMaxLen = iMaxLen;
    if( bNumericOnly ) {
        m_pKeyTable = &sNumericTable;
    } else {
        CDJPlayerState* pDJPS = CDJPlayerState::GetInstance();
        m_pKeyTable = pDJPS->GetUIEnableExtChars() ? &sExtAlphaTable : &sAlphaTable;
    }
    if (m_pCurrentString->DataGet())
    {
        TCHAR* cGet   = m_pCurrentString->DataGet();
        m_iCursor     = tstrlen(cGet);
        m_CursorPos.x = m_pCurrentString->mReal.wLeft + TextWidth(cGet, m_pCurrentFont) + 1;
        m_pCurrentString->SetFirstVisible(0);
        while (m_CursorPos.x >= (m_pCurrentString->mClient.wRight - 5))
        {
            RetardCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
            m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() + 1);
        }
    }
    else
    {
        m_iCursor     = 0;
        m_CursorPos.x = m_pCurrentString->mClient.wLeft + 1;
        m_CursorPos.y = m_pCurrentString->mClient.wTop;
    }
    SetTimer(ES_TIMER_TIMEOUT, TIMEOUT_LENGTH, 0);
    SetParent(pParent);    
}


void
CEditScreen::DoCallback()
{
   	KillTimer(ES_TIMER_TIMEOUT);
   	KillTimer(ES_TIMER_CURSOR_ADVANCE);
    // don't allow saving of null strings
    // todo, we should eventually allow this and handle null strings differently in the callback function
    // In addition to checking for 0 length strings, check for all whitespace strings: "   "
    bool   bAllWhitespace = true;
    for (TCHAR *cGet = m_pCurrentString->DataGet(); (*cGet) && bAllWhitespace; ++cGet)
        if ((*cGet) != (TCHAR)' ')
            bAllWhitespace = false;
    if((m_pCurrentString->DataGet() == NULL) || bAllWhitespace)
    {
        DEBUGP( DBG_EDIT_SCREEN, DBGLEV_INFO, "es:don't allow saving empty strings\n");
        m_bSave = false;
    }
    if(m_pfnCB)
        m_pfnCB(m_bSave);
    if(m_bHideScreen)
    {
        HideScreen();
        Presentation()->MoveFocusTree(m_pParent);
    }
    else
        m_bHideScreen = true;
}


void
CEditScreen::SetEditText(const TCHAR* szText)
{
	m_pCurrentString->DataSet(szText);
	m_iCursor = tstrlen(szText);
	Screen()->Invalidate(m_pCurrentString->mReal);
	Draw();
}

void
CEditScreen::ForceRedraw()
{
    Invalidate(mReal);
    Draw();
}

void
CEditScreen::SetViewMode(CDJPlayerState::EUIViewMode eViewMode)
{
	CScreen::SetViewMode(eViewMode);
	SynchWithViewMode();
}
    
void
CEditScreen::BuildScreen()
{
	PegRect ChildRect, ZoomRect;
    mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	ZoomRect.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
    RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
    
    // edit area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 24, mReal.wRight, mReal.wTop + 40);
	m_pEditString = new CEditString(ChildRect, NULL, 0, FF_NONE|AF_ENABLED|EF_EDIT|TT_COPY );
	m_pEditString->SetFont(&FONT_PLAYSCREENBIG);
	m_pEditString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	m_pCurrentFont = m_pEditString->GetFont();
	m_pCurrentString = m_pEditString;
    Add(m_pEditString);

    // zoom edit string
	ChildRect.Set(mReal.wLeft, mReal.wTop + 5, mReal.wRight, mReal.wBottom - 15);
	m_pZoomTextString = new CEditString(ChildRect, NULL, 0, FF_NONE|AF_ENABLED|EF_EDIT|TT_COPY );
	m_pZoomTextString->SetFont(&FONT_PLAYSCREENZOOM);
	m_pZoomTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);

    // screen title
	ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, mReal.wTop + 13);
	m_pScreenTitle = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY);
	m_pScreenTitle->SetFont(&FONT_MENUSCREENTITLE);
	m_pScreenTitle->RemoveStatus(PSF_ACCEPTS_FOCUS);
	Add(m_pScreenTitle);
    SetTitleText(LS(SID_EDIT));

    // the horizontal bar on the top of the screen
	ChildRect.Set(mReal.wLeft, mReal.wTop + 16, mReal.wRight, mReal.wTop + 17);
	m_pTopScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pTopScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pTopScreenHorizontalDottedBarIcon);

	// the horizontal bar on the screen
	ChildRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom - 13);
	m_pScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
	m_pScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pScreenHorizontalDottedBarIcon);

    // the message region of the screen
    ChildRect.Set(mReal.wLeft, mReal.wBottom - 13, mReal.wRight, mReal.wBottom);
	m_pMessageTextString = new CSystemMessageString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
	m_pMessageTextString->SetFont(&FONT_PLAYSCREEN);
	m_pMessageTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	Add(m_pMessageTextString);
    SetMessageText(m_pDefaultSystemMessage, CSystemMessageString::STATUS);
 
}

void
CEditScreen::SynchWithViewMode()
{
	switch(m_eViewMode)
	{
	case CDJPlayerState::ZOOM:
		m_pCurrentFont   = m_pZoomTextString->GetFont();
		m_pCurrentString = m_pZoomTextString;
		Remove(m_pEditString, false);
        Remove(m_pScreenTitle, false);
		m_pZoomTextString->DataSet(m_pEditString->DataGet());
		Add(m_pZoomTextString, false);
		break;
	case CDJPlayerState::NORMAL:
	default:
		m_pCurrentFont = m_pEditString->GetFont();
		m_pCurrentString = m_pEditString;
		Remove(m_pZoomTextString, false);
		m_pEditString->DataSet(m_pZoomTextString->DataGet());
		Add(m_pEditString, false);
        Add(m_pScreenTitle, false);
		break;
	}
	if (m_pCurrentString->DataGet())
    {
		TCHAR* cGet   = m_pCurrentString->DataGet();
        m_CursorPos.x = m_pCurrentString->mClient.wLeft + 1;
        for (int iCursor = 0; iCursor < m_iCursor; ++iCursor)
            AdvanceCursor(cGet[iCursor]);
		m_pCurrentString->SetFirstVisible(0);
		while (m_CursorPos.x >= (m_pCurrentString->mClient.wRight - 5))
		{
			RetardCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
			m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() + 1);
		}
    }
    Draw();
}

BOOL CEditScreen::InsertKey(SIGNED iKey)
{
    TCHAR *cBuff;
    TCHAR *cGet;
	TCHAR NewChar;
    cGet = m_pCurrentString->DataGet();
    // dc- For now, if the user requests unlimited length strings...ignore them and force 127 byte max
	SIGNED miMaxLen = (m_iMaxLen ? m_iMaxLen : 127);

    if (iKey >= IR_KEY_0_space && iKey <= IR_KEY_9_wxyz)
    {
        if (miMaxLen > 0)
        {
            if (cGet)
            {
                if ((SIGNED) tstrlen(cGet) >= miMaxLen)
                {
                    return FALSE;
                }
            }
        }
		if(m_bShowCursor)
		{
			m_CurrentKey = iKey;
			m_Repeat     = 0;
			if (cGet)
			{
                TCHAR PreviousChar = ((m_iCursor >= 1) ? cGet[m_iCursor-1] : 0);
                ShowSubKey(m_CurrentKey, m_Repeat, PreviousChar);
				cBuff = new TCHAR[tstrlen(cGet) + 2];
				tstrcpy(cBuff, cGet);
				NewChar = (TCHAR) m_pKeyTable->GetKey(iKey, m_Repeat++, PreviousChar);
				*(cBuff + m_iCursor) = NewChar;
				tstrcpy(cBuff + m_iCursor + 1, cGet + m_iCursor);
				m_pCurrentString->DataSet(cBuff);
				delete cBuff;
			}
			else
			{
                ShowSubKey(m_CurrentKey, m_Repeat, 0);
				TCHAR cTemp[2];
				NewChar = (TCHAR) m_pKeyTable->GetKey(iKey, m_Repeat++, 0);
				cTemp[0] = NewChar;
				cTemp[1] = '\0';
				m_pCurrentString->DataSet(cTemp);
			}
            // There are no subkeys when it is just numbers, so don't add a timeout
            // for the cursor to return.  This makes entering e.g. '111' much less
            // painful.
            if (m_pKeyTable != &sNumericTable)
            {
                // Add the timer for the subkeys
                m_bShowCursor = false;
                SetTimer(ES_TIMER_CURSOR_ADVANCE, CURSOR_ADVANCE_TIMEOUT,0);
            }
			m_iCursor++;
			AdvanceCursor(NewChar);
		}
		else
		{
			RetardCursor(cGet[m_iCursor-1]);
			if(m_CurrentKey == (unsigned)iKey)
			{
                TCHAR PreviousChar = ((m_iCursor >= 2) ? cGet[m_iCursor-2] : 0);
                ShowSubKey(m_CurrentKey, m_Repeat, PreviousChar);
				cGet[m_iCursor-1] = (TCHAR) m_pKeyTable->GetKey(iKey, m_Repeat++, PreviousChar);
				AdvanceCursor(cGet[m_iCursor-1]);
			}
			else
			{
				m_Repeat = 0;
				m_CurrentKey = iKey;
                TCHAR PreviousChar = cGet[m_iCursor-1];
                ShowSubKey(m_CurrentKey, m_Repeat, PreviousChar);
#if 0 // You must timeout out to set new characters.  
				cGet[m_iCursor-1] = (TCHAR) m_pKeyTable->GetKey(iKey, m_Repeat++, m_bCaps);
				AdvanceCursor(cGet[m_iCursor-1]);
#else // A new keypress or timeout sets the character.
                AdvanceCursor(cGet[m_iCursor-1]);
				cBuff = new TCHAR[tstrlen(cGet) + 2];
				tstrcpy(cBuff, cGet);
				NewChar = (TCHAR) m_pKeyTable->GetKey(iKey, m_Repeat++, PreviousChar);
				*(cBuff + m_iCursor) = NewChar;
				tstrcpy(cBuff + m_iCursor + 1, cGet + m_iCursor);
				m_pCurrentString->DataSet(cBuff);
				delete cBuff;
				m_iCursor++;
				AdvanceCursor(NewChar);
#endif                
			}
			Invalidate();
			KillTimer(ES_TIMER_CURSOR_ADVANCE);
			SetTimer(ES_TIMER_CURSOR_ADVANCE, CURSOR_ADVANCE_TIMEOUT,0);
		}
		while (m_CursorPos.x >= (m_pCurrentString->mClient.wRight - 5))
        {
			RetardCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
            m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() + 1);
        }
        return TRUE;
    }
    else
    {
        switch(iKey)
        {
        case IR_KEY_DELETE:
            if (m_iCursor && cGet)
            {
				cBuff = new TCHAR[tstrlen(cGet) + 2];
                tstrcpy(cBuff, cGet);
                RetardCursor(*(cBuff + m_iCursor - 1));
                tstrcpy(cBuff + m_iCursor - 1, cGet + m_iCursor);
                m_iCursor--;
				if(m_iCursor && m_iCursor == m_pCurrentString->GetFirstVisible())
				{
					m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() - 1);
					AdvanceCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
				}
				while (m_CursorPos.x < m_pCurrentString->mClient.wLeft)
				{
					AdvanceCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
                    if (m_pCurrentString->GetFirstVisible() > 0)
				        m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() - 1);
				}
                m_pCurrentString->DataSet(cBuff);
                delete cBuff;

                m_bShowCursor = true;
                SetMessageText(m_pDefaultSystemMessage, CSystemMessageString::STATUS);
                Invalidate();
                Draw();
            }
            break;

        case IR_KEY_CLEAR:
            m_iCursor     = 0;
            m_CursorPos.x = m_pCurrentString->mClient.wLeft + 1;
            m_CursorPos.y = m_pCurrentString->mClient.wTop;
            m_pCurrentString->DataSet(LS(SID_EMPTY_STRING));
            m_pCurrentString->SetFirstVisible(0);
            break;

        case IR_KEY_NEXT:
            if (cGet)
            {
                if (m_iCursor < (SIGNED) tstrlen(cGet))
                {
                    AdvanceCursor(*(cGet + m_iCursor));
                    m_iCursor++;
					while (m_CursorPos.x >= (m_pCurrentString->mClient.wRight - 5))
					{
						RetardCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
					    m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() + 1);
					}
                    Invalidate();
                }
            }
            break;

        case IR_KEY_PREV:
            if (m_iCursor && cGet)
            {
                RetardCursor(*(cGet + m_iCursor - 1));
                m_iCursor--;
				if(m_iCursor && m_iCursor == m_pCurrentString->GetFirstVisible())
				{
					m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() - 1);
					AdvanceCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
				}
				while (m_CursorPos.x < m_pCurrentString->mClient.wLeft)
				{
					AdvanceCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
                    if (m_pCurrentString->GetFirstVisible() > 0)
    				    m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() - 1);
				}
                Invalidate();
            }
            break;
        default:
            return FALSE;
        }
    }
    return (TRUE);
}

void
CEditScreen::SetTitleText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pScreenTitle->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pScreenTitle->DataGet(), m_pScreenTitle->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    m_pScreenTitle->Resize(ChildRect);
    Screen()->Invalidate(m_pScreenTitle->mReal);
    Draw();
}

void
CEditScreen::SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType)
{
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
	Draw();
}

void
CEditScreen::SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType)
{
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
	Draw();
}

void
CEditScreen::ShowSubKey(SIGNED iKey, unsigned int Repeat, TCHAR PreviousChar)
{
    SetMessageText(m_pKeyTable->GetKeyValues(iKey, PreviousChar), CSystemMessageString::STATUS);
    Repeat %= m_pKeyTable->GetKeyValuesLength(iKey);
    m_pMessageTextString->SetMark(Repeat, Repeat + 1);
}
