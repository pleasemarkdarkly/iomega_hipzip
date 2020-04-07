#ifndef _TIMER_THREAD_
#define _TIMER_THREAD_

#include <util/upnp/api/upnp.h>
#include <time.h>
#include <errno.h>

#include <util/upnp/genlib/scheduler.h>

//#include <malloc.h>
#include <util/upnp/api/upnp_debug.h>

#include <cyg/kernel/kapi.h>

#ifndef _WIN32
#ifdef USE_PTHREADS
#include <pthread.h>
#endif	// USE_PTHREADS
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else 
#define EXTERN_C 
#endif
#else
#include "../../../../win32/win32.h"
#endif

typedef struct UPNP_TIMEOUT
{
	int EventType;
	int handle;
	int eventId;
	void *Event;
} upnp_timeout;


typedef struct TIMER_EVENT
{
#ifdef USE_PTHREADS
	time_t time;
#else
	cyg_tick_count_t time;
#endif
	ScheduleFunc callback;
	void * argument;
	int eventId;
	struct TIMER_EVENT * next;
} timer_event;

typedef struct TIMER_THREAD_STRUCT {
#ifdef USE_PTHREADS
	pthread_mutex_t mutex;
	pthread_cond_t newEventCond; 
#else
	cyg_mutex_t mutex;
	cyg_cond_t newEventCond; 
#endif
	int newEvent;
	int shutdown;
	int currentEventId;
	timer_event * eventQ;
} timer_thread_struct;


extern timer_thread_struct GLOBAL_TIMER_THREAD;

EXTERN_C int InitTimerThread(timer_thread_struct * timer);

EXTERN_C int StopTimerThread(timer_thread_struct * timer);

EXTERN_C int ScheduleTimerEvent(int TimeOut, 
				ScheduleFunc callback,
				void * argument,
				timer_thread_struct * timer,
				int * eventId);

EXTERN_C int RemoveTimerEvent(int eventId, void **argument, timer_thread_struct *timer);

EXTERN_C void free_upnp_timeout(upnp_timeout *event);

#endif
