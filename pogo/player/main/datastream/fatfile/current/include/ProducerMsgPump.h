#ifndef __PIS_INCLUDE_PMP_DEFINED__ 
#define __PIS_INCLUDE_PMP_DEFINED__

#include <main/datastream/fatfile/Producer.h>

#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>													// CYGFUN_KERNEL_API_C #defined

#define PRODUCER_THREAD_STACK_SIZE (1024*32)

// marshall events with parameters across the thread boundary to the consumer
class CProducerMsgPump
{
public:
	CProducerMsgPump();
	~CProducerMsgPump();

	void SetAggressiveMode();
	void SetCautiousMode();

	// enter a spun-up, producing mode, unless paused.
	void StartProducing();
	void StopProducing();
	void StopProducingNonBlock();

	// pause and resume the producer, but without losing the fillmode status.
	void PauseProducerThread();
	void ResumeProducerThread();

	void FirstStartup();
	void ShutDown();
	static cyg_mutex_t* GetMutProducer() { return &m_mutProducer; } 
private:

	// depending on fillmode and pausemode, start and stop the producer.
	void StartProducingAux();
	void StopProducer();

	static void ThreadEntry(CYG_ADDRESS data);								
	void ThreadFunc();

	CProducer*	m_pProducer;

	cyg_flag_t	m_Flag;
	Cyg_Thread*	m_pThread;
	static cyg_mutex_t	m_mutProducer;
	char* m_Stack;
	bool m_bPauseDeferred;
	bool m_bAggressiveMode;
	bool m_bFillMode;
	bool m_bPaused;
	bool m_bStarted;
    bool m_bCullDone;
	bool m_bProducing;
	int	m_nBufsFilledAtLowPrio;
    static const cyg_flag_value_t FLAG_FILLMODE = 1;	
    static const cyg_flag_value_t FLAG_RUNNING = 2;		
	static const cyg_flag_value_t FLAG_SHUTDOWN = 4;	
	static const cyg_flag_value_t FLAG_DONE = 8;
	static const cyg_flag_value_t FLAG_PAUSE = 16;
};

#endif // __PIS_INCLUDE_PMP_DEFINED__
