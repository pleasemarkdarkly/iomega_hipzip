#include <util/debug/debug.h>
#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>
#include <cyg/kernel/mutex.hxx>

#include <datastream/input/InputStream.h>

#include <main/datastream/fatfile/Consumer.h>
#include <main/datastream/fatfile/ConsumerMsgPump.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_CONSUMER_PUMP, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_CONSUMER_PUMP);

CConsumerMsgPump::CConsumerMsgPump() : m_bPaused(false)
{
    DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:ctor\n");	
	m_ConsumerStack = new char[CONSUMER_THREAD_STACK_SIZE];
	cyg_flag_init( &m_Flag );
	m_pConsumer = new CConsumer();
	cyg_mutex_init( &m_mutConsumer );
	m_pThread = new Cyg_Thread( 10, //schedule priority
								&CConsumerMsgPump::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"BufferConsumer",
								(CYG_ADDRESS)m_ConsumerStack,
								(cyg_ucount32)CONSUMER_THREAD_STACK_SIZE);
	m_pThread->resume();
}

CConsumerMsgPump::~CConsumerMsgPump()
{
    DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:dtor\n");	
	cyg_mutex_destroy( &m_mutConsumer );
	delete m_pThread;
	m_pThread = NULL;
	delete m_pConsumer;
	m_pConsumer = NULL;
	delete [] m_ConsumerStack;
	m_ConsumerStack = NULL;
}

int CConsumerMsgPump::Seek(IInputStream::InputSeekPos Origin, int Offset)
{
	if (Origin==IInputStream::SeekStart) {
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:sk %d start\n",Offset);	
	}
    else if (Origin==IInputStream::SeekCurrent) {
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:sk %d current\n",Offset);
	}
	else if (Origin==IInputStream::SeekEnd) {
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:sk %d end\n",Offset);
	}
	cyg_mutex_lock( &m_mutConsumer );
	m_nSeekOffset = Offset;
	m_SeekOrigin = Origin;
	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
	cyg_flag_setbits( &m_Flag, FLAG_SEEK );
	cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
	cyg_mutex_unlock( &m_mutConsumer );
	return m_nSeekReturn;
}

int CConsumerMsgPump::Read(void* pBuffer, int nBytes)
{
    DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:rd %d\n",nBytes);	
	cyg_mutex_lock( &m_mutConsumer );
	m_pReadBuffer = pBuffer;
	m_nReadBytes = nBytes;
	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
	cyg_flag_setbits( &m_Flag, FLAG_READ );
	cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
	cyg_mutex_unlock( &m_mutConsumer );
	return m_nReadReturn;
}

void CConsumerMsgPump::ThreadEntry(CYG_ADDRESS data)
{
	CConsumerMsgPump* pThis = reinterpret_cast<CConsumerMsgPump*>(data);
	pThis->ThreadFunc();
}

void CConsumerMsgPump::ThreadFunc()
{
	cyg_flag_value_t flagval;
	while (1)
	{
        DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:wt\n");	
    	cyg_flag_setbits( &m_Flag, FLAG_DONE );
        flagval = cyg_flag_wait( &m_Flag, FLAG_READ | FLAG_SEEK | FLAG_SHUTDOWN | FLAG_PAUSE, CYG_FLAG_WAITMODE_OR );
    	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
		if ( flagval & FLAG_READ )
		{
            DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:flg rd\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_READ );
			m_nReadReturn = m_pConsumer->Read(m_pReadBuffer,m_nReadBytes);
		}
		else if ( flagval & FLAG_SEEK )
		{
            DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:flg sk\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_SEEK );
			m_nSeekReturn = m_pConsumer->Seek(m_SeekOrigin, m_nSeekOffset);
		}
        else if ( flagval & FLAG_PAUSE )
        {
            DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:flg ps\n");	
            cyg_flag_maskbits( &m_Flag, ~FLAG_PAUSE );
            cyg_flag_wait( &m_Flag, FLAG_RESUME, CYG_FLAG_WAITMODE_OR );
            cyg_flag_maskbits( &m_Flag, ~FLAG_RESUME );
        }
		else if ( flagval & FLAG_SHUTDOWN )
		{
            DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:flg shdn\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_SHUTDOWN );
			break;
		}
		else {
			DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_ERROR, "CP:unhandled flag value in consumer msg pump\n");
		}
	}
}

CConsumer* CConsumerMsgPump::GetConsumer()
{
	return m_pConsumer;
}

void CConsumerMsgPump::PauseConsumerThread()
{
    DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:pause\n");
    cyg_mutex_lock( &m_mutConsumer );
	if (!m_bPaused)
    {
        m_bPaused = true;
        cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
        cyg_flag_setbits( &m_Flag, FLAG_PAUSE );
    }
    cyg_mutex_unlock( &m_mutConsumer );
}

void CConsumerMsgPump::ResumeConsumerThread()
{
    DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_TRACE, "cp:resume\n");
    cyg_flag_setbits( &m_Flag, FLAG_DONE );
    cyg_mutex_lock( &m_mutConsumer );
	if (m_bPaused)
    {
        cyg_flag_setbits( &m_Flag, FLAG_RESUME );
        m_bPaused = false;
    }
    cyg_mutex_unlock( &m_mutConsumer );
}

void CConsumerMsgPump::JogDoneFlag()
{
    cyg_flag_setbits( &m_Flag, FLAG_DONE );
}
