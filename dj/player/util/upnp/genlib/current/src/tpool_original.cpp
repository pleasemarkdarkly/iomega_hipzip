///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////

// $Revision: 1.4 $
// $Date: 2000/10/05 18:32:02 $

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <util/upnp/genlib/tpool.h>
#include <util/upnp/genlib/utilall.h>
#include <util/upnp/genlib/util.h>
#include <util/upnp/genlib/xdlist.h>
#include <util/upnp/genlib/miscexceptions.h>
#include <util/upnp/genlib/dbllist.h>

#include <util/upnp/api/config.h>
extern "C"
{
#ifdef USE_PTHREADS
#include <cyg/posix/signal.h>	// - ecm
#endif
#include <sys/time.h>
}

#include <pkgconf/kernel.h>

#include <cyg/infra/diag.h>

#include <util/upnp/genlib/noexceptions.h>

//typedef cyg_uint64 cyg_tick_count_t;
//externC void cyg_ticks_to_timespec( cyg_tick_count ticks, struct timespec *tp );
//void cyg_ticks_to_timespec( cyg_uint64 ticks, struct timespec *tp );

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#endif

#define EVENT_TIMEDOUT 		-2
#define EVENT_TERMINATE		-3

//#define MY_STACKSIZE (128 * 1024)
#define MY_STACKSIZE (256 * 1024)
#define UPNP_TPOOL_MAX_THREAD_COUNT	10

static cyg_handle_t s_hThreadPool[UPNP_TPOOL_MAX_THREAD_COUNT];
static cyg_thread s_ThreadObjPool[UPNP_TPOOL_MAX_THREAD_COUNT];
static char s_chStackPool[UPNP_TPOOL_MAX_THREAD_COUNT][MY_STACKSIZE];
static bool s_bThreadUse[UPNP_TPOOL_MAX_THREAD_COUNT];
static bool s_bThreadAlive[UPNP_TPOOL_MAX_THREAD_COUNT];

//typedef xdlistNode<PoolQueueItem> ThreadPoolNode;
typedef dblListNode ThreadPoolNode;

//#define DEBUG_THREAD_USE(s...) diag_printf(##s)
#define DEBUG_THREAD_USE(s...) /**/

struct ThreadArg
{
    unsigned *timeoutSecs;
    ThreadPoolQueue* q;
    
    // pool data
#ifdef USE_PTHREADS
    pthread_mutex_t* poolMutex;
    pthread_cond_t* condVariable;
    pthread_cond_t* zeroCountCondVariable;
#else
	cyg_mutex_t* poolMutex;
	cyg_cond_t* condVariable;
	cyg_cond_t* zeroCountCondVariable;
#endif
    unsigned* numThreads;
    bool* die;
	int	iStackIndex;
};

// returns
//	 0: success
//	 EVENT_TIMEDOUT
//	 EVENT_TERMINATE
int GetNextItemInQueue( ThreadArg* arg,
    OUT PoolQueueItem& callback )
{
    unsigned timeoutSecs = *arg->timeoutSecs;
#ifdef USE_PTHREADS
    int code = 0;
#else
	cyg_bool_t code = true;
#endif
    int retCode;
    ThreadPoolQueue* q;
    
    q = arg->q;
    
#ifdef USE_PTHREADS
    pthread_mutex_lock( arg->poolMutex );
#else
	cyg_mutex_lock( arg->poolMutex );
#endif

    // wait until item becomes available
    if ( timeoutSecs < 0 )
    {
        while ( q->length() == 0 && *arg->die == false )
        {
#ifdef USE_PTHREADS
            pthread_cond_wait( arg->condVariable,
                arg->poolMutex );
#else
			cyg_cond_wait( arg->condVariable );
#endif
        }
    }
    else
    {
#ifdef USE_PTHREADS
        timeval now;
        timespec timeout;
/*
        gettimeofday( &now, NULL );
        timeout.tv_sec = now.tv_sec + timeoutSecs;
        timeout.tv_nsec = now.tv_usec * 1000;
*/
		// - ecm : If Jesus exists then this will work.
		unsigned int cct = cyg_current_time();
		cyg_ticks_to_timespec( cct, &timeout );
        timeout.tv_sec = timeout.tv_sec + timeoutSecs;
#else
		cyg_tick_count_t timeout = cyg_current_time() + timeoutSecs * 100;
#endif
        
        while ( q->length() == 0 &&
                *arg->die == false &&
#ifdef USE_PTHREADS
                code != ETIMEDOUT )
#else
				code )
#endif
        {
#ifdef USE_PTHREADS
            code = pthread_cond_timedwait( arg->condVariable,
                arg->poolMutex, &timeout );
#else
            code = cyg_cond_timed_wait( arg->condVariable, timeout );
#endif
        }
    }
    
