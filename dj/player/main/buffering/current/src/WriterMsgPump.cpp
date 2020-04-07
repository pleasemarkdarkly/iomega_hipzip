#include <main/buffering/BufferDebug.h>
#include <main/buffering/BufferInStreamImp.h>
#include <main/buffering/WriterMsgPump.h>
//#include "DriveInterface.h"
#include <util/debug/debug.h>

#include <util/debug/debug.h>
//DEBUG_MODULE_S(DBG_WRITER_PUMP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON | DBGLEV_LOW_LEVEL_TRACE );
DEBUG_MODULE_S(DBG_WRITER_PUMP, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_WRITER_PUMP);  // debugging prefix : (8) bwm

// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
//  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
//extern void NotifyBlockReadBegin();
// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
//  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
//extern void NotifyBlockReadEnd();
cyg_mutex_t	CWriterMsgPump::m_mutWriter;

CWriterMsgPump::CWriterMsgPump() :
    m_bPauseDeferred(false),
    m_bAggressiveMode(true),
    m_bAwake(false), 
    m_bPaused(false),
    m_bStarted(false),
    m_bStopped(true),
    m_bCullDone(false),
    m_bProducing(false),
    m_nBlocksFilledAtLowPrio(0),
    m_pWorker(0)
{
	m_Stack = new char[PRODUCER_THREAD_STACK_SIZE];
    DEBUGP( DBG_WRITER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "bwm:Ctor\n"); 
	cyg_flag_init( &m_Flag );
	m_pThread = new Cyg_Thread( HIGH_DJ_BUFFERING_THREAD_PRIO, // thread priority
								&CWriterMsgPump::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"BlockWriter",
								(CYG_ADDRESS)m_Stack,
								(cyg_ucount32)PRODUCER_THREAD_STACK_SIZE);
	m_pThread->resume();
	cyg_mutex_init( &m_mutWriter );
	m_pWriter = new CBufferWriter;
}

CWriterMsgPump::~CWriterMsgPump()
{
    m_bStopped = false;
    ShutDown();

    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }

	cyg_mutex_destroy( &m_mutWriter );
	cyg_flag_destroy( &m_Flag );
	delete m_pWriter;
	delete [] m_Stack;
	delete m_pThread;
}

void CWriterMsgPump::ThreadEntry(CYG_ADDRESS data)
{
	CWriterMsgPump* pThis = reinterpret_cast<CWriterMsgPump*>(data);
	pThis->ThreadFunc();
}

void CWriterMsgPump::CullAtLowPrio()
{
    DEBUGP( DBG_WRITER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "bwm:Cull\n"); 
    m_pThread->set_priority( NORMAL_DJ_BUFFERING_THREAD_PRIO );
    m_pWorker->ReclaimDeadBlocks();
    m_pWorker->DistributeFreeBlocks();
    m_pThread->set_priority( HIGH_DJ_BUFFERING_THREAD_PRIO );
}

void CWriterMsgPump::ThreadFunc()
{
	cyg_flag_value_t flagval;
    int nBlocksWrittenSinceCull = 0;
	while (1)
	{
		DEBUGP( DBG_WRITER_PUMP, DBGLEV_LOW_LEVEL_TRACE, "bwm:wt\n");
		flagval = cyg_flag_wait( &m_Flag, FLAG_FILLMODE | FLAG_SHUTDOWN | FLAG_PAUSE, CYG_FLAG_WAITMODE_OR );

		if ( flagval & FLAG_FILLMODE )
		{
            // periodically collect blocks from documents that no longer need them.
            if (!m_bCullDone || nBlocksWrittenSinceCull > BUFFER_RE_CULL_THRESHOLD)
            {
                DEBUGP( DBG_WRITER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "bwm:Wake\n");
                m_bCullDone = true;
                CullAtLowPrio();
                nBlocksWrittenSinceCull = 0;
            }

            // write the block
            eBWResult result;
            if (!nBlocksWrittenSinceCull)
            {
                // write the first block in low priority, hopefully avoiding perf crunch on cd.
                m_pThread->set_priority( NORMAL_DJ_BUFFERING_THREAD_PRIO );
                result = m_pWriter->WriteBlock();
                m_pThread->set_priority( HIGH_DJ_BUFFERING_THREAD_PRIO );
            }
            else
                result = m_pWriter->WriteBlock();
            ++nBlocksWrittenSinceCull;

            // if the writer is all caught up, go to sleep until the buffers run down.
			if (result == BW_DONE || m_bPauseDeferred)
			{
				m_bPauseDeferred = false;
				DEBUGP( DBG_WRITER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "bwm:SleepDone\n");
				cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
				m_bAwake = false;
				m_bProducing = false;
                m_bCullDone = false;
				DEBUG_SLEEP_DRIVE;
				cyg_thread_delay(20);
				while (!IsDriveAsleep())
					SleepDrive();
			}
            // the write hit EOF.
            else if (result == BW_EOF)
            {
                // trigger a block redistribution, since the file ended and may have released post-eof blocks.
                DEBUGP( DBG_WRITER_PUMP, DBGLEV_TRACE, "bwm:re-cull for eof pending..\n"); 
                m_bCullDone = false;
            }
            // a minimal amount has been buffered, let decoding catch up for a while before we jump into high-prio buffering.
			else if (!m_bAggressiveMode)
			{
				m_nBlocksFilledAtLowPrio++;
				if (m_nBlocksFilledAtLowPrio >= BLOCKS_TO_FILL_AT_LOW_PRIORITY)
				{		
					DEBUGP( DBG_WRITER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "bwm:SleepTired\n");
					m_nBlocksFilledAtLowPrio = 0;
					SetAggressiveMode();
					cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
					m_bAwake = false;
					m_bProducing = false;
                    m_bCullDone = false;
					DEBUG_SLEEP_DRIVE;
					cyg_thread_delay(20);
					SleepDrive();
					SleepDrive();
				}
			}
			else
			{
				m_nBlocksFilledAtLowPrio = 0;
			}
		}
		else if ( flagval & FLAG_PAUSE )
		{
			m_nBlocksFilledAtLowPrio = 0;
			DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:Resume\n");
			cyg_flag_maskbits( &m_Flag, ~FLAG_PAUSE );
			cyg_flag_setbits( &m_Flag, FLAG_DONE );
		}
		else if ( flagval & FLAG_SHUTDOWN )
		{
			DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:Shutdown\n");
			break;
		}
		else {
			DEBUGP( DBG_WRITER_PUMP, DBGLEV_ERROR, "BWM:UnhandledFlag\n");
		}
	}
    m_bStopped = true;
}

