//==========================================================================
//
//        dlmalloc1.cxx
//
//        dlmalloc memory pool test 1
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):     dsm, jlarmour
// Contributors:  
// Date:          2000-06-18
// Description:   Tests basic dlmalloc memory pool functionality
//####DESCRIPTIONEND####

#include <pkgconf/memalloc.h>
#include <pkgconf/system.h>

#ifdef CYGPKG_KERNEL
#include <pkgconf/kernel.h>

#include <cyg/kernel/sched.hxx>        // Cyg_Scheduler::start()
#include <cyg/kernel/thread.hxx>       // Cyg_Thread

#include <cyg/kernel/sched.inl>
#include <cyg/kernel/thread.inl>

#include <cyg/kernel/timer.hxx>         // Cyg_Timer
#include <cyg/kernel/clock.inl>         // Cyg_Clock

#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + 20*CYGNUM_HAL_STACK_FRAME_SIZE)
#define NTHREADS 2
#include "testaux.hxx"

#endif

#include <cyg/memalloc/dlmalloc.hxx>

#include <cyg/infra/testcase.h>

static const cyg_int32 memsize = 10240;

static cyg_uint8 mem[2][memsize];

static Cyg_Mempool_dlmalloc mempool0(mem[0], memsize);

static Cyg_Mempool_dlmalloc mempool1(mem[1], memsize);


static void check_in_mp0(cyg_uint8 *p, cyg_int32 size)
{
    CYG_TEST_CHECK(NULL != p,
                   "Allocation failed");
    CYG_TEST_CHECK(mem[0] <= p && p+size < mem[1],
                   "Block outside memory pool");
}


static void entry0( CYG_ADDRWORD data )
{
    cyg_int32 f0,f1,f2,t0;
    cyg_uint8 *p0, *p1;
    cyg_int32 most_of_mem=memsize/4*3;
    Cyg_Mempool_Status stat;
    
    mempool0.get_status( CYG_MEMPOOL_STAT_ORIGBASE|
                         CYG_MEMPOOL_STAT_BLOCKSIZE|
                         CYG_MEMPOOL_STAT_MAXFREE|
                         CYG_MEMPOOL_STAT_ORIGSIZE, stat );
    
    CYG_TEST_CHECK(mem[0] == stat.origbase, "get_status: base wrong");
    CYG_TEST_CHECK(memsize == stat.origsize, "get_status: size wrong");

    CYG_TEST_CHECK(0 < stat.maxfree && stat.maxfree <= stat.origsize,
                   "get_status: maxfree wildly wrong");
    
    CYG_TEST_CHECK(-1 == stat.blocksize, "blocksize wrong" );

    mempool0.get_status( CYG_MEMPOOL_STAT_TOTALFREE|
                         CYG_MEMPOOL_STAT_ARENASIZE, stat );
    t0 = stat.arenasize;
    CYG_TEST_CHECK(t0 > 0, "Negative total memory" );
    f0 = stat.totalfree;
    CYG_TEST_CHECK(f0 > 0, "Negative free memory" );
    CYG_TEST_CHECK(t0 <= memsize, "get_totalsize: Too much memory");
    CYG_TEST_CHECK(f0 <= t0 , "More memory free than possible" );

    mempool0.get_status( CYG_MEMPOOL_STAT_WAITING, stat );
    CYG_TEST_CHECK( !stat.waiting,
                    "Thread waiting for memory; there shouldn't be");
    
    CYG_TEST_CHECK( NULL == mempool0.try_alloc(memsize+1),
                    "Managed to allocate too much memory");
    
#ifdef CYGSEM_MEMALLOC_ALLOCATOR_VARIABLE_THREADAWARE
    p0 = mempool0.alloc(most_of_mem);
#else
    p0 = mempool0.try_alloc(most_of_mem);
#endif
    check_in_mp0(p0, most_of_mem);

    mempool0.get_status( CYG_MEMPOOL_STAT_TOTALFREE, stat );
    f1 = stat.totalfree;
    CYG_TEST_CHECK(f1 > 0, "Negative free memory" );
    CYG_TEST_CHECK(f1 < f0, "Free memory didn't decrease after allocation" );

    CYG_TEST_CHECK( NULL == mempool0.try_alloc(most_of_mem),
                    "Managed to allocate too much memory");
    
    CYG_TEST_CHECK(mempool0.free(p0, most_of_mem), "Couldn't free");

    mempool0.get_status( CYG_MEMPOOL_STAT_TOTALFREE, stat );
    f2 = stat.totalfree;
    CYG_TEST_CHECK(f2 > f1, "Free memory didn't increase after free" );
    
    // should be able to reallocate now memory is free
    p0 = mempool0.try_alloc(most_of_mem);
    check_in_mp0(p0, most_of_mem);

    p1 = mempool0.try_alloc(10);
    check_in_mp0(p1, 10);
    
    CYG_TEST_CHECK(p1+10 <= p0 || p1 >= p0+most_of_mem,
                   "Ranges of allocated memory overlap");

    CYG_TEST_CHECK(mempool0.free(p0, 0), "Couldn't free");
    CYG_TEST_CHECK(mempool0.free(p1, 10), "Couldn't free");
    
#ifdef CYGSEM_MEMALLOC_ALLOCATOR_VARIABLE_THREADAWARE
# ifdef CYGFUN_KERNEL_THREADS_TIMER
    // This shouldn't have to wait
    p0 = mempool0.alloc(most_of_mem,
        Cyg_Clock::real_time_clock->current_value() + 100000);
    check_in_mp0(p0, most_of_mem);
    p1 = mempool0.alloc(most_of_mem,
        Cyg_Clock::real_time_clock->current_value() + 2);
    CYG_TEST_CHECK(NULL == p1, "Timed alloc unexpectedly worked");
    p1 = mempool0.alloc(10,
        Cyg_Clock::real_time_clock->current_value() + 2);
    check_in_mp0(p1, 10);
    
    // Expect thread 1 to have run while processing previous timed
    // allocation.  It should therefore tbe waiting.
    mempool1.get_status( CYG_MEMPOOL_STAT_WAITING, stat );
    CYG_TEST_CHECK(stat.waiting, "There should be a thread waiting");
# endif
#endif
    
    CYG_TEST_PASS_FINISH("dlmalloc memory pool 1 OK");
}

#ifdef CYGSEM_MEMALLOC_ALLOCATOR_VARIABLE_THREADAWARE
static void entry1( CYG_ADDRWORD data )
{
    mempool1.alloc(memsize+1);
    CYG_TEST_FAIL("Oversized alloc returned");
}
#endif

void dlmalloc1_main( void )
{
    CYG_TEST_INIT();
    CYG_TEST_INFO("Starting dlmalloc memory pool 1 test");

#ifdef CYGSEM_MEMALLOC_ALLOCATOR_VARIABLE_THREADAWARE
    new_thread(entry0, 0);
    new_thread(entry1, 1);

    Cyg_Scheduler::start();
#elif defined(CYGPKG_KERNEL)
    new_thread(entry0, 0);

    Cyg_Scheduler::start();
#else
    entry0(0);
#endif

    CYG_TEST_FAIL_FINISH("Not reached");
}

externC void
cyg_start( void )
{ 
#ifdef CYGSEM_HAL_STOP_CONSTRUCTORS_ON_FLAG
    cyg_hal_invoke_constructors();
#endif
    dlmalloc1_main();
}
// EOF dlmalloc1.cxx