#ifdef USE_PTHREADS
    DBG( pthread_t tempThread = pthread_self(); )
#else
	DBG( cyg_handle_t tempThread = cyg_thread_self(); )
#endif
    
    if ( *arg->die == true )
    {
        retCode = EVENT_TERMINATE;
        
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: got terminate msg\n", tempThread); )
    }
#ifdef USE_PTHREADS
    else if ( code == ETIMEDOUT )
#else
    else if ( !code )
#endif
    {
        retCode = EVENT_TIMEDOUT;
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: got timeout msg\n", tempThread); )
    }
    else
    {
        ThreadPoolNode* node;
        PoolQueueItem* callbackptr;
        
        assert( q->length() > 0 );
        
        node = q->getFirstItem();
        callbackptr = (PoolQueueItem*) node->data;
        callback = *callbackptr;
        q->remove( node );

        // broadcast zero count condition of queue
        if ( arg->q->length() == 0 )
        {
            //DBG( printf("thread pool q len = 0 broadcast\n"); )
#ifdef USE_PTHREADS
            pthread_cond_broadcast( arg->zeroCountCondVariable );
#else
            cyg_cond_broadcast( arg->zeroCountCondVariable );
#endif
        }
        
        retCode = 0;
    }
    
#ifdef USE_PTHREADS
	pthread_mutex_unlock( arg->poolMutex );	
#else
	cyg_mutex_unlock( arg->poolMutex );	
#endif
    
    return retCode;
}

#ifdef USE_PTHREADS
static void* ThreadCallback( void* the_arg )
#else
static void ThreadCallback( CYG_ADDRESS the_arg )
#endif
{
    ThreadArg* arg = (ThreadArg *)the_arg;
    int retCode;
    PoolQueueItem callback;
    
    // DBG
    //unsigned threadNum;
    
    //{
    //}
    ///////////////
    
#ifdef USE_PTHREADS
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "thread %ld: started...\n", pthread_self()); )
#else
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "thread %ld: started...\n", cyg_thread_self()); )
#endif
    //DBG( int countxxx = 0; )
    
///	while ( true )
///	{
        // pull out next callback from queue
#ifdef USE_PTHREADS
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: waiting for item\n", pthread_self()); )
#else
        DBG(
            UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
                "thread %ld: waiting for item\n", cyg_thread_self()); )
#endif
        retCode = GetNextItemInQueue( arg, callback );
        
        if ( retCode == EVENT_TIMEDOUT || retCode == EVENT_TERMINATE )
        {
            //DBG( printf("thread got signal %d\n", retCode); )
///			break;		// done with thread
        }
        
        // invoke callback
        callback.func( callback.arg );
///	}
    
    // decrement active thread count
#ifdef USE_PTHREADS
    pthread_mutex_lock( arg->poolMutex );
#else
	cyg_mutex_lock( arg->poolMutex );
#endif
    
    *arg->numThreads = *arg->numThreads - 1;
    
#ifdef USE_PTHREADS
    pthread_mutex_unlock( arg->poolMutex );
#else
	s_bThreadUse[arg->iStackIndex] = false;
	DEBUG_THREAD_USE("THREAD: Returned thread index %d: %x\n", arg->iStackIndex, s_hThreadPool[arg->iStackIndex]);

	cyg_mutex_unlock( arg->poolMutex );
#endif

    delete arg;

#ifdef USE_PTHREADS
    pthread_exit( NULL );
    return NULL;
#else
	cyg_thread_exit( );
#endif
}


///////////////////
// ThreadPool

ThreadPool::ThreadPool()
{
    numThreads = 0;
    maxThreads = DEF_MAX_THREADS;
    lingerTime = DEF_LINGER_TIME;
    allDie = false;
    
	// Initialize thread stacks.
	for (int i = 0; i < UPNP_TPOOL_MAX_THREAD_COUNT; ++i)
	{
		s_bThreadUse[i] = false;
		s_bThreadAlive[i] = false;
	}

    int success;
    
    //DBG( printf("thread pool constructor\n"); )
    
#ifdef USE_PTHREADS
    success = pthread_mutex_init( &mutex, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating mutex\n"); )
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "Mutex creation error in thread pool" );
    }
#else
	cyg_mutex_init( &mutex );
#endif
    
#ifdef USE_PTHREADS
    success = pthread_cond_init( &condVariable, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating cond var\n"); )
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "Thread Pool: error creating cond variable" );
    }
#else
	cyg_cond_init( &condVariable, &mutex );
#endif
    
