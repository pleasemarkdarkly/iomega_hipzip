//
// EditString.cpp: implementation of CEditString class
// chuckf@fullplaymedia.com 12/05/01
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/EditString.h>
#include <main/ui/Keys.h>

#define CURSOR_ADVANCE_TIMEOUT 15

static const Key_t Key0 = {2, " 0"};
static const Key_t Key1 = {33, "!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~1"};
static const Key_t Key2 = {4, "abc2"};
static const Key_t Key3 = {4, "def3"};
static const Key_t Key4 = {4, "ghi4"};
static const Key_t Key5 = {4, "jkl5"};
static const Key_t Key6 = {4, "mno6"};
static const Key_t Key7 = {5, "pqrs7"};
static const Key_t Key8 = {4, "tuv8"};
static const Key_t Key9 = {5, "wxyz9"};

CKeyTable::CKeyTable()
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

char CKeyTable::GetKey(unsigned int KeyNum, unsigned int KeyRepeated, bool bCaps)
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
	
CEditString::CEditString() :
PegString(0,0,0,NULL,0,FF_NONE|AF_ENABLED|EF_EDIT,-1),
m_bCaps(false),
m_bShowCursor(true),
mp_ReturnText(NULL),
m_CurrentKey(0) {}

CEditString::CEditString(const PegRect &Position, PEGCHAR **pText, PegFont *pFont) :
PegString(Position, *pText, 0, FF_NONE|AF_ENABLED|EF_EDIT),
m_bCaps(false),
m_bShowCursor(true),
mp_ReturnText(pText),
m_CurrentKey(0)
{
	SetFont(pFont);
}

CEditString::~CEditString()
{
}

void CEditString::Draw()
{

	PegColor Color(muColors[PCI_NORMAL], muColors[PCI_NORMAL], CF_FILL);
	BeginDraw();
	Rectangle(mReal, Color, 0);
	Color.Set(muColors[PCI_NTEXT], muColors[PCI_NORMAL], CF_FILL);
	PegPoint Point;
    Point.y = mClient.wTop;
    Point.x = mClient.wLeft + 1;

    PegRect OldClip = mClip;
    mClip &= mClient;

    if (mpText)
    {
		DrawText(Point, mpText + miFirstVisibleChar,
                Color, mpFont);
    }
	if(m_bShowCursor)
	{
		Line(mCursorPos.x, mClient.wTop + 1, mCursorPos.x, mClient.wBottom - 1, Color);
	}
    mClip = OldClip;

    if (First())
    {
        DrawChildren();
    }
	DrawChildren();
	EndDraw();
}

SIGNED CEditString::Message(const PegMessage &Mesg)
{
    switch(Mesg.wType)
    {
	case PM_TIMER:
        if( == TIMER_EDITSTRING)
		m_bShowCursor = true;
		Invalidate();
		Draw();
		break;
    case PM_CURRENT:
        PegThing::Message(Mesg);
        if (DataGet())
        {
			PEGCHAR* cGet = DataGet();
            miMarkStart = 0;
            miMarkEnd = strlen(cGet);
            miCursor = miMarkEnd ;
            mCursorPos.x = mClient.wLeft + TextWidth(cGet, mpFont);
            miFirstVisibleChar = 0;
            while (mCursorPos.x >= mClient.wRight)
            {
                RetardCursor(*(cGet + miFirstVisibleChar));
                miFirstVisibleChar++;
            }
        }
        else
        {
            miCursor = 0;
            mCursorPos.x = mClient.wLeft + 1;
            mCursorPos.y = mClient.wTop;
        }
        Invalidate(mClient);
        Draw();
        break;
    case PM_KEY:
		if (InsertKey(Mesg.iData))
		{
			Draw();
			CheckSendSignal(PSF_TEXT_EDIT);
		}
		else
		{
			switch(Mesg.iData)
			{
			case IR_KEY_SELECT:
				CheckSendSignal(PSF_TEXT_EDITDONE);
				if (DataGet())
				{
					delete *mp_ReturnText;
					*mp_ReturnText = new PEGCHAR[strlen(DataGet()) + 1];
					strcpy(*mp_ReturnText, DataGet());
					Parent()->Invalidate();
					Remove(this);
				}
				break;
			case IR_KEY_EXIT:
				Remove(this);
				break;
			case IR_KEY_SHIFT:
				m_bCaps = !m_bCaps;
				break;
			default:
				PegThing::Message(Mesg);
				break;
			}
		}
        break;
    default:
        PegThing::Message(Mesg);
        break;
    }
    return 0;
}

