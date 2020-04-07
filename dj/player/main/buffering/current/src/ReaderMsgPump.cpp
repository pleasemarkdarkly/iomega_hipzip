#include <main/buffering/BufferDebug.h>
#include <util/debug/debug.h>
#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>
#include <cyg/kernel/mutex.hxx>

#include <datastream/input/InputStream.h>

#include <main/buffering/BufferTypes.h>
#include <main/buffering/BufferReader.h>
#include <main/buffering/ReaderMsgPump.h>
#include <main/buffering/WriterMsgPump.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_READER_PUMP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_READER_PUMP);  // debugging prefix : (63) bd

CReaderMsgPump::CReaderMsgPump() : m_bPaused(false), m_pWorker(0)
{
    DEBUGP( DBG_READER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "brm:ctor\n");	
	m_ReaderStack = new char[CONSUMER_THREAD_STACK_SIZE];
	cyg_flag_init( &m_Flag );
	m_pReader = new CBufferReader();
	cyg_mutex_init( &m_mutReader );
	m_pThread = new Cyg_Thread( 9, // thread priority
								&CReaderMsgPump::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"BlockReader",
								(CYG_ADDRESS)m_ReaderStack,
								(cyg_ucount32)CONSUMER_THREAD_STACK_SIZE);
	m_pThread->resume();
}

CReaderMsgPump::~CReaderMsgPump()
{
    DEBUGP( DBG_READER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "brm:dtor\n");	
    m_bStopped = false;
    ShutDown();

    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }

	cyg_mutex_destroy( &m_mutReader );
	delete m_pThread;
	m_pThread = NULL;
	delete m_pReader;
	m_pReader = NULL;
	delete [] m_ReaderStack;
	m_ReaderStack = NULL;
}

int CReaderMsgPump::Seek(eInputSeekPos Origin, int Offset)
{
	if ((IInputStream::InputSeekPos)Origin==IInputStream::SeekStart) {
		DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:sk %d start\n",Offset);	
	}
    else if ((IInputStream::InputSeekPos)Origin==IInputStream::SeekCurrent) {
		DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:sk %d current\n",Offset);
	}
	else if ((IInputStream::InputSeekPos)Origin==IInputStream::SeekEnd) {
		DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:sk %d end\n",Offset);
	}
	cyg_mutex_lock( &m_mutReader );
	m_nSeekOffset = Offset;
	m_SeekOrigin = Origin;
	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
	cyg_flag_setbits( &m_Flag, FLAG_SEEK );
	cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
	cyg_mutex_unlock( &m_mutReader );
	return m_nSeekReturn;
}

int CReaderMsgPump::Read(void* pBlock, int nBytes)
{
    DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:rd %d\n",nBytes);	
	cyg_mutex_lock( &m_mutReader );
	m_pReadBlock = pBlock;
	m_nReadBytes = nBytes;
	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
	cyg_flag_setbits( &m_Flag, FLAG_READ );
	cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
	cyg_mutex_unlock( &m_mutReader );
	return m_nReadReturn;
}

void CReaderMsgPump::ThreadEntry(CYG_ADDRESS data)
{
	CReaderMsgPump* pThis = reinterpret_cast<CReaderMsgPump*>(data);
	pThis->ThreadFunc();
}

void CReaderMsgPump::ThreadFunc()
{
	cyg_flag_value_t flagval;
	while (1)
	{
        DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:wt\n");	
    	cyg_flag_setbits( &m_Flag, FLAG_DONE );
        flagval = cyg_flag_wait( &m_Flag, FLAG_READ | FLAG_SEEK | FLAG_SHUTDOWN | FLAG_PAUSE, CYG_FLAG_WAITMODE_OR );
    	cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
		if ( flagval & FLAG_READ )
		{
            DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:flg rd\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_READ );
			m_nReadReturn = m_pReader->Read(m_pReadBlock,m_nReadBytes);
		}
		else if ( flagval & FLAG_SEEK )
		{
            DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:flg sk\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_SEEK );
			m_nSeekReturn = m_pReader->Seek(m_SeekOrigin, m_nSeekOffset);
		}
        else if ( flagval & FLAG_PAUSE )
        {
            DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:flg ps\n");	
            cyg_flag_maskbits( &m_Flag, ~FLAG_PAUSE );
            cyg_flag_wait( &m_Flag, FLAG_RESUME, CYG_FLAG_WAITMODE_OR );
            cyg_flag_maskbits( &m_Flag, ~FLAG_RESUME );
        }
		else if ( flagval & FLAG_SHUTDOWN )
		{
            DEBUGP( DBG_READER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "brm:flg shdn\n");	
			cyg_flag_maskbits( &m_Flag, ~FLAG_SHUTDOWN );
			break;
		}
		else {
			DEBUGP( DBG_READER_PUMP, DBGLEV_ERROR, "brm:unhandled flag value in consumer msg pump\n");
		}
	}
    m_bStopped = true;
}

CBufferReader* CReaderMsgPump::GetReader()
{
	return m_pReader;
}

void CReaderMsgPump::PauseReaderThread()
{
    DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:pause\n");
    cyg_mutex_lock( &m_mutReader );
	if (!m_bPaused)
    {
        m_bPaused = true;
        cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
        cyg_flag_setbits( &m_Flag, FLAG_PAUSE );
    }
    cyg_mutex_unlock( &m_mutReader );
}

void CReaderMsgPump::ResumeReaderThread()
{
    DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:resume\n");
	if (m_bPaused)
    {
        cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
        cyg_flag_setbits( &m_Flag, FLAG_RESUME );
        cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR );
        m_bPaused = false;
    }
}

void CReaderMsgPump::ShutDown()
{
	cyg_mutex_lock( &m_mutReader );
	DEBUGP( DBG_READER_PUMP, DBGLEV_VERBOSE, "brm:set shutdown\n");
	cyg_flag_setbits( &m_Flag, FLAG_SHUTDOWN );
	cyg_mutex_unlock( &m_mutReader );
}

void CReaderMsgPump::JogDoneFlag()
{
    cyg_flag_setbits( &m_Flag, FLAG_DONE );
}

void CReaderMsgPump::SetWorker(CBufferWorker* pWkr)
{
    m_pWorker = pWkr;
    m_pReader->SetWorker(pWkr);
}
