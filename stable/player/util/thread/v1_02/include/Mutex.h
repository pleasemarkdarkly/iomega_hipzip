// Mutex.h: Recursively lockable mutex
// philr@iobjects.com 08/10/01
// (c) Interactive Objects

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus

class CMutex {
private:
  cyg_handle_t owner;         // owner of mutex
  cyg_mutex_t structure_lock; // structure access lock
  cyg_mutex_t lock;           // mutex
  unsigned int count;         // # of times owner locked mutex

public:
  CMutex(void);
  ~CMutex(void);
  void Lock(void);
  void Unlock(void);
};

/*	CMutexLock is designed to automatically lock and unlock a specified mutex within
	a block scope. This way you can have multiple exit paths and not worry about making
	sure it's unlocked from all of them. Even more useful if we ever use exceptions.
	Example:
	void CClassWithAMutex::Member()
	{
		CMutexLock l(m_Mutex);
		if (condition) {
			do stuff;
			return ERROR;
		} else {
			do other stuff
		}
	}

	m_Mutex will be unlocked no matter which way you exit.
*/

template <typename M>
class CMutexLock {
private:
	M &_mutex;

	// Copy constructor and assignment are intentionally unimplemented. This class
	// shouldn't be copied or assigned to.

	CMutexLock( const CMutexLock &) ;
	const CMutexLock& operator=( const CMutexLock & );
public:
	CMutexLock(M &m) : _mutex(m) { _mutex.Lock(); }
	~CMutexLock() { _mutex.Unlock(); }
};

// Specialization of CMutexLock supporting either a cyg_mutex_t or a cyg_mutex_t *
template<>
class CMutexLock<cyg_mutex_t> {
private:
	cyg_mutex_t &_mutex;
	CMutexLock( const CMutexLock &) ;
	const CMutexLock& operator=( const CMutexLock & );

public:
	CMutexLock(cyg_mutex_t &m) : _mutex(m) { cyg_mutex_lock(&_mutex); }
	~CMutexLock() { cyg_mutex_unlock(&_mutex); }
};

extern "C" {
    
#endif  // __cplusplus

// C API
// This definition must match CMutex... well, not really, but at least in size
typedef struct cmutex_s 
{
    cyg_handle_t owner;
    cyg_mutex_t structure_lock;
    cyg_mutex_t lock;
    unsigned int count;
} cmutex_t;

void cmutex_init( cmutex_t* cm );
void cmutex_destroy( cmutex_t* cm );
void cmutex_lock( cmutex_t* cm );
void cmutex_unlock( cmutex_t* cm );

#ifdef __cplusplus
};
#endif // __cplusplus
#endif // __MUTEX_H__
