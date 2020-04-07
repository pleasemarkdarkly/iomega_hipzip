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

// $Revision: 1.8 $
// $Date: 2000/07/29 21:49:41 $

#ifndef SCHEDULER_H
#define SCHEDULER_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void(*ScheduleFunc)( void *arg );

///////
// queues function func to be executed in a worker thread
// input:
//   func: function to be executed
//   arg: argument to passed to the function
// returns:
//   0 if success; -1 if not enuf mem; -2 if input func in NULL
int         tpool_Schedule( ScheduleFunc func, void* arg );

unsigned    tpool_GetMaxThreads( void );
void        tpool_SetMaxThreads( unsigned maxThreads );

void        tpool_SetLingerTime( unsigned seconds );
unsigned    tpool_GetLingerTime( void );

unsigned    tpool_GetNumThreadsRunning( void );
unsigned    tpool_GetNumJobsPending( void );

// wait until num jobs in queue == 0 or pool destroyed
void        tpool_WaitForZeroJobs( void );

void        tpool_Cleanup( void );
    
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* SCHEDULER_H */
