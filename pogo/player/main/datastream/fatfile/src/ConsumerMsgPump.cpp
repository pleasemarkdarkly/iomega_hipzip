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
DEBUG_MODULE_S(DBG_CONSUMER_PUMP, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_CONSUMER_PUMP);

CConsumerMsgPump::CConsumerMsgPump()
{
	m_ConsumerStack = new char[CONSUMER_THREAD_STACK_SIZE];
	cyg_flag_init( &m_Flag );
	m_pThread = new Cyg_Thread( 10, //schedule priority
								&CConsumerMsgPump::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"BufferConsumer",
								(CYG_ADDRESS)m_ConsumerStack,
								(cyg_ucount32)CONSUMER_THREAD_STACK_SIZE);
	m_pThread->resume();
	m_pConsumer = new CConsumer();
	cyg_mutex_init( &m_mutConsumer );
}

CConsumerMsgPump::~CConsumerMsgPump()
{
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
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_INFO, "Seek %d from start\n",Offset,Origin);	
	}
    else if (Origin==IInputStream::SeekCurrent) {
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_INFO, "Seek %d from current\n",Offset,Origin);
	}
	else if (Origin==IInputStream::SeekEnd) {
		DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_INFO, "Seek %d from end\n",Offset,Origin);
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
		flagval = cyg_flag_wait( &m_Flag, FLAG_READ | FLAG_SEEK | FLAG_SHUTDOWN , CYG_FLAG_WAITMODE_OR );
		if ( flagval & FLAG_READ )
		{
			cyg_flag_maskbits( &m_Flag, ~FLAG_READ );
			m_nReadReturn = m_pConsumer->Read(m_pReadBuffer,m_nReadBytes);
			cyg_flag_setbits( &m_Flag, FLAG_DONE );
		}
		else if ( flagval & FLAG_SEEK )
		{
			cyg_flag_maskbits( &m_Flag, ~FLAG_SEEK );
			m_nSeekReturn = m_pConsumer->Seek(m_SeekOrigin, m_nSeekOffset);
			cyg_flag_setbits( &m_Flag, FLAG_DONE );
		}
		else if ( flagval & FLAG_SHUTDOWN )
		{
			cyg_flag_maskbits( &m_Flag, ~FLAG_SHUTDOWN );
			break;
		}
		else {
			DEBUGP( DBG_CONSUMER_PUMP, DBGLEV_INFO, "unhandled flag value in prod msg pump\n");
		}
	}
}

CConsumer* CConsumerMsgPump::GetConsumer()
{
	return m_pConsumer;
}
