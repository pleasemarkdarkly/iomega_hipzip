//
// EditIPScreen.cpp: implementation of CEditIPScreen class
// Usage: This class is derived from the CScreen class, and will be overlayed
//		on the calling screen and give the user the abliity to edit a value and
//      choose to save or ignore the changes.
// chuckf@fullplaymedia.com 02/12/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/EditIPScreen.h>

#include <main/ui/Strings.hpp>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/UI.h>
#include <main/ui/Timers.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_EDIT_IP_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_EDIT_IP_SCREEN );

#define TIMEOUT_LENGTH                 4500

static const IPKey_t Key0 = {1, "0"};
static const IPKey_t Key1 = {1, "1"};
static const IPKey_t Key2 = {1, "2"};
static const IPKey_t Key3 = {1, "3"};
static const IPKey_t Key4 = {1, "4"};
static const IPKey_t Key5 = {1, "5"};
static const IPKey_t Key6 = {1, "6"};
static const IPKey_t Key7 = {1, "7"};
static const IPKey_t Key8 = {1, "8"};
static const IPKey_t Key9 = {1, "9"};

CIPKeyTable::CIPKeyTable()
{
	Keys[0] = &Key0;
	Keys[1] = &Key1;
	Keys[2] = &Key2;
	Keys[3] = &Key3;
	Keys[4] = &Key4;
	Keys[5] = &Key5;
	Keys[6] = &Key6;
	Keys[7] = &Key7;
	Keys[8] = &Key8;
	Keys[9] = &Key9;
}

char CIPKeyTable::GetKey(unsigned int KeyNum, unsigned int KeyRepeated, bool bCaps)
{
	char ReturnChar;
	KeyNum-= IR_KEY_0_space;
	if(KeyRepeated)
		KeyRepeated%= Keys[KeyNum]->Length;
	ReturnChar = Keys[KeyNum]->KeyValues[KeyRepeated];
	if(bCaps && ReturnChar >= 'a' && ReturnChar <= 'z')
		ReturnChar-= ('a' - 'A');
	return ReturnChar;
}

CEditIPScreen* CEditIPScreen::s_pEditIPScreen = 0;

// This is a singleton class.
CEditIPScreen*
CEditIPScreen::GetInstance()
{
	if (!s_pEditIPScreen) {
		s_pEditIPScreen = new CEditIPScreen(NULL);
	}
	return s_pEditIPScreen;
}

CEditIPScreen::CEditIPScreen(CScreen* pParent)
  : CScreen(pParent),
  m_bCaps(true),
  m_Repeat(0),
  m_CurrentKey(0),
  m_pDefaultSystemMessage(LS(SID_PUSH_SAVE_TO_KEEP_CHANGES)),
  m_iCursor(0),
  m_pfnCB(NULL),
  m_bSave(false)
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "EditIPScreen:Ctor\n");
	BuildScreen();
}

CEditIPScreen::~CEditIPScreen()
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "EditIPScreen:Dtor\n");
}

void
CEditIPScreen::Draw()
{
    BeginDraw();
    CScreen::Draw();
    Line(m_CursorPos.x, m_pCurrentString->mClient.wTop + 1, m_CursorPos.x, m_pCurrentString->mClient.wBottom - 1, BLACK);
    EndDraw();
}

void CEditIPScreen::AdvanceCursor(char advance)
{
	TCHAR Temp[2] = {advance, '\0'};
	m_CursorPos.x+= TextWidth(Temp, m_pCurrentFont);
}

void CEditIPScreen::RetardCursor(char retard)
{
	TCHAR Temp[2] = {retard, '\0'};
	m_CursorPos.x-= TextWidth(Temp, m_pCurrentFont);
}