#ifdef USE_PTHREADS
    success = pthread_cond_init( &zeroCountCondVariable, NULL );
    if ( success == -1 )
    {
        DBG(
            UpnpPrintf(
                UPNP_CRITICAL, TPOOL, __FILE__, __LINE__,
                    "thread pool: error creating zero cond var\n"); )
		diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
        throw OutOfMemoryException( "Thread Pool: error creating count condition variable" );
    }
#else
	cyg_cond_init( &zeroCountCondVariable, &mutex );
#endif
}

ThreadPool::~ThreadPool()
{
    // signal all threads to die
#ifdef USE_PTHREADS
    pthread_mutex_lock( &mutex );
    allDie = true;
    pthread_cond_broadcast( &condVariable );
    pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_lock( &mutex );
	allDie = true;
	cyg_cond_broadcast( &condVariable );
	cyg_mutex_unlock( &mutex );
#endif
    
    DBG(
        UpnpPrintf( UPNP_INFO, TPOOL, __FILE__, __LINE__,
            "thread pool destructor: sent die msg\n"); )
    
    // wait till all threads are done
    while ( true )
    {
        if ( numThreads == 0 )
        {
            break;
        }

        //DBG( printf("thread destructor waiting: pending = %d, running = %d\n", getNumJobsPending(), getNumThreadsRunning()); )

        // send signal again, if missed previously
#ifdef USE_PTHREADS
        pthread_cond_broadcast( &condVariable );
#else
		cyg_cond_broadcast( &condVariable );
#endif
        //sleep( 1 );
		cyg_thread_delay(100);
    }

    // signal no more threads to run
#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
	pthread_cond_broadcast( &zeroCountCondVariable );
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_lock( &mutex );
	cyg_cond_broadcast( &zeroCountCondVariable );
	cyg_mutex_unlock( &mutex );
#endif

    int code;
        
    // destroy zeroCountCondVariable
#ifdef USE_PTHREADS
    while ( true )
    {
		code = pthread_cond_destroy( &zeroCountCondVariable );
        if ( code == 0 )
        {
            break;
        }

	
	sleep( 1 );
    }
#else
	cyg_cond_destroy( &zeroCountCondVariable );
#endif
    
#ifdef USE_PTHREADS
	code = pthread_cond_destroy( &condVariable );
	assert( code == 0 );

	code = pthread_mutex_destroy( &mutex );
	assert( code == 0 );
#else
	cyg_cond_destroy( &condVariable );
	cyg_mutex_destroy( &mutex );
#endif
}

