#include <util/thread/Mutex.h>
#include <cyg/infra/diag.h>
#include <new.h>

CMutex::CMutex(void) : owner(0), count(0) {
	cyg_mutex_init(&structure_lock);
	cyg_mutex_init(&lock);
}

CMutex::~CMutex(void) {
	cyg_mutex_destroy(&structure_lock);
	cyg_mutex_destroy(&lock);
}

void CMutex::Lock(void) {
	cyg_mutex_lock(&structure_lock);
	if (owner != cyg_thread_self()) {
		// try to obtain the mutex.  if we cannot do so immediately, release
		// the structure lock and wait on the mutex.
		if (!cyg_mutex_trylock(&lock)) {
			cyg_mutex_unlock(&structure_lock);
			cyg_mutex_lock(&lock);
			cyg_mutex_lock(&structure_lock);
		}
		owner = cyg_thread_self();
	}
	count++;
	cyg_mutex_unlock(&structure_lock);
}

void CMutex::Unlock(void) {
	cyg_mutex_lock(&structure_lock);
	if ((owner == cyg_thread_self()) && (--count == 0)) {
		cyg_mutex_unlock(&lock);
		owner = 0;
	}
	cyg_mutex_unlock(&structure_lock);
}

// C API
void cmutex_init( cmutex_t* cm ) 
{
    // I'd like to do the following:
    //    ((CMutex*)cm)->CMutex();
    // but C++ is retarded. hence we have to use placement new in order to get
    // the constructor call. 
    cm = (cmutex_t*) new(cm) CMutex();
}

void cmutex_destroy( cmutex_t* cm )
{
    ((CMutex*)cm)->~CMutex();
}

void cmutex_lock( cmutex_t* cm )
{
    ((CMutex*)cm)->Lock();
}

void cmutex_unlock( cmutex_t* cm )
{
    ((CMutex*)cm)->Unlock();
}
