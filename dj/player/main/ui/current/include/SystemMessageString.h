// SystemMessageString.h: A variant of PegString that allows the queing of strings 
// danb@iobjects.com 12/31/01
// (c) Interactive Objects

#ifndef SYSTEMMESSAGESTRING_H_
#define SYSTEMMESSAGESTRING_H_

#include <main/ui/MultiString.h>

class CSystemMessageString : public CMultiString
{
public:
    CSystemMessageString(const PegRect &Rect, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    CSystemMessageString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth,
        const TCHAR *Text = NULL, WORD wId = 0, 
        WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT, SIGNED iLen = -1);
    
    CSystemMessageString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    virtual ~CSystemMessageString();
    
    typedef enum SysMsgType { STATUS = 0, INFO, REALTIME_INFO };

    void SystemMessage(const char *Text, SysMsgType Type);
    void SystemMessage(const TCHAR *Text, SysMsgType Type);
};

#endif  // SYSTEMMESSAGESTRING_H_
