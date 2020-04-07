#include <util/upnp/genlib/timer_thread.h>

#include <stdlib.h>

timer_thread_struct GLOBAL_TIMER_THREAD;

void free_upnp_timeout(upnp_timeout *event)
{
	if (event)
	{ 
		if (event->Event)
			free(event->Event);
		free(event);
	}
}


void TimerThread(void * input)
{ 
#ifdef USE_PTHREADS
	time_t next_event_time;
	time_t current_time;
	struct timespec timeout;
	int retcode;
#else
	cyg_tick_count_t next_event_time;
	cyg_tick_count_t current_time;
	cyg_tick_count_t timeout;
	cyg_bool_t retcode;
#endif
	timer_event * current_event;

	timer_thread_struct * timer=(timer_thread_struct *) input;

	while (1)
	{ 

#ifdef USE_PTHREADS
		if ( (pthread_mutex_lock(&timer->mutex)!=0) || (timer->shutdown))
#else
		cyg_mutex_lock(&timer->mutex);
		if (timer->shutdown)
#endif
		{ 
#ifdef USE_PTHREADS
			pthread_mutex_unlock(&timer->mutex);
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
#else
			cyg_mutex_unlock(&timer->mutex);
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",cyg_thread_self()));
#endif
			return;
		}

#ifdef USE_PTHREADS
		time(&current_time);
#else
		current_time = cyg_current_time();
#endif
		if ((current_event=timer->eventQ)!=NULL)
		{
			next_event_time=current_event->time;
		}
	//	else next_event_time=-1;
		else next_event_time=0;
		//make callback if time has expired

	//	if ( (next_event_time!=-1) && (current_time>=next_event_time))
		if ( (next_event_time!=0) && (current_time>=next_event_time))
		{ //remove from the Q
			timer->eventQ=timer->eventQ->next;
#ifdef USE_PTHREADS
			pthread_mutex_unlock(&timer->mutex);
#else
			cyg_mutex_unlock(&timer->mutex);
#endif
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"SCHEDULING TIMER EVENT INTO THREAD POOL"));
			tpool_Schedule( current_event->callback, current_event->argument); 
			free(current_event);
			continue;
		}

		//      if (next_event_time!=-1)
		if (next_event_time!=0)
		{
#ifdef USE_PTHREADS
			timeout.tv_sec=next_event_time;
			timeout.tv_nsec=0;
			retcode=0;
#else
			timeout = next_event_time;
			retcode = true;
#endif

#ifdef USE_PTHREADS
			while ( (timer->newEvent==0) && (retcode!=ETIMEDOUT))
			{
				retcode=pthread_cond_timedwait(&timer->newEventCond, &timer->mutex,&timeout);
			}
#else
			while ( (timer->newEvent==0) && (retcode))
			{ 
				retcode = cyg_cond_timed_wait(&timer->newEventCond, timeout);
			}
#endif

			if (timer->shutdown)
			{
#ifdef USE_PTHREADS
				pthread_mutex_unlock(&timer->mutex);
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
#else
				cyg_mutex_unlock(&timer->mutex);
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD SHUT DOWN"));
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",cyg_thread_self()));
#endif
				return;
			}
			if (timer->newEvent==1)
				timer->newEvent=0;
#ifdef USE_PTHREADS
			pthread_mutex_unlock(&timer->mutex);
#else
			cyg_mutex_unlock(&timer->mutex);
#endif
			continue;

		}
		else
		{
			while ((timer->newEvent==0))
			{
#ifdef USE_PTHREADS
				retcode=pthread_cond_wait(&timer->newEventCond, &timer->mutex);
#else
				retcode = cyg_cond_wait(&timer->newEventCond);
#endif
			}

			if (timer->shutdown)
			{
			//shutdown     
#ifdef USE_PTHREADS
				pthread_mutex_unlock(&timer->mutex);
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER_THREAD SHUTDOWN"));
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",pthread_self()));
#else
				cyg_mutex_unlock(&timer->mutex);
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER_THREAD SHUTDOWN"));
				DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"timer thread shutdown self : %ld\n",cyg_thread_self()));
#endif
				return;
			}
			timer->newEvent=0;
#ifdef USE_PTHREADS
			pthread_mutex_unlock(&timer->mutex);
#else
			cyg_mutex_unlock(&timer->mutex);
#endif
			continue;
		}
	}

}

int InitTimerThread(timer_thread_struct * timer)
{
#ifdef USE_PTHREADS
	pthread_mutex_init(&timer->mutex, NULL);
	pthread_mutex_lock(&timer->mutex);
	pthread_cond_init(&timer->newEventCond,NULL);
#else
	cyg_mutex_init(&timer->mutex);
	cyg_mutex_lock(&timer->mutex);
	cyg_cond_init(&timer->newEventCond, &timer->mutex);
#endif
  
	timer->eventQ=NULL;
	timer->newEvent=0;
	timer->currentEventId=0;
	timer->shutdown=0;

#ifdef USE_PTHREADS
	pthread_mutex_unlock(&timer->mutex);
#else
	cyg_mutex_unlock(&timer->mutex);
#endif
	//schedule timer_thread
	if (tpool_Schedule( TimerThread, timer)!=0)
		return UPNP_E_INIT_FAILED;

	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER THREAD INITIALIZED"));
	return UPNP_E_SUCCESS;
}

