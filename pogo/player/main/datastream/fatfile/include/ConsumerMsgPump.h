#ifndef __PIS_INCLUDE_CONSMSGPMP_DEFINED__ 
#define __PIS_INCLUDE_CONSMSGPMP_DEFINED__

//#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>
#include <cyg/kernel/mutex.hxx>

#include <datastream/input/InputStream.h>
//#include <main/datastream/fatfile/Consumer.h>

class CConsumer;

// (epg,10/18/2001): TODO: that's a lot.
#define CONSUMER_THREAD_STACK_SIZE (1024 * 64)

// marshall events with parameters across the thread boundary to the consumer
class CConsumerMsgPump
{
public:
	CConsumerMsgPump();
	~CConsumerMsgPump();
    int Seek(IInputStream::InputSeekPos Origin, int Offset);
	int Read(void* pBuffer,int dwBytes);
	CConsumer* GetConsumer();
private:
	static void ThreadEntry(CYG_ADDRESS data);								// thread entry point
	void ThreadFunc();
	CConsumer*	m_pConsumer;
	cyg_mutex_t	m_mutConsumer;
	cyg_flag_t	m_Flag;
	Cyg_Thread*	m_pThread;
	// seek params + return
	int						m_nSeekOffset;
	IInputStream::InputSeekPos	m_SeekOrigin;
	int						m_nSeekReturn;
	// read params + return
	void*				m_pReadBuffer;
	int				m_nReadBytes;
	int						m_nReadReturn;
	cyg_sem_t m_semConsumerDone;
	static const cyg_flag_value_t FLAG_READ = 1;				// read
	static const cyg_flag_value_t FLAG_DONE = 8;				// read
	static const cyg_flag_value_t FLAG_SEEK = 2;				// seek 
	static const cyg_flag_value_t FLAG_SHUTDOWN = 4;
	void ConsumerThreadFunc();														// main consumer thread function
	char* m_ConsumerStack;			// workroom for the consumer thread
};

#endif // __PIS_INCLUDE_CONSMSGPMP_DEFINED__