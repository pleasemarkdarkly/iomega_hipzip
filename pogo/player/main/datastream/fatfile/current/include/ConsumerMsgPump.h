#ifndef __PIS_INCLUDE_CONSMSGPMP_DEFINED__ 
#define __PIS_INCLUDE_CONSMSGPMP_DEFINED__

//#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>
#include <cyg/kernel/mutex.hxx>

#include <datastream/input/InputStream.h>

class CConsumer;

// (epg,10/18/2001): TODO: that's a lot.
#define CONSUMER_THREAD_STACK_SIZE (1024 * 64)

class CConsumerMsgPump
{
public:
	CConsumerMsgPump();
	~CConsumerMsgPump();
    int Seek(IInputStream::InputSeekPos Origin, int Offset);
	int Read(void* pBuffer,int dwBytes);
	CConsumer* GetConsumer();
  	// pause and resume the consumer
	void PauseConsumerThread();
	void ResumeConsumerThread();
    void JogDoneFlag();
private:
	static void ThreadEntry(CYG_ADDRESS data);
	void ThreadFunc();

    bool m_bPaused;
	CConsumer*	m_pConsumer;
	cyg_mutex_t	m_mutConsumer;
	cyg_flag_t	m_Flag;
	Cyg_Thread*	m_pThread;
	// seek params + return
	int m_nSeekOffset;
	IInputStream::InputSeekPos	m_SeekOrigin;
	int m_nSeekReturn;
	// read params + return
	void* m_pReadBuffer;
	int m_nReadBytes;
	int	m_nReadReturn;
    // flag values
	static const cyg_flag_value_t FLAG_READ = 1;			
	static const cyg_flag_value_t FLAG_SEEK = 2;			
	static const cyg_flag_value_t FLAG_DONE = 4;			
	static const cyg_flag_value_t FLAG_PAUSE = 8;			
	static const cyg_flag_value_t FLAG_RESUME = 16;			
	static const cyg_flag_value_t FLAG_SHUTDOWN = 32;       
	char* m_ConsumerStack;
};

#endif // __PIS_INCLUDE_CONSMSGPMP_DEFINED__