void CWriterMsgPump::FirstStartup()
{
	if (!m_bStarted)
	{
		m_bStarted = true;
		StartProducing();
	}
}

// unless paused, start the producer
void CWriterMsgPump::StartProducing()
{
	cyg_mutex_lock( &m_mutWriter );
	m_bAwake = true;
	if (!m_bPaused && !m_bProducing)
	{
		DEBUG_WAKE_DRIVE;
        m_bCullDone = false;
		WakeDrive();
		StartProducingAux();
	}
	cyg_mutex_unlock( &m_mutWriter );
}

// exit fill buffer mode
void CWriterMsgPump::StopProducing()
{
	cyg_mutex_lock( &m_mutWriter );
	m_bAwake = false;
	StopWriter();
	cyg_mutex_unlock( &m_mutWriter );
}

void CWriterMsgPump::StopProducingNonBlock()
{
	m_bPauseDeferred = true;	// just set a flag and let the producer notice it in due time (without blocking on the operation in the caller thread)
}

void CWriterMsgPump::PauseWriterThread()
{
	DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:pause\n");
	cyg_mutex_lock( &m_mutWriter );
	m_bPaused = true;
	if (m_bAwake)
	{
		StopWriter();
	}
	cyg_mutex_unlock( &m_mutWriter );
}

void CWriterMsgPump::ResumeWriterThread()
{
    // if we're not paused, short circuit.
    DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:resume\n");

	cyg_mutex_lock( &m_mutWriter );
	m_bPaused = false;
	if (!m_bAwake)
    {
	    m_bAwake = true;
	    if (!m_bPaused && !m_bProducing)
	    {
		    DEBUG_WAKE_DRIVE;
            m_bCullDone = false;
		    WakeDrive();
		    StartProducingAux();
	    }
    }
    else
        StartProducingAux();
	cyg_mutex_unlock( &m_mutWriter );
}

void CWriterMsgPump::StartProducingAux()
{
	if (!m_bProducing)
	{
		// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
        //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
		//NotifyBlockReadBegin();
		m_bProducing = true;
		DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:fill\n");
		cyg_flag_setbits( &m_Flag, FLAG_FILLMODE );
	}
}

void CWriterMsgPump::StopWriter()
{
	if (m_bProducing)
	{
   		// (epg,10/19/2001): TODO: this was an externally defined fn in the UI that 
        //  restricted compute-intensive ops during buffer reads.  hopefully we won't need them.
		//NotifyBlockReadEnd();
		m_bProducing = false;
		DEBUGP( DBG_WRITER_PUMP, DBGLEV_VERBOSE, "bwm:~done,~fill,pause\n");
		cyg_flag_maskbits( &m_Flag, ~FLAG_DONE );
		cyg_flag_maskbits( &m_Flag, ~FLAG_FILLMODE );
		cyg_flag_setbits( &m_Flag, FLAG_PAUSE );
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

void CWriterMsgPump::ShutDown()
{
	cyg_mutex_lock( &m_mutWriter );
	DEBUGP( DBG_WRITER_PUMP, DBGLEV_HIGH_LEVEL_TRACE, "bwm:set shutdown\n");
	cyg_flag_setbits( &m_Flag, FLAG_SHUTDOWN );
	cyg_mutex_unlock( &m_mutWriter );
}

void CWriterMsgPump::SetAggressiveMode()
{
	m_bAggressiveMode = true;
}

// in this mode, we'll run at lower prio and just buffer enough to get us by (and hopefully graduate us to the high prio mode)
void CWriterMsgPump::SetCautiousMode()
{
    // (epg,10/23/2001): TODO: if this becomes needed, reenable cautious mode.
	//m_bAggressiveMode = false;
	m_nBlocksFilledAtLowPrio = 0;
}

void CWriterMsgPump::SetWorker(CBufferWorker* pWkr)
{
    m_pWorker = pWkr;
    m_pWriter->SetWorker(pWkr);
}