SIGNED
CEditIPScreen::Message(const PegMessage &Mesg)
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
                ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
                return 0;
            case IR_KEY_SHIFT:
                m_bCaps = !m_bCaps;
                return 0;
            case IR_KEY_MENU:
            case KEY_MENU:
                m_bSave = false;
                DoCallback();
                return 0;
            // let these keys fall through and get handled elsewhere
            case KEY_POWER:
            case IR_KEY_POWER:
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
CEditIPScreen::Config(CScreen* pParent, EditCallback* pfnCB, const TCHAR* szText, bool bSave)
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:Config +\n");
    m_pfnCB = pfnCB;
    // initialize the ipstring to zero or "0.0.0.0" to start
    m_pIPString->DataSet((unsigned int)0);
    // set the ipstring to the incoming text.  does error checking on the way in
    // so it won't change from "0.0.0.0" unless the text resolves to a valid ip address
    m_pIPString->DataSet(szText);
    SetEditText(m_pIPString->DataGet());
    SetMessageText(m_pDefaultSystemMessage, CSystemMessageString::STATUS);
    m_bSave       = bSave;
    m_bCaps       = true;
    m_iCursor     = 0;
    m_CursorPos.x = m_pCurrentString->mClient.wLeft + 1;
    m_CursorPos.y = m_pCurrentString->mClient.wTop;
    AdvanceQuad();
    SetTimer(ES_TIMER_TIMEOUT, TIMEOUT_LENGTH, 0);
    SetParent(pParent);
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:Config -\n");
}


void
CEditIPScreen::DoCallback()
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:DoCallback\n");
   	KillTimer(ES_TIMER_TIMEOUT);
    // don't allow saving of null strings
    // todo, we should eventually allow this and handle null strings differently in the callback function
    if(m_pCurrentString->DataGet() == NULL)
    {
        DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_INFO, "eips:don't allow saving empty strings\n");
        m_bSave = false;
    }
    HideScreen();
   	Presentation()->MoveFocusTree(m_pParent);
    if(m_pfnCB)
    {
        if(m_pIPString->ValidateIPAddressString(m_pCurrentString->DataGet()))
        {
            DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_INFO, "eips:Valid New IP\n");
            m_pIPString->DataSet(m_pCurrentString->DataGet());
            m_pfnCB(m_bSave);
        }
        else
        {
            DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_INFO, "eips:Invalid New IP, Don't Save\n");
            m_pfnCB(false);
        }
    }
}


void
CEditIPScreen::SetEditText(const TCHAR* szText)
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:SetEditText +\n");
	m_pCurrentString->DataSet(szText);
	m_iCursor = 0;
	Screen()->Invalidate(m_pCurrentString->mReal);
	Draw();
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:SetEditText -\n");
}

void
CEditIPScreen::ForceRedraw()
{
    Invalidate(mReal);
    Draw();
}

void
CEditIPScreen::SetViewMode(CDJPlayerState::EUIViewMode eViewMode)
{
	CScreen::SetViewMode(eViewMode);
	SynchWithViewMode();
}
    
