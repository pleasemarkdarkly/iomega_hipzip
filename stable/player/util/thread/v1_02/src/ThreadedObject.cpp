#include <util/thread/ThreadedObject.h>
#include <cyg/infra/diag.h>

IThreadedObject::IThreadedObject(cyg_addrword_t iPriority,
								 char *iName,
								 cyg_ucount32 iStackSize)
	: m_ThreadStack(0), m_bStopped(true)
{
	m_ThreadStack = new char[iStackSize];

	if (!m_ThreadStack) {
		diag_printf("Doaph! Failed to allocate stack.\n");
	}

    cyg_thread_create( iPriority,
                       IThreadedObject::EntryPoint,
                       (cyg_addrword_t) this,
                       iName,
                       m_ThreadStack,
                       iStackSize,
                       &m_ThreadHandle,
                       &m_ThreadData );
}

IThreadedObject::~IThreadedObject()
{
    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }
    // junk the thread now
    while( !cyg_thread_delete( m_ThreadHandle ) ) {
        cyg_thread_delay( 1 );
    }
    // and free the stack
    delete [] m_ThreadStack;
}

void
IThreadedObject::Start()
{
    m_bStopped = false;
    cyg_thread_resume( m_ThreadHandle );
}

void
IThreadedObject::EntryPoint(cyg_addrword_t obj) {
	IThreadedObject *instance = reinterpret_cast<IThreadedObject *>(obj);
	instance->ThreadBody();
	instance->m_bStopped = true;
}
