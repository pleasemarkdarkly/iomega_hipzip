// Timer.h: interface for a generic timer
// danc@iobjects.com 07/28/01
// (c) Interactive Objects

#ifndef __TIMER_H__
#define __TIMER_H__

#include <util/eresult/eresult.h>

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#define TIMER_ERROR_ZONE   0x81

const int TIMER_NO_ERROR  = MAKE_ERESULT( SEVERITY_SUCCESS, TIMER_ERROR_ZONE, 0x00 );
const int TIMER_NOT_FOUND = MAKE_ERESULT( SEVERITY_FAILED,  TIMER_ERROR_ZONE, 0x00 );
const int TIMER_NO_SPACE  = MAKE_ERESULT( SEVERITY_FAILED,  TIMER_ERROR_ZONE, 0x00 );

typedef void (*pTimerFunc)(void* Arg);

typedef unsigned int timer_handle_t;

// Multiply this value by 10ms
#define TIMER_TICK              10
// Convert milliseconds into timer intervals
#define TIMER_MILLISECONDS(x)   ((x) / (TIMER_TICK * 10))

// Interval is in 100ms units
// for infinite running, specify -1 Iterations
EXTERN_C ERESULT register_timer( pTimerFunc Func, void* Arg, int Interval, int Iterations, timer_handle_t* handle );
EXTERN_C ERESULT suspend_timer( timer_handle_t handle );
EXTERN_C ERESULT resume_timer( timer_handle_t handle );
EXTERN_C ERESULT unregister_timer( timer_handle_t handle );
EXTERN_C ERESULT reset_timer( timer_handle_t handle );
EXTERN_C ERESULT modify_timer( timer_handle_t handle, int Interval, int Iterations );

// Register a timer with iterations that won't automatically unregister itself when done.
EXTERN_C ERESULT register_timer_persist( pTimerFunc Func, void* Arg, int Interval, int Iterations, timer_handle_t* handle );

EXTERN_C ERESULT shutdown_timers(void);

#endif // __TIMER_H__
