#ifndef __PIS_INCLUDE_CONSMSGPMP_DEFINED__ 
#define __PIS_INCLUDE_CONSMSGPMP_DEFINED__

//#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>
#include <cyg/kernel/mutex.hxx>

#include <main/buffering/BufferTypes.h>
#include <datastream/input/InputStream.h>

class CBufferReader;

// (epg,10/18/2001): TODO: that's a lot.
#define CONSUMER_THREAD_STACK_SIZE (1024 * 64)

class CReaderMsgPump
{
public:
	CReaderMsgPump();
	~CReaderMsgPump();
    int Seek(eInputSeekPos Origin, int Offset);
	int Read(void* pBlock,int dwBytes);
	CBufferReader* GetReader();
  	// pause and resume the consumer
	void PauseReaderThread();
	void ResumeReaderThread();
    void ShutDown();
    void JogDoneFlag();
    void SetWorker(CBufferWorker* pWkr);
public:
	static void ThreadEntry(CYG_ADDRESS data);
	void ThreadFunc();

    bool m_bPaused;
	CBufferReader*	m_pReader;
	cyg_mutex_t	m_mutReader;
	cyg_flag_t	m_Flag;
	Cyg_Thread*	m_pThread;
    bool m_bStopped;
	// seek params + return
	int m_nSeekOffset;
	eInputSeekPos	m_SeekOrigin;
	int m_nSeekReturn;
	// read params + return
	void* m_pReadBlock;
	int m_nReadBytes;
	int	m_nReadReturn;
    // flag values
	static const cyg_flag_value_t FLAG_READ = 1;			
	static const cyg_flag_value_t FLAG_SEEK = 2;			
	static const cyg_flag_value_t FLAG_DONE = 4;			
	static const cyg_flag_value_t FLAG_PAUSE = 8;			
	static const cyg_flag_value_t FLAG_RESUME = 16;			
	static const cyg_flag_value_t FLAG_SHUTDOWN = 32;       
	char* m_ReaderStack;
    CBufferWorker* m_pWorker;
};

#endif // __PIS_INCLUDE_CONSMSGPMP_DEFINED__
