//
// diag.h: helper routines to print system state
//

#ifndef __DIAG_H__
#define __DIAG_H__

#ifdef __cplusplus
extern "C" {
#endif
#if 0    // brace alignment
}
#endif

#include <pkgconf/kernel.h>

#ifdef CYGFUN_KERNEL_THREADS_CPU_USAGE

//! If CPU usage tracking is available, print the amount of cpu
//! used by each thread. This routine locks the scheduler. If
//! do_reset != 0, then cpu times will be reset after they are read.
void print_thread_cpu_usage(int do_reset);

//! If CPU usage tracking is available, reset the counts for
//! each thread. Typically you would use this at the beginning
//! of an operation that you wanted to gauge, then use
//! 'print_thread_cpu_usage()' at the end. This routine locks
//! the scheduler.
void reset_thread_cpu_usage(void);

#endif // CYGFUN_KERNEL_THREADS_CPU_USAGE


//! This routine will print memory usage statistics
void print_mem_usage(void);

//! This routine will print thread states
void print_thread_states(void);

//! This routine will dump the current stack, if a symbol table is available at the end of the current image
//! If limit != 0, then dump_stack will stop after printing limit items.
void dump_thread_stack( int limit );

#if defined(ENABLE_LEAK_TRACING)
// TODO move this out to it's own header, since the function isn't part of
//  diag.cpp
//! This routine will dump the list of memory that is currently allocated; in other words, memory leaks
void dump_leak_trace( void );
#endif

#ifdef __cplusplus
};
#endif

#endif // __DIAG_H__