BOOL CEditString::InsertKey(SIGNED iKey)
{
    PEGCHAR *cBuff;
    PEGCHAR *cGet;
	PEGCHAR NewChar;
    cGet = DataGet();

    if (iKey >= IR_KEY_0_space && iKey <= IR_KEY_9_wxyz)
    {
        if (miMaxLen > 0)
        {
            if (cGet)
            {
                if ((SIGNED) strlen(cGet) >= miMaxLen)
                {
                    return FALSE;
                }
            }
        }
		if(m_bShowCursor)
		{
			m_CurrentKey = iKey;
			m_Repeat = 0;
			if (cGet)
			{
				cBuff = new PEGCHAR[strlen(cGet) + 2];
				strcpy(cBuff, cGet);
				NewChar = (PEGCHAR) m_KeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
				*(cBuff + miCursor) = NewChar;
				strcpy(cBuff + miCursor + 1, cGet + miCursor);
				DataSet(cBuff);
				delete cBuff;
			}
			else
			{
				PEGCHAR cTemp[2];
				NewChar = (PEGCHAR) m_KeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
				cTemp[0] = NewChar;
				cTemp[1] = '\0';
				DataSet(cTemp);
			}
			m_bShowCursor = false;
			SetTimer(TIMER_EDITSTRING, CURSOR_ADVANCE_TIMEOUT, 0);
			miCursor++;
			AdvanceCursor(NewChar);
			State.mbChanged = 1;
		}
		else
		{
			RetardCursor(cGet[miCursor-1]);
			if(m_CurrentKey == iKey)
			{
				cGet[miCursor-1] = (PEGCHAR) m_KeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
				AdvanceCursor(cGet[miCursor-1]);
			}
			else
			{
				AdvanceCursor(cGet[miCursor-1]);
				m_Repeat = 0;
				m_CurrentKey = iKey;
				cBuff = new PEGCHAR[strlen(cGet) + 2];
				strcpy(cBuff, cGet);
				NewChar = (PEGCHAR) m_KeyTable.GetKey(iKey, m_Repeat++, m_bCaps);
				*(cBuff + miCursor) = NewChar;
				strcpy(cBuff + miCursor + 1, cGet + miCursor);
				DataSet(cBuff);
				delete cBuff;
				miCursor++;
				State.mbChanged = 1;
				AdvanceCursor(NewChar);
			}
			Invalidate();
			KillTimer(TIMER_EDITSTRING);
			SetTimer(TIMER_EDITSTRING, CURSOR_ADVANCE_TIMEOUT, 0);
		}
        return TRUE;
    }
    else
    {
        switch(iKey)
        {
        case IR_KEY_DELETE:
            if (miCursor && cGet)
            {
                cBuff = new PEGCHAR[strlen(cGet) + 2];
                strcpy(cBuff, cGet);
                RetardCursor(*(cBuff + miCursor - 1));
                strcpy(cBuff + miCursor - 1, cGet + miCursor);
                miCursor--;
                DataSet(cBuff);
                delete cBuff;
            }
            break;
        case IR_KEY_NEXT:
            if (cGet)
            {
                if (miCursor < (SIGNED) strlen(cGet))
                {
                    AdvanceCursor(*(cGet + miCursor));
                    miCursor++;
                    Invalidate();
                }
            }
            break;

        case IR_KEY_PREV:
            if (miCursor && cGet)
            {
                RetardCursor(*(cGet + miCursor - 1));
                miCursor--;
                Invalidate();
            }
            break;
        default:
            return FALSE;
        }
    }
    State.mbChanged = 1;
    return (TRUE);
}
