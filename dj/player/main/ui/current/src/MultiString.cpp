// MultiString.cpp: A variant of PegString that allows the queing of strings 
// danb@iobjects.com 12/31/01
// (c) Interactive Objects

#include <main/ui/MultiString.h>
#include <main/ui/Timers.h>
#include <stdlib.h>
#include <util/diag/diag.h>
#include <util/debug/debug.h>
DEBUG_MODULE_S( MULTISTRING, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( MULTISTRING );

// this is so that multiple instances of this class can be used and
// their timers won't interfere with eachother
WORD CMultiString::s_uiInstaceCounter = 0;

CMultiString::CMultiString(const PegRect &Rect, const TCHAR *Text, WORD wId,
    WORD wStyle, SIGNED iLen) :
    PegString(Rect, Text, wId, wStyle, iLen),
    m_bTimerIsActive(false),
    m_szPersistentString(NULL)
{
    cyg_mutex_init(&m_mtxMultiString);
    m_uiInstance = s_uiInstaceCounter++;
    SavePersistentString(Text);
}

CMultiString::CMultiString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text, WORD wId, 
    WORD wStyle, SIGNED iLen) :
    PegString(iLeft, iTop, Text, wId, wStyle, iLen),
    m_bTimerIsActive(false),
    m_szPersistentString(NULL)
{
    cyg_mutex_init(&m_mtxMultiString);
    m_uiInstance = s_uiInstaceCounter++;
    SavePersistentString(Text);
}

CMultiString::CMultiString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth, 
    const TCHAR *Text, WORD wId, WORD wStyle, SIGNED iLen) : 
    PegString(iLeft, iTop, iWidth, Text, wId, wStyle, iLen),
    m_bTimerIsActive(false),
    m_szPersistentString(NULL)
{
    cyg_mutex_init(&m_mtxMultiString);
    m_uiInstance = s_uiInstaceCounter++;
    SavePersistentString(Text);
}

CMultiString::~CMultiString()
{
    ClearMessageQueue();
    cyg_mutex_destroy(&m_mtxMultiString);
    SavePersistentString(0);
}

WORD CMultiString::_TIMER()
{
    return TIMER_MULTISTRING + m_uiInstance;
}

SIGNED CMultiString::Message(const PegMessage &Mesg)
{
    switch(Mesg.wType)
    {
	case PM_TIMER:
		if (Mesg.iData == _TIMER())
		{
            DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:Timer\n");
            SetNextMessage();
            return 0;
		}
	default:
		break;
    }
    return PegString::Message(Mesg);
}

void CMultiString::DataSet(const TCHAR *Text)
{
	cyg_mutex_lock(&m_mtxMultiString);
    KillTimer(_TIMER());
    m_bTimerIsActive = false;
    ClearMessageQueue();
    PegString::DataSet(Text);
    SavePersistentString(Text);
    cyg_mutex_unlock(&m_mtxMultiString);
}

void CMultiString::DataSet(const TCHAR *Text, int iPriority, int iTimeout)
{
    // add the new message to the message array
    if (Text)
    {
        // create a new message
        message_t msg;
        msg.szString = (TCHAR*)malloc((tstrlen(Text) + 1)* sizeof(TCHAR));
        if(msg.szString)
            tstrcpy(msg.szString, Text);
        msg.iPriority = iPriority;
        msg.iTimeout = iTimeout;

        EnqueMessage(msg);
    }
    else
        DataSet(Text);
}

void CMultiString::EnqueMessage(message_t &msg)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:EnqueMessage\n");
    //print_mem_usage();
	cyg_mutex_lock(&m_mtxMultiString);
    // find where it should go in the list by it's priority
    if(m_lstMessages.IsEmpty())
    {
        m_lstMessages.PushFront(msg);
    }
    else
    {
        MessageListIterator it = m_lstMessages.GetTail();
        while(it != m_lstMessages.GetEnd())
        {
            if((*it).iPriority <= msg.iPriority)
            {
                m_lstMessages.Insert(msg, it);
                it = 0;
            }
            else if(it == m_lstMessages.GetHead())
            {
                m_lstMessages.PushFront(msg);
                it = 0;
            }
            else 
                ++it;
        }
    }

    // start the timer to process these messages if it's not active
    if(m_bTimerIsActive == false)
    {
        SetTimer(_TIMER(), 1, 0);
        m_bTimerIsActive = true;
    }

    // todo:  delete all other messages of a lower priority
    // if this message isn't supposed to time out?
    cyg_mutex_unlock(&m_mtxMultiString);
}