int RemoveTimerEvent(int eventId, void **argument, timer_thread_struct *timer)
{
	timer_event * current_event=NULL;
	timer_event * previous=NULL;
	int found=0;

	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__, "TRYING TO REMOVE EVENT : %d\n",eventId));
	if (eventId!=-1)
	{
#ifdef USE_PTHREADS
		pthread_mutex_lock(&timer->mutex);
#else
		cyg_mutex_lock(&timer->mutex);
#endif
		current_event=timer->eventQ;
		while (current_event)
		{
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TRYING TO REMOVE EVENT : %d\n",eventId));
			if (current_event->eventId==eventId)
			{
				found=1;
				break;
			}
			previous=current_event;
			current_event=current_event->next;
		}
		if (found)
		{
			(*argument)=current_event->argument;
			if (previous)
			{
				previous->next=current_event->next;
			}
			else
			{
				timer->eventQ=current_event->next;
			}
			free(current_event);
#ifdef USE_PTHREADS
			pthread_cond_signal(&timer->newEventCond);
#else
			cyg_cond_signal(&timer->newEventCond);
#endif
			DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"REMOVING TIMER EVENT"));
		}
		else
			(*argument)=NULL;
#ifdef USE_PTHREADS
		pthread_mutex_unlock(&timer->mutex);
#else
		cyg_mutex_unlock(&timer->mutex);
#endif
	}

	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER EVENT REMOVED"));
	return found;
}

int StopTimerThread(timer_thread_struct * timer)
{
	timer_event * current_event;

#ifdef USE_PTHREADS
	pthread_mutex_lock(&timer->mutex);
#else
	cyg_mutex_lock(&timer->mutex);
#endif

	timer->shutdown=1;

	while (timer->eventQ)
	{
		current_event=timer->eventQ;
		timer->eventQ=timer->eventQ->next;
#ifdef USE_PTHREADS
		pthread_mutex_unlock(&timer->mutex);
#else
		cyg_mutex_unlock(&timer->mutex);
#endif

		current_event->callback(current_event->argument);
		free(current_event);

#ifdef USE_PTHREADS
		pthread_mutex_lock(&timer->mutex);
#else
		cyg_mutex_lock(&timer->mutex);
#endif
	}

	//signal timer thread to stop
	timer->newEvent=1;
#ifdef USE_PTHREADS
	pthread_cond_signal(&timer->newEventCond);
	pthread_mutex_unlock(&timer->mutex);
	pthread_mutex_destroy(&timer->mutex);
	pthread_cond_destroy(&timer->newEventCond);
#else
	cyg_cond_signal(&timer->newEventCond);
	cyg_mutex_unlock(&timer->mutex);
	cyg_mutex_destroy(&timer->mutex);
	cyg_cond_destroy(&timer->newEventCond);
#endif
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"STOP TIMER FINISHED"));
	return UPNP_E_SUCCESS;
}

int ScheduleTimerEvent(int TimeOut, ScheduleFunc callback, void * argument,
		       timer_thread_struct *timer, int * eventId)
{
	timer_event *new_event;
	timer_event *prev=NULL;
	timer_event *finger=NULL;
#ifdef USE_PTHREADS
	time_t current_time;
	time_t expireTime;
	//get time
	time(&current_time);
	expireTime=current_time+TimeOut;
#else
	cyg_tick_count_t current_time;
	cyg_tick_count_t expireTime;
	//get time
	current_time = cyg_current_time();
	expireTime = current_time + TimeOut * 100;
#endif

	new_event=(timer_event*) malloc(sizeof(timer_event));
	if (new_event==NULL)
		return UPNP_E_OUTOF_MEMORY;

	new_event->time=expireTime;
	new_event->callback=callback;
	new_event->argument=argument;
	new_event->next=NULL;
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TRYING TO GET TIMER MUTEX"));
#ifdef USE_PTHREADS
	pthread_mutex_lock(&timer->mutex);
#else
	cyg_mutex_lock(&timer->mutex);
#endif
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"GOT TIMER MUTEX"));
	if (timer->shutdown)
	{
		free(new_event);
#ifdef USE_PTHREADS
		pthread_mutex_unlock(&timer->mutex);
#else
		cyg_mutex_unlock(&timer->mutex);
#endif
		return UPNP_E_INVALID_PARAM;
	}
	new_event->eventId=timer->currentEventId++;

	//check for overflow
	if (timer->currentEventId<0)
		timer->currentEventId=0;

	finger=timer->eventQ;

	while ( (finger) && (finger->time<new_event->time) )
	{
		prev=finger;
		finger=finger->next;
	}

	new_event->next=finger;

	if (prev)
	{
		prev->next=new_event;
	}
	else
	{ 
		timer->eventQ=new_event;
	}

	timer->newEvent=1;
	(*eventId)=new_event->eventId;
#ifdef USE_PTHREADS
	pthread_cond_signal(&timer->newEventCond);
#else
	cyg_cond_signal(&timer->newEventCond);
#endif
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"ADDING TIMER EVENT TO TIMER Q"));
	DBGONLY(UpnpPrintf(UPNP_INFO,API,__FILE__,__LINE__,"TIMER EVENT ID %d\n",new_event->eventId));
#ifdef USE_PTHREADS
	pthread_mutex_unlock(&timer->mutex);		 
#else
	cyg_mutex_unlock(&timer->mutex);		 
#endif
	return UPNP_E_SUCCESS;
}