void
CEditIPScreen::BuildScreen()
{
	PegRect ChildRect;
    mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
	InitClient();
    RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
    
    // ip string
	ChildRect.Set(mReal.wLeft - 1, mReal.wTop - 1, mReal.wLeft - 1, mReal.wTop - 1);
    m_pIPString = new CIPAddressString(ChildRect, LS(SID_BLANK_IP_ADDRESS), 0, FF_NONE|AF_ENABLED|EF_EDIT|TT_COPY );
    // this isn't visible

    // edit area
	ChildRect.Set(mReal.wLeft, mReal.wTop + 24, mReal.wRight, mReal.wTop + 40);
    m_pEditIPString = new CEditString(ChildRect, NULL, 0, FF_NONE|AF_ENABLED|EF_EDIT|TT_COPY );
	m_pEditIPString->SetFont(&FONT_PLAYSCREENBIG);
	m_pEditIPString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
	m_pCurrentFont   = m_pEditIPString->GetFont();
	m_pCurrentString = m_pEditIPString;
    Add(m_pEditIPString);

    // zoom edit area
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
CEditIPScreen::SynchWithViewMode()
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:SynchWithViewMode +\n");
	switch(m_eViewMode)
	{
	case CDJPlayerState::ZOOM:
		m_pCurrentFont   = m_pZoomTextString->GetFont();
		m_pCurrentString = m_pZoomTextString;
		Remove(m_pEditIPString, false);
        Remove(m_pScreenTitle, false);
		m_pZoomTextString->DataSet(m_pEditIPString->DataGet());
		Add(m_pZoomTextString, false);
		break;
	case CDJPlayerState::NORMAL:
	default:
		m_pCurrentFont   = m_pEditIPString->GetFont();
		m_pCurrentString = m_pEditIPString;
		Remove(m_pZoomTextString, false);
		m_pEditIPString->DataSet(m_pZoomTextString->DataGet());
		Add(m_pEditIPString, false);
		Add(m_pScreenTitle, false);
		break;
	}
	if (m_pCurrentString->DataGet())
    {
		TCHAR* cGet = m_pCurrentString->DataGet();
		m_iCursor = tstrlen(cGet);
		m_CursorPos.x = m_pCurrentString->mReal.wLeft + TextWidth(cGet, m_pCurrentFont) + 1;
		m_pCurrentString->SetFirstVisible(0);
		while (m_CursorPos.x >= (m_pCurrentString->mClient.wRight - 5))
		{
			RetardCursor(*(cGet + m_pCurrentString->GetFirstVisible()));
			m_pCurrentString->SetFirstVisible(m_pCurrentString->GetFirstVisible() + 1);
		}
    }
	//if(Parent())
	//	Parent()->Draw();
    Draw();

    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:SynchWithViewMode -\n");
}

bool
CEditIPScreen::InsertKey(SIGNED iKey)
{
    DEBUGP( DBG_EDIT_IP_SCREEN, DBGLEV_TRACE, "eips:InsertKey (%d)\n", iKey);
    TCHAR *cBuff;
    TCHAR *cGet;
	TCHAR NewChar;
    cGet = m_pCurrentString->DataGet();
	SIGNED miMaxLen = 127;

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
        m_CurrentKey = iKey;
        m_Repeat     = 0;
        
        ShowSubKey(m_CurrentKey, m_Repeat);
        if (cGet)
		{
            if (m_bClearQuad)
            {
                int iStartQuad;
                int iEndQuad;
                for (iStartQuad = m_iCursor; (iStartQuad > 0) && (cGet[iStartQuad - 1] != '.'); --iStartQuad)
                    ;
                for (iEndQuad = m_iCursor; (cGet[iEndQuad]) && (cGet[iEndQuad] != '.'); ++iEndQuad)
                    ;
                
                for (; m_iCursor > iStartQuad; --m_iCursor)
                    RetardCursor(cGet[m_iCursor - 1]);

                cBuff = new TCHAR[tstrlen(cGet) + 2];
                tstrcpy(cBuff, cGet);
                tstrcpy(&cBuff[iStartQuad], &cGet[iEndQuad]);
                m_pCurrentString->DataSet(cBuff);
                delete cBuff;

                m_pCurrentString->SetMark((SIGNED)0, (SIGNED)0);
                m_bClearQuad = false;
            }

            cBuff = new TCHAR[tstrlen(cGet) + 2];
            tstrcpy(cBuff, cGet);
            NewChar = (TCHAR) m_IPKeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
            *(cBuff + m_iCursor) = NewChar;
            tstrcpy(cBuff + m_iCursor + 1, cGet + m_iCursor);
            m_pCurrentString->DataSet(cBuff);
            delete cBuff;
        }
        else
		{
            TCHAR cTemp[2];
            NewChar = (TCHAR) m_IPKeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
            cTemp[0] = NewChar;
            cTemp[1] = '\0';
            m_pCurrentString->DataSet(cTemp);
        }
        m_iCursor++;
        AdvanceCursor(NewChar);
        if (GetQuadSize() == 3)
            AdvanceQuad();
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
                if ((m_iCursor > 0) && (cGet[m_iCursor - 1] == '.'))
                {
                    RetardCursor(cGet[m_iCursor - 1]);
                    m_iCursor--;
                }

                if ((m_iCursor > 0) && (cGet[m_iCursor - 1] != '.'))
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
                }
                m_bClearQuad = false;
                m_pCurrentString->SetMark((SIGNED)0, (SIGNED)0);
                Invalidate();
            }
            break;

        case IR_KEY_CLEAR:
            m_iCursor = 0;
            m_CursorPos.x = m_pCurrentString->mClient.wLeft + 1;
            m_CursorPos.y = m_pCurrentString->mClient.wTop;
            m_pCurrentString->DataSet(LS(SID_BLANK_IP_ADDRESS));
            m_pCurrentString->SetFirstVisible(0);
            AdvanceQuad();
            Invalidate();
            break;

        case IR_KEY_NEXT:
            AdvanceQuad();
            Invalidate();
            break;

        case IR_KEY_PREV:
            RetardQuad();
            Invalidate();
            break;
        default:
            return FALSE;
        }
    }
    return (TRUE);
}


