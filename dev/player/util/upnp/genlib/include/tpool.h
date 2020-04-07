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
// * Neither name of the Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
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

// $Revision: 1.10 $
// $Date: 2000/09/06 20:16:37 $

#ifndef GENLIB_TPOOL_TPOOL_H
#define GENLIB_TPOOL_TPOOL_H

#include <stdio.h>
#ifndef _WIN32
#include <semaphore.h>
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <cyg/kernel/kapi.h>
#endif
#else
#include <util/upnp/api/interface.h>
#endif

#include <util/upnp/genlib/scheduler.h>
#include <util/upnp/genlib/xdlist.h>
#include <util/upnp/genlib/dbllist.h>

//#define USE_PTHREADS

#include <cyg/kernel/kapi.h>

struct PoolQueueItem
{
    ScheduleFunc func;
    void *arg;
};

//typedef xdlist<PoolQueueItem> ThreadPoolQueue;
typedef dblList ThreadPoolQueue;


class ThreadPool
{
public:
    enum {  DEF_MAX_THREADS = 10,
        DEF_LINGER_TIME = 2 * 60,     // seconds
        MAX_LINGER_TIME = 60 * 60 };
            
public:
	// throws OutOfMemoryException
    ThreadPool();

    virtual ~ThreadPool();

    int schedule( ScheduleFunc f, void* arg );
    
    void setMaxThreads( unsigned max );
    unsigned getMaxThreads();
    unsigned getNumThreadsRunning();
    unsigned getNumJobsPending();
    
    void setLingerTime( unsigned seconds );
    unsigned getLingerTime();
    
    void waitForZeroJobs();
        
private:
    ThreadPoolQueue q;
    
private:
    unsigned numThreads;
    unsigned maxThreads;
    unsigned lingerTime; // time for idle thread to die (in secs)
    bool allDie;         // to kill all threads
    
    //pthread_mutex_t mutex;
#ifdef USE_PTHREADS
    pthread_mutex_t mutex;
    pthread_cond_t condVariable;
    pthread_cond_t zeroCountCondVariable; // q size == 0 || allDie
#else
	cyg_mutex_t mutex;
	cyg_cond_t condVariable;
	cyg_cond_t zeroCountCondVariable; // q size == 0 || allDie
#endif
};

#endif /* GENLIB_TPOOL_TPOOL_H */