/////////
// queues function f to be executed in a worker thread
// input:
//   f: function to be executed
//   arg: argument to passed to the function
// returns:
//   0 if success; -1 if not enuf mem; -2 if input func in NULL
int ThreadPool::schedule( ScheduleFunc f, void* arg )
{
    int retCode = 0;
	int code = 0;
    
    if ( f == NULL )
        return -2;
        
#ifdef USE_PTHREADS
	pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif

//	try
//	{
        PoolQueueItem* job;

        job = (PoolQueueItem*)malloc( sizeof(PoolQueueItem) );
        if ( job == NULL )
        {
//			diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//			throw OutOfMemoryException( "ThreadPool::schedule()" );
			DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION( "ThreadPool::schedule()" );
			retCode = -1;
			goto CatchError;
        }

        job->func = f;
        job->arg = arg;
        
        q.addAfterTail( job );
        
        // signal a waiting thread
#ifdef USE_PTHREADS
		pthread_cond_signal( &condVariable );
#else
		cyg_cond_signal( &condVariable );
#endif
        
        // generate a thread if not too many threads
        if ( numThreads < maxThreads )
        {
            ThreadArg* t_arg = new ThreadArg;
            if ( t_arg == NULL )
            {
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -2;	// optional error - out of mem
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION( "ThreadPool::schedule()" );
				code = -2;
				goto CatchError;
			}	
        
			t_arg->timeoutSecs = &lingerTime;
			t_arg->q = &q;
			t_arg->poolMutex = &mutex;
			t_arg->condVariable = &condVariable;
			t_arg->zeroCountCondVariable = &zeroCountCondVariable;
			t_arg->numThreads = &numThreads;
			t_arg->die = &allDie;
        
#ifdef USE_PTHREADS
			pthread_t thread;
			int code;
			pthread_attr_t attr;
			sched_param param;

			pthread_attr_init( &attr );

			pthread_attr_setinheritsched( &attr, PTHREAD_EXPLICIT_SCHED );

			param.sched_priority = 31 - 10;
			pthread_attr_setschedparam ( &attr, &param );
        
			// start a new thread
			code = pthread_create( &thread, &attr, ThreadCallback, t_arg );

			if ( code == 0 )
			{
				numThreads++;
				code = pthread_detach( thread );
				assert( code == 0 );
			}	

#else
			// Find a free thread stack entry.
			int i;
			for (i = 0; i < UPNP_TPOOL_MAX_THREAD_COUNT; ++i)
			{
				if (!s_bThreadUse[i])
				{
					s_bThreadUse[i] = true;
					t_arg->iStackIndex = i;
					if (s_bThreadAlive[i])
					{
						DEBUG_THREAD_USE("THREAD: Killing thread index %d: %x\n", i, s_hThreadPool[i]);
						cyg_thread_delete(s_hThreadPool[i]);
						s_bThreadAlive[i] = false;
					}

					DEBUG_THREAD_USE("THREAD: Using thread index %d: %x\n", i, s_hThreadPool[i]);
					break;
				}
			}

			if (i == UPNP_TPOOL_MAX_THREAD_COUNT)
			{
				// Too many damn threads.
				diag_printf("*** Too many threads! ***\n");
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -2;	// optional error - out of mem
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION( "ThreadPool::schedule()" );
				code = -2;
				goto CatchError;
			}

#if CYGSEM_KERNEL_SCHED_MLQUEUE == 1
			cyg_thread_create( UPNP_TPOOL_THREAD_PRIORITY_BASE, ThreadCallback, (unsigned int)t_arg, "generic upnp thread", &s_chStackPool[t_arg->iStackIndex], MY_STACKSIZE, &s_hThreadPool[t_arg->iStackIndex], &s_ThreadObjPool[t_arg->iStackIndex]);
#else
			cyg_thread_create( UPNP_TPOOL_THREAD_PRIORITY_BASE + t_arg->iStackIndex, ThreadCallback, (unsigned int)t_arg, "generic upnp thread", &s_chStackPool[t_arg->iStackIndex], MY_STACKSIZE, &s_hThreadPool[t_arg->iStackIndex], &s_ThreadObjPool[t_arg->iStackIndex]);
#endif
			if (!s_hThreadPool[t_arg->iStackIndex])
			{
				diag_printf("**** OH SHIT! Thread creation failure! ****\n");
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -2;	// optional error - out of mem
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION( "ThreadPool::schedule()" );
				code = -2;
				goto CatchError;
			}

			numThreads++;
			s_bThreadAlive[t_arg->iStackIndex] = true;
			cyg_thread_resume(s_hThreadPool[t_arg->iStackIndex]);

#endif
            
            if ( numThreads == 0 )
            {
//				diag_printf("About to die in " __FILE__ ": %d\n", __LINE__);
//				throw -2;
				DEBUG_THROW_OUT_OF_MEMORY_EXCEPTION( "ThreadPool::schedule()" );
				code = -2;
				goto CatchError;
            }
        }
/*
    }
    catch ( OutOfMemoryException& e )
    {
        retCode = -1;
    }
    catch ( int code )
    {
        if ( code == -2 )
        {
            // couldn't start new thread
            if ( numThreads == 0 )
            {
                retCode = -1;
            }
        }
    }
*/

CatchError:
	if ( code == -2 )
	{
		// couldn't start new thread
		if ( numThreads == 0 )
			retCode = -1;
	}
    
#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif
    
    return retCode;
}

void ThreadPool::setMaxThreads( unsigned max )
{
    maxThreads = max;
}

unsigned ThreadPool::getMaxThreads()
{
    return maxThreads;
}

unsigned ThreadPool::getNumJobsPending()
{
    return q.length();
}

unsigned ThreadPool::getNumThreadsRunning()
{
    return numThreads;
}

unsigned ThreadPool::getLingerTime()
{
    return lingerTime;
}

void ThreadPool::setLingerTime( unsigned seconds )
{
    if ( seconds > MAX_LINGER_TIME )
    {
        lingerTime = MAX_LINGER_TIME;
    }
    else
    {
        lingerTime = seconds;
    }
}

// returns if num jobs in q == 0 or die signal has been given
void ThreadPool::waitForZeroJobs()
{
  cyg_thread_delay(100);
  //    sleep( 1 );
    
#ifdef USE_PTHREADS
    pthread_mutex_lock( &mutex );
#else
	cyg_mutex_lock( &mutex );
#endif
    // wait until num jobs == 0
    if ( q.length() != 0 && allDie == false )
    {
#ifdef USE_PTHREADS
		pthread_cond_wait( &zeroCountCondVariable, &mutex );
#else
		cyg_cond_wait( &zeroCountCondVariable );
#endif
    }
#ifdef USE_PTHREADS
	pthread_mutex_unlock( &mutex );
#else
	cyg_mutex_unlock( &mutex );
#endif
    //DBG( printf("waitForZeroJobs(): done\n"); )
}
