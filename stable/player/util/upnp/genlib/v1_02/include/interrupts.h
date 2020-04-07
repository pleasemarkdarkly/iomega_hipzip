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

// $Revision: 1.7 $
// $Date: 2000/08/10 22:04:58 $

#ifndef GENLIB_TPOOL_INTERRUPTS_H
#define GENLIB_TPOOL_INTERRUPTS_H

#include <util/upnp/genlib/util.h>
#ifndef _WIN32
#ifdef USE_PTHREADS
#include <pthread.h>
#else
#include <cyg/kernel/kapi.h>
#endif
#endif

#define TINTR_E_NOT_ENOUGH_MEM      -2
#define TINTR_E_NOT_INITIALIZED     -3
#define TINTR_E_ALREADY_INITIALIZED -4


typedef void (*InterruptHandler)(void);

#ifdef __cplusplus
extern "C" {
#endif

// returns:
//   0: success
//  -1: std error; check errno
//  TINTR_E_NOT_INITIALIZED
#ifdef USE_PTHREADS
int tintr_SetHandler( IN pthread_t thread, IN InterruptHandler handler,
    OUT InterruptHandler* oldHandler );
#else
int tintr_SetHandler( IN cyg_handle_t thread, IN InterruptHandler handler,
    OUT InterruptHandler* oldHandler );
#endif

// interrupts given thread and calls its interrupt handler

// Note: even if thread handler does not exist, 'thread' will
//  be interrupted in blocking calls
// returns
//   0: success;
//  -1: std error check errno
//  TINTR_E_NOT_INITIALIZED 
#ifdef USE_PTHREADS
int tintr_Interrupt( IN pthread_t thread );
#else
int tintr_Interrupt( IN cyg_handle_t thread );
#endif

// initializes thread interrupt manager with given signal number
// returns:
//  0: success
// -1: std error: check errno
// TINTR_E_ALREADY_INITIALIZED
int tintr_Init( int signalNum );

// uninitializes interrupt handler
// returns:
//   0: success
//  -1: std error; check errno
// TINTR_E_NOT_INITIALIZED
int tintr_Done( void );


#ifdef __cplusplus
}
#endif


#endif /* GENLIB_TPOOL_INTERRUPTS_H */

