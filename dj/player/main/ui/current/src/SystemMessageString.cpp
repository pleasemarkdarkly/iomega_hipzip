// SystemMessageString.cpp: A variant of MultiString that allows the queing of strings 
// danb@iobjects.com 12/31/01
// (c) Interactive Objects

#include <main/ui/SystemMessageString.h>
#include <stdlib.h>

CSystemMessageString::CSystemMessageString(const PegRect &Rect, const TCHAR *Text, WORD wId,
    WORD wStyle, SIGNED iLen) :
    CMultiString(Rect, Text, wId, wStyle, iLen)
{
}

CSystemMessageString::CSystemMessageString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text, WORD wId, 
    WORD wStyle, SIGNED iLen) :
    CMultiString(iLeft, iTop, Text, wId, wStyle, iLen)
{
}

CSystemMessageString::CSystemMessageString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth, 
    const TCHAR *Text, WORD wId, WORD wStyle, SIGNED iLen) : 
    CMultiString(iLeft, iTop, iWidth, Text, wId, wStyle, iLen)
{
}

CSystemMessageString::~CSystemMessageString()
{
}

void CSystemMessageString::SystemMessage(const char *Text, SysMsgType Type)
{
    TCHAR* szTemp = (TCHAR*)malloc(sizeof(TCHAR) * (strlen(Text) + 1));
    SystemMessage(CharToTchar(szTemp, Text), Type);
    free(szTemp);
}

void CSystemMessageString::SystemMessage(const TCHAR *Text, SysMsgType Type)
{
    switch (Type)
    {
    case STATUS:
        SavePersistentString(Text);
        Invalidate(mReal);
        if(Parent())
            if (Presentation()->GetCurrentThing() == Parent())
                Parent()->Draw();
        else
            Draw();
        break;
    case INFO:
        DataSet(Text, 1, 300);
        if (SizeOfQueue() > 10)
            DeleteNextMessageOfPriority(1);
        break;
    case REALTIME_INFO:
        ClearMessagesOfPriority(2);
        DataSet(Text, 2, 300);
        SetNextMessage();
        break;
    default:
        break;
    }
}
