#ifndef __PIS_INCLUDE_PMP_DEFINED__ 
#define __PIS_INCLUDE_PMP_DEFINED__

#include <main/buffering/BufferWriter.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>													// CYGFUN_KERNEL_API_C #defined

#include <util/thread/Mutex.h>

#define PRODUCER_THREAD_STACK_SIZE (1024*32)

class CBufferWorker;

// marshall events with parameters across the thread boundary to the consumer
class CWriterMsgPump
{
public:
	CWriterMsgPump();
	~CWriterMsgPump();

	void SetAggressiveMode();
	void SetCautiousMode();

	// enter a spun-up, producing mode, unless paused.
	void StartProducing();
	void StopProducing();
	void StopProducingNonBlock();

	// pause and resume the producer, but without losing the fillmode status.
	void PauseWriterThread();
	void ResumeWriterThread();

	void FirstStartup();
	void ShutDown();
	static cyg_mutex_t* GetMutWriter() { return &m_mutWriter; } 

    void SetWorker(CBufferWorker* pWkr);

    // to be called from inside writer thread
    void CullAtLowPrio();

public:

	// depending on fillmode and pausemode, start and stop the producer.
	void StartProducingAux();
	void StopWriter();

	static void ThreadEntry(CYG_ADDRESS data);								
	void ThreadFunc();


	CBufferWriter*	m_pWriter;

	cyg_flag_t	m_Flag;
	Cyg_Thread*	m_pThread;
	static cyg_mutex_t	m_mutWriter;
	char* m_Stack;
	bool m_bPauseDeferred;
	bool m_bAggressiveMode;
	bool m_bAwake;
	bool m_bPaused;
	bool m_bStarted;
    bool m_bStopped;
    bool m_bCullDone;
	bool m_bProducing;
	int	m_nBlocksFilledAtLowPrio;
    static const cyg_flag_value_t FLAG_FILLMODE = 1;	
    static const cyg_flag_value_t FLAG_RUNNING = 2;		
	static const cyg_flag_value_t FLAG_SHUTDOWN = 4;	
	static const cyg_flag_value_t FLAG_DONE = 8;
	static const cyg_flag_value_t FLAG_PAUSE = 16;
    CBufferWorker* m_pWorker;
};

#endif // __PIS_INCLUDE_PMP_DEFINED__
