#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>
//#include "DriveInterface.h"
#include <util/debug/debug.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_PRODUCER_PUMP, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(DBG_PRODUCER_PUMP);

// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
//  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
//extern void NotifyBufferReadBegin();
// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
//  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
//extern void NotifyBufferReadEnd();
cyg_mutex_t	CProducerMsgPump::m_mutProducer;

CProducerMsgPump::CProducerMsgPump() : m_pThread(0), m_bTimeToRest(false), m_bAggressiveMode(false), m_bFillMode(false), 
                                       m_bPaused(false), m_bStarted(false), m_nBufsFilledAtLowPrio(0)
{
	m_Stack = new char[PRODUCER_THREAD_STACK_SIZE];
	DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:flag init\n");
	cyg_flag_init( &m_Flag );
	m_pThread = new Cyg_Thread( HIGH_BUFFERING_THREAD_PRIO, //schedule priority
								&CProducerMsgPump::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"BufferProducer",
								(CYG_ADDRESS)m_Stack,
								(cyg_ucount32)PRODUCER_THREAD_STACK_SIZE);
	m_pThread->resume();
	cyg_mutex_init( &m_mutProducer );
	m_pProducer = new CProducer;
}

CProducerMsgPump::~CProducerMsgPump()
{
	cyg_mutex_destroy( &m_mutProducer );
	DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:flag destroy\n");
	cyg_flag_destroy( &m_Flag );
	delete m_pProducer;
	delete [] m_Stack;
	delete m_pThread;
}

void CProducerMsgPump::ThreadEntry(CYG_ADDRESS data)
{
	DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:Producer thread entry\n");
	CProducerMsgPump* pThis = reinterpret_cast<CProducerMsgPump*>(data);
	pThis->ThreadFunc();
}

void CProducerMsgPump::ThreadFunc()
{
	cyg_flag_value_t flagval;
	int nCharsFilled = 0;
	while (1)
	{
		DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:wait any\n");
		flagval = cyg_flag_wait( &m_Flag, FLAG_FILLMODE | FLAG_SHUTDOWN | FLAG_PAUSE, CYG_FLAG_WAITMODE_OR );

		if ( flagval & FLAG_FILLMODE )
		{
			nCharsFilled = m_pProducer->FillBuffer();
			if (nCharsFilled == -1 || m_bTimeToRest)
			{
				m_bTimeToRest = false;
				DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:producer finished\n");
				// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
                //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
                //NotifyBufferReadEnd();
				DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:mask fillmode\n");
				cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
				m_bFillMode = false;
				m_bProducing = false;
				DEBUG_SLEEP_DRIVE;
				cyg_thread_delay(20);
				while (!IsDriveAsleep())
					SleepDrive();
			}
			else if (!m_bAggressiveMode)
			{
				m_nBufsFilledAtLowPrio++;
				if (m_nBufsFilledAtLowPrio >= BUFFERS_TO_FILL_AT_LOW_PRIORITY)
				{		
					m_nBufsFilledAtLowPrio = 0;
				    // (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
                    //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
					//NotifyBufferReadEnd();
					SetAggressiveMode();
					DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:mask fillmode\n");
					cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
					m_bFillMode = false;
					m_bProducing = false;
					DEBUG_SLEEP_DRIVE;
					cyg_thread_delay(20);
					SleepDrive();
					SleepDrive();
				}
			}
			else
			{
				m_nBufsFilledAtLowPrio = 0;
			}
		}
		else if ( flagval & FLAG_PAUSE )
		{
			m_nBufsFilledAtLowPrio = 0;
			DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:mask pause, set done\n");
			cyg_flag_maskbits( &m_Flag, ~FLAG_PAUSE );
			cyg_flag_setbits( &m_Flag, FLAG_DONE );
			// fall through and wait for another msg.
		}
		else if ( flagval & FLAG_SHUTDOWN )
		{
			DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:shutdown\n");
			break;
		}
		else {
			DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:unhandled flag value in prod msg pump\n");
		}
	}
}

void CProducerMsgPump::FirstStartup()
{
	if (!m_bStarted)
	{
		m_bStarted = true;
		StartProducing();
	}
}

void CProducerMsgPump::StartProducing()
{
	cyg_mutex_lock( &m_mutProducer );
	m_bFillMode = true;
	if (!m_bPaused && !m_bProducing)
	{
		DEBUG_WAKE_DRIVE;
		WakeDrive();
		StartProducer();
	}
	cyg_mutex_unlock( &m_mutProducer );
}

void CProducerMsgPump::ExitFillMode()
{
	cyg_mutex_lock( &m_mutProducer );
	m_bFillMode = false;
	StopProducer();
	cyg_mutex_unlock( &m_mutProducer );
}

void CProducerMsgPump::QuickExitFillMode()
{
	m_bTimeToRest = true;	// just set a flag and let the producer notice it in due time (without blocking on the operation in the caller thread)
}

void CProducerMsgPump::ResumeProducerThread()
{
	cyg_mutex_lock( &m_mutProducer );
	m_bPaused = false;
	if (m_bFillMode)
		StartProducer();
	cyg_mutex_unlock( &m_mutProducer );
}

void CProducerMsgPump::StartProducer()
{
	if (!m_bProducing)
	{
		// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
        //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
		//NotifyBufferReadBegin();
		m_bProducing = true;
		DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:set fillmode\n");
		cyg_flag_setbits( &m_Flag, FLAG_FILLMODE );
	}
}

void CProducerMsgPump::StopProducer()
{
	if (m_bProducing)
	{
   		// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
        //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
		//NotifyBufferReadEnd();
		m_bProducing = false;
		DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:mask done, fillmode, set pause\n");
		cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
		cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
		cyg_flag_setbits( &m_Flag, FLAG_PAUSE );
		DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:wait done\n");
		cyg_flag_wait( &m_Flag, FLAG_DONE, CYG_FLAG_WAITMODE_OR);
		if (!m_bPaused)
		{
			DEBUG_SLEEP_DRIVE;
			cyg_thread_delay(20);
			SleepDrive();
			SleepDrive();
		}
	}
}

void CProducerMsgPump::PauseProducerThread()
{
	DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:PauseProducerThread\n");
	cyg_mutex_lock( &m_mutProducer );
	m_bPaused = true;
	if (m_bFillMode)
	{
		StopProducer();
	}
	cyg_mutex_unlock( &m_mutProducer );
}

void CProducerMsgPump::ShutDown()
{
	cyg_mutex_lock( &m_mutProducer );
	DEBUGP( DBG_PRODUCER_PUMP, DBGLEV_INFO, "PPMP:set shutdown\n");
	cyg_flag_setbits( &m_Flag, FLAG_SHUTDOWN );
	cyg_mutex_unlock( &m_mutProducer );
}

void CProducerMsgPump::SetAggressiveMode()
{
	m_bAggressiveMode = true;
}

// in this mode, we'll run at lower prio and just buffer enough to get us by (and hopefully graduate us to the high prio mode)
void CProducerMsgPump::SetCautiousMode()
{
	m_bAggressiveMode = false;
	m_nBufsFilledAtLowPrio = 0;
}
