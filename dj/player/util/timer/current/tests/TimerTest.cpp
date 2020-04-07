// TimerTest.cpp: test harness for timer routines
// danc@iobjects.com 07/28/01
// (c) Interactive Objects

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>

#include <util/timer/Timer.h>

extern "C" void cyg_user_start( void );
extern "C" void thread_func( cyg_addrword_t data );

static cyg_handle_t ThreadHandle;
static cyg_thread   ThreadData;
static char  ThreadStack[ 8192 ];

static void handler_func1( void* arg );
static void handler_func2( void* arg );
static void handler_func3( void* arg );


void thread_func( cyg_addrword_t data ) 
{
    timer_handle_t handles[9];
    
    // handler_func1 tests eternal functions at varying rates
    register_timer( handler_func1, (void*)11, 1, -1, &handles[0] );
    register_timer( handler_func1, (void*)12, 2, -1, &handles[1] );
    register_timer( handler_func1, (void*)13, 3, -1, &handles[2] );
    
    // handler_func2 tests time limited functions at the same rate
    register_timer( handler_func2, (void*)21, 1,  5, &handles[3] );
    register_timer( handler_func2, (void*)22, 2,  5, &handles[4] );
    register_timer( handler_func2, (void*)23, 3,  5, &handles[5] );

    // handler_func3 tests time limited functions at different rates
    register_timer( handler_func3, (void*)31, 1,  6, &handles[6] );
    register_timer( handler_func3, (void*)32, 2,  3, &handles[7] );
    register_timer( handler_func3, (void*)33, 3,  2, &handles[8] );

    for( int i = 0; i < 9; i++ ) {
        resume_timer( handles[i] );
    }
    // exit
}

static void handler_func1( void* arg ) 
{
    diag_printf("handler func1 arg %d fired\n", (int) arg );
}
static void handler_func2( void* arg ) 
{
    diag_printf("handler func2 arg %d fired\n", (int) arg );
}
static void handler_func3( void* arg ) 
{
    diag_printf("handler func3 arg %d fired\n", (int) arg );
}

void cyg_user_start( void ) 
{
    cyg_thread_create( 10,
                       thread_func,
                       0,
                       "Test thread",
                       (void*)ThreadStack,
                       8192,
                       &ThreadHandle,
                       &ThreadData );
    cyg_thread_resume( ThreadHandle );
}

