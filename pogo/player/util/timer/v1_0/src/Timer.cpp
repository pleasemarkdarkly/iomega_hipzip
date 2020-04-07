// Timer.cpp: central timer manager
// danc@iobjects.com 07/28/01
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <util/timer/Timer.h>
#include <util/thread/Mutex.h>

#include <string.h>   // memset

// Known issues
//  * The time between registration and the first timer call is only
//    defined to be greater than or equal to the specified interval
//  * The precision of the timing on the call can be impacted by
//    user level API calls

typedef struct timer_entry_s 
{
    pTimerFunc Func;   // function to call
    void* Arg;         // argument to pass
    int Interval;      // how frequently are we called?
    int Remaining;     // how long until the next call?
    int Iterations;    // how many total times do we call this?
	int TotalIterations; // used for resetting Iterations after suspend
    unsigned int
         Used:1,
         Suspended:1,
         AutoUnregister:1;  // unregister this timer after all iterations expire?
    
} timer_entry_t;

#define TIMER_TABLE_SIZE        15
#define TIMER_THREAD_PRIORITY   5
#define TIMER_STACK_SIZE        8192

class CTimerRegistry 
{
public:
    static CTimerRegistry* GetInstance() 
        {
            if( m_pSingleton == NULL ) {
                m_pSingleton = new CTimerRegistry;
            }
            return m_pSingleton;
        }
    static void Destroy()
        {
            if( m_pSingleton ) delete m_pSingleton;
            m_pSingleton = NULL;
        }
    inline void Lock() 
        { m_Mutex.Lock();   }
    inline void Unlock() 
        { m_Mutex.Unlock(); }
    
    ERESULT RegisterTimer( pTimerFunc Func, void* Arg, int Interval, int Iterations, bool AutoUnregister, timer_handle_t* handle );
    ERESULT SuspendTimer( timer_handle_t handle );
    ERESULT ResumeTimer( timer_handle_t handle );
    ERESULT UnregisterTimer( timer_handle_t handle );
	ERESULT ResetTimer( timer_handle_t handle );
    ERESULT ModifyTimer( timer_handle_t handle, int Interval, int Iterations );
    
private:
    static CTimerRegistry* m_pSingleton;

    static void AlarmFunc( cyg_handle_t alarm, cyg_addrword_t data );
    
    static void TimerThreadEntry(cyg_addrword_t data);
    void TimerThread();
    
    CTimerRegistry();
    ~CTimerRegistry();

    timer_entry_t m_TimerTable[ TIMER_TABLE_SIZE ];
    int m_Used;            // number of spots used
    CMutex m_Mutex;        // used to lock internal data
    cyg_sem_t m_Sem;       // used to wake the thread from the timer

    // Alarm data
    cyg_handle_t m_CounterHandle;
    cyg_handle_t m_AlarmHandle;
    cyg_alarm m_AlarmData;

    // Thread data
    cyg_handle_t m_ThreadHandle;
    cyg_thread m_ThreadData;
    char m_ThreadStack[ TIMER_STACK_SIZE ];
};

// declare the singleton
CTimerRegistry* CTimerRegistry::m_pSingleton = NULL;

CTimerRegistry::CTimerRegistry() 
{
    // set all timers to 'suspended'
    for( int i = 0; i < TIMER_TABLE_SIZE; i++ ) {
        m_TimerTable[i].Used = 0;
    }
    m_Used = 0;
    
	// create semaphore in construction, 
	// to avoid thread priority race
    cyg_semaphore_init( &m_Sem, 0 );

    cyg_thread_create( TIMER_THREAD_PRIORITY,
                       CTimerRegistry::TimerThreadEntry,
                       (cyg_addrword_t) this,
                       "Timer thread",
                       (void*) m_ThreadStack,
                       TIMER_STACK_SIZE,
                       &m_ThreadHandle,
                       &m_ThreadData );
    
    cyg_thread_resume( m_ThreadHandle );
}

CTimerRegistry::~CTimerRegistry() 
{
    // TODO: do we need to delete the counter also?
    cyg_alarm_delete( m_AlarmHandle );
    
    while( !cyg_thread_delete( m_ThreadHandle ) ) {
        cyg_thread_delay( 1 );
    }
    cyg_semaphore_destroy( &m_Sem );
}

void CTimerRegistry::TimerThreadEntry( cyg_addrword_t data ) 
{
    reinterpret_cast<CTimerRegistry*>(data)->TimerThread();
}

void CTimerRegistry::TimerThread() 
{

    
    cyg_clock_to_counter( cyg_real_time_clock(), &m_CounterHandle );
    cyg_alarm_create( m_CounterHandle, (cyg_alarm_t*)AlarmFunc, (unsigned int)this, &m_AlarmHandle, &m_AlarmData );
    cyg_alarm_initialize( m_AlarmHandle, cyg_current_time() + TIMER_TICK, TIMER_TICK );

    for( ;; ) {
        // Wait for the timer interrupt
        cyg_semaphore_wait( &m_Sem );

        Lock();
        
        // Now that it has occurred, wake up and process the timer list
        for( int i = 0; i < TIMER_TABLE_SIZE; i++ ) {
            if( m_TimerTable[i].Used && !m_TimerTable[i].Suspended ) {
                m_TimerTable[i].Remaining--;
                if( m_TimerTable[i].Remaining <= 0 ) {
                    m_TimerTable[i].Func( m_TimerTable[i].Arg );
                    m_TimerTable[i].Remaining = m_TimerTable[i].Interval;
                    
                    if( m_TimerTable[i].Iterations > 0 ) {
                        m_TimerTable[i].Iterations--;
                    }
                    if( m_TimerTable[i].Iterations == 0 ) {
                        // unregister timer
                        if (m_TimerTable[i].AutoUnregister)
						{
                            this->UnregisterTimer( (timer_handle_t) &m_TimerTable[i] );
						}
                        else
						{
							// reset iteration count, so it can be usefully resumed again
                            m_TimerTable[i].Suspended = 1;
							m_TimerTable[i].Iterations = m_TimerTable[i].TotalIterations;
						}
                    }
                }
            }
        }

        Unlock();
    }
}