void CMultiString::RemoveMessage(MessageListIterator it)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:RemoveMessage\n");
	cyg_mutex_lock(&m_mtxMultiString);
    message_t msg = m_lstMessages.Remove(it);
    if(msg.szString)
        free(msg.szString);
    cyg_mutex_unlock(&m_mtxMultiString);
}

void CMultiString::ClearMessageQueue()
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:ClearMessageQueue\n");
    while(!m_lstMessages.IsEmpty())
        RemoveMessage(m_lstMessages.GetTail());
}

void CMultiString::ClearMessagesOfPriority(int iPriority)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:ClearMessagesOfPriority\n");
    if(!m_lstMessages.IsEmpty())
    {
        MessageListIterator it = m_lstMessages.GetHead();
        while (it != m_lstMessages.GetEnd())
        {
            MessageListIterator next = it + 1;
            if ((*it).iPriority == iPriority)
                RemoveMessage(it);
            it = next;
        }
    }
}

void CMultiString::DeleteNextMessage()
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:DeleteNextMessage\n");
    if(!m_lstMessages.IsEmpty())
    {
        // get the highest priority message
        MessageListIterator it = m_lstMessages.GetHead();
        RemoveMessage(it);
    }
}

void CMultiString::DeleteNextMessageOfPriority(int iPriority)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:DeleteNextMessageOfPriority\n");
    if(!m_lstMessages.IsEmpty())
    {
        MessageListIterator it = m_lstMessages.GetHead();
        while (it != m_lstMessages.GetEnd())
        {
            MessageListIterator next = it + 1;
            if ((*it).iPriority == iPriority)
            {
                RemoveMessage(it);
                return;
            }
            it = next;
        }
    }
}

void CMultiString::DeleteLastMessage()
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:DeleteLastMessage\n");
    /* implement me */
}

void CMultiString::DeleteLastMessageOfPriority(int iPriority)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:DeleteLastMessageOfPriority\n");
    /* implement me */
}

int CMultiString::SizeOfQueue()
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:SizeOfQueue\n");
    return m_lstMessages.Size();
}

int CMultiString::SizeOfQueueOfPriority(int iPriority)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:SizeOfQueueOfPriority\n");
    if(!m_lstMessages.IsEmpty())
    {
        int iSize = 0;
        MessageListIterator it = m_lstMessages.GetHead();
        while (it != m_lstMessages.GetEnd())
        {
            if ((*it).iPriority == iPriority)
                iSize++;
            ++it;
        }
        return iSize;
    }
    else
        return 0;
}

    
void CMultiString::SavePersistentString(const TCHAR* Text)
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:SavePersistentString\n");
    if (m_szPersistentString == Text)
    {
        DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:SavePersistentString: No Saving the Same String Twice.  Bad.\n");
        return;
    }

    if (m_szPersistentString)
    {
        free((TCHAR*)m_szPersistentString);
        m_szPersistentString = NULL;
    }

    if (Text)
    {
        m_szPersistentString = (TCHAR*)malloc((tstrlen(Text) + 1) * sizeof(TCHAR));
        if (m_szPersistentString)
            tstrcpy(m_szPersistentString, Text);
    }

    if(m_bTimerIsActive == false)
        PegString::DataSet(Text);
}

void CMultiString::SetNextMessage()
{
    DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:SetNextMessage\n");

    if(m_lstMessages.IsEmpty())
    {
        // if there are no messages, restore the persistent string
        DataSet(m_szPersistentString);
    }
    else
    {
        KillTimer(_TIMER());
        m_bTimerIsActive = false;

        // display the highest priority message
        MessageListIterator it = m_lstMessages.GetHead();
        PegString::DataSet((*it).szString);

        char szTemp[256];
        DEBUGP( MULTISTRING, DBGLEV_TRACE, "MultiString:Display Queued Message: %s\n", TcharToCharN(szTemp, (*it).szString, 255));

        // if this message doesn't time out, flush the remaining messages
        if((*it).iTimeout <= 0)
        {
            // clear the message queue
            while(!m_lstMessages.IsEmpty())
                RemoveMessage(m_lstMessages.GetTail());
            SavePersistentString((*it).szString);
        }
        else
        {
            SetTimer(_TIMER(), (*it).iTimeout, 0);
            m_bTimerIsActive = true;
        }

        // we've serviced this message, now remove it
        RemoveMessage(it);
    }
    Invalidate(mReal);
    if(Parent())
        if (Presentation()->GetCurrentThing() == Parent())
            Parent()->Draw();
    else
        Draw();
}