void
CEditIPScreen::SetTitleText(const TCHAR* szText)
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
    //Draw();
}

void
CEditIPScreen::SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType)
{
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
	Draw();
}

void
CEditIPScreen::SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType)
{
	m_pMessageTextString->SystemMessage(szText, iMessageType);
	Screen()->Invalidate(m_pMessageTextString->mReal);
	Draw();
}

void
CEditIPScreen::ShowSubKey(SIGNED iKey, unsigned int Repeat)
{
    SetMessageText(m_IPKeyTable.GetKeyValues(iKey), CSystemMessageString::STATUS);
    Repeat %= m_IPKeyTable.GetKeyValuesLength(iKey);
    m_pMessageTextString->SetMark(Repeat, Repeat + 1);
}

void
CEditIPScreen::AdvanceQuad()
{
    TCHAR* cGet = m_pCurrentString->DataGet();

    // Skip dot if at end of quad
    if (cGet[m_iCursor] == '.')
        AdvanceCursor(cGet[m_iCursor++]);
        
    // Locate next dot in string
    for (; cGet[m_iCursor] && (cGet[m_iCursor] != '.'); ++m_iCursor)
        AdvanceCursor(cGet[m_iCursor]);

    // Highlight this quad
    int iStartQuad;
    for (iStartQuad = m_iCursor; (iStartQuad > 0) && (cGet[iStartQuad - 1] != '.'); --iStartQuad)
        ;
    m_pCurrentString->SetMark(iStartQuad, m_iCursor);
    
    m_bClearQuad = true;
}

void
CEditIPScreen::RetardQuad()
{
    TCHAR* cGet = m_pCurrentString->DataGet();

    // Go to the beginning of the current quad
    int iStartQuad;
    for (iStartQuad = m_iCursor; (iStartQuad > 0) && (cGet[iStartQuad - 1] != '.'); --iStartQuad)
        ;
    // Skip the dot
    if (iStartQuad > 0)
        --iStartQuad;
    // Adjust the cursor
    for (; m_iCursor > iStartQuad; --m_iCursor)
        RetardCursor(cGet[m_iCursor - 1]);

    // Highlight this quad
    for (iStartQuad = m_iCursor; (iStartQuad > 0) && (cGet[iStartQuad - 1] != '.'); --iStartQuad)
        ;
    m_pCurrentString->SetMark(iStartQuad, m_iCursor);
        
    m_bClearQuad = true;
}

int
CEditIPScreen::GetQuadSize()
{
    TCHAR* cGet = m_pCurrentString->DataGet();

    int iQuadStart = 0;
    int iQuadEnd   = 0;
    for (int i = 0; cGet[i]; ++i, ++iQuadEnd)
    {
        if (cGet[i] == '.')
        {
            if (i < m_iCursor)
                iQuadStart = i + 1;
            else if (m_iCursor <= i)
                break;
        }
    }
    return (iQuadEnd - iQuadStart);
}