void CTimerRegistry::AlarmFunc( cyg_handle_t alarm, cyg_addrword_t data ) 
{
    CTimerRegistry* pTimerRegistry = reinterpret_cast<CTimerRegistry*>(data);
    cyg_semaphore_post( &( pTimerRegistry->m_Sem ) );
}


ERESULT CTimerRegistry::RegisterTimer( pTimerFunc Func, void* Arg, int Interval, int Iterations, bool AutoUnregister, timer_handle_t* handle ) 
{
    ERESULT res = TIMER_NO_SPACE;

    this->Lock();

    if( m_Used == TIMER_TABLE_SIZE ) {
        this->Unlock();
        return TIMER_NO_SPACE;
    }
    
    for( int i = 0; i < TIMER_TABLE_SIZE; i++ ) {
        if( !m_TimerTable[i].Used ) {
            m_TimerTable[i].Used = 1;
            m_TimerTable[i].Suspended = 1;
            m_TimerTable[i].Func = Func;
            m_TimerTable[i].Arg = Arg;
            m_TimerTable[i].Interval = Interval;
            m_TimerTable[i].Remaining = Interval;
            m_TimerTable[i].Iterations = Iterations;
			m_TimerTable[i].TotalIterations = Iterations;
            m_TimerTable[i].AutoUnregister = AutoUnregister ? 1 : 0;
            res = TIMER_NO_ERROR;

            m_Used++;
            *handle = (timer_handle_t)&m_TimerTable[i];
            
            break;
        }
    }

    this->Unlock();
    
    return res;
}

ERESULT CTimerRegistry::SuspendTimer( timer_handle_t handle ) 
{
    ERESULT res = TIMER_NO_ERROR;
    
    this->Lock();

    timer_entry_t* entry = (timer_entry_t*) handle;

    entry->Suspended = 1;
  
    this->Unlock();
    
    return res;
}

ERESULT CTimerRegistry::ResumeTimer( timer_handle_t handle ) 
{
    ERESULT res = TIMER_NO_ERROR;
    
    this->Lock();

    timer_entry_t* entry = (timer_entry_t*)handle;

    entry->Suspended = 0;
    
    this->Unlock();
    
    return res;
}

ERESULT CTimerRegistry::ResetTimer( timer_handle_t handle ) 
{
    ERESULT res = TIMER_NO_ERROR;
    
    this->Lock();

    timer_entry_t* entry = (timer_entry_t*)handle;
   	entry->Iterations = entry->TotalIterations;
    entry->Remaining = entry->Interval;

    this->Unlock();
    
    return res;
}

ERESULT CTimerRegistry::UnregisterTimer( timer_handle_t handle ) 
{
    ERESULT res = TIMER_NO_ERROR;
    
    this->Lock();

    timer_entry_t* entry = (timer_entry_t*)handle;

    memset( (void*) entry, 0, sizeof( timer_entry_t ) );

    m_Used--;
    
    this->Unlock();

    return res;
}

ERESULT CTimerRegistry::ModifyTimer( timer_handle_t handle, int Interval, int Iterations )
{
    ERESULT res = TIMER_NO_ERROR;

    this->Lock();
    
    timer_entry_t* entry = (timer_entry_t*)handle;

    entry->Iterations = entry->TotalIterations = Iterations;
    entry->Remaining  = entry->Interval        = Interval;
    
    this->Unlock();
    return res;
}


//
// External API
//
ERESULT register_timer( pTimerFunc Func, void* Arg, int Interval, int Iterations, timer_handle_t* handle ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->RegisterTimer( Func, Arg, Interval, Iterations, true, handle );

    return res;
}

// Register a timer with iterations that won't automatically unregister itself when done.
ERESULT register_timer_persist( pTimerFunc Func, void* Arg, int Interval, int Iterations, timer_handle_t* handle )
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->RegisterTimer( Func, Arg, Interval, Iterations, false, handle );

    return res;
}

ERESULT suspend_timer( timer_handle_t handle ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->SuspendTimer( handle );

    return res;
}

ERESULT resume_timer( timer_handle_t handle ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->ResumeTimer( handle );

    return res;
}

ERESULT unregister_timer( timer_handle_t handle ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->UnregisterTimer( handle );
    
    return res;
}


ERESULT reset_timer( timer_handle_t handle ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->ResetTimer( handle );

    return res;
}

ERESULT modify_timer( timer_handle_t handle, int Interval, int Iterations ) 
{
    ERESULT res;
    CTimerRegistry* pRegistry = CTimerRegistry::GetInstance();

    res = pRegistry->ModifyTimer( handle, Interval, Iterations );

    return res;
}

ERESULT shutdown_timers(void) 
{
    CTimerRegistry::Destroy();
    return TIMER_NO_ERROR;
}
