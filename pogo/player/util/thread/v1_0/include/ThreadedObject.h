#ifndef __THREADEDOBJECT_H__
#define __THREADEDOBJECT_H__

#include <cyg/kernel/kapi.h>

#define RUN_AS_STARTUP_THREAD(ITHREADEDOBJECT_SUBCLASS) \
	extern "C" void cyg_user_start(void) { (new ITHREADEDOBJECT_SUBCLASS)->Start(); }


class IThreadedObject {
public:
	IThreadedObject(cyg_addrword_t pri, char *name, cyg_ucount32 stacksize);
	virtual ~IThreadedObject();
	
	void Start();
	
	virtual void ThreadBody(void) = 0;
	
	static void EntryPoint(cyg_addrword_t obj);
		
    cyg_handle_t m_ThreadHandle;
    cyg_thread m_ThreadData;
    char* m_ThreadStack;
    volatile bool m_bStopped;
};

#endif //__THREADEDOBJECT_H__

