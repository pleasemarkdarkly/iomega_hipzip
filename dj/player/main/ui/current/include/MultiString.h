// MultiString.h: A variant of PegString that allows the queing of strings 
// danb@iobjects.com 12/31/01
// (c) Interactive Objects

#ifndef MULTISTRING_H_
#define MULTISTRING_H_

#include <gui/peg/peg.hpp>
//#include <gui/peg/pstring.hpp>
#include <util/datastructures/SimpleList.h>
#include <cyg/kernel/kapi.h>

class CMultiString : public PegString
{
public:
    CMultiString(const PegRect &Rect, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    CMultiString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth,
        const TCHAR *Text = NULL, WORD wId = 0, 
        WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT, SIGNED iLen = -1);
    
    CMultiString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text = NULL,
        WORD wId = 0, WORD wStyle = FF_RECESSED|AF_ENABLED|EF_EDIT,
        SIGNED iLen = -1);
    
    virtual ~CMultiString();
    
    virtual void DataSet(const TCHAR *Text);
    virtual void DataSet(const TCHAR *Text, int iPriority, int iTimeout = 0);
    virtual SIGNED Message(const PegMessage &Mesg);
    
protected:
    void ClearMessageQueue();
    void ClearMessagesOfPriority(int iPriority);
    void SavePersistentString(const TCHAR* szString);
    void SetNextMessage();
    void DeleteNextMessage();
    void DeleteNextMessageOfPriority(int iPriority);
    void DeleteLastMessage();
    void DeleteLastMessageOfPriority(int iPriority);
    int SizeOfQueue();
    int SizeOfQueueOfPriority(int iPriority);

private:
    typedef struct message_s {
        TCHAR*  szString;
        int     iPriority;
        int     iTimeout;
    } message_t;
    typedef SimpleList<message_t> MessageList;
    typedef SimpleListIterator<message_t> MessageListIterator;

    void EnqueMessage(message_t &msg);
    void RemoveMessage(MessageListIterator it);
    WORD _TIMER();

    MessageList m_lstMessages;

    cyg_mutex_t m_mtxMultiString;

    bool m_bTimerIsActive;

    TCHAR* m_szPersistentString;

    static WORD s_uiInstaceCounter;
    WORD m_uiInstance;
};

#endif  // MULTISTRING_H_
