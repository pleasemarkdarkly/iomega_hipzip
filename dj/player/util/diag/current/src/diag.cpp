//
// diag.cpp: helper routines to print system state
//

#include <util/diag/diag.h>
#include <util/debug/debug.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/thread.inl>
#include <cyg/kernel/kapi.h>
#ifdef CYGDBG_KERNEL_DEBUG_GDB_THREAD_SUPPORT
#include <cyg/hal/hal_arch.h>
#define DISPLAY_REGISTERS
#endif

#include <stdlib.h>
//#include "symtable.h"

DEBUG_MODULE_S(DIAG, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DIAG);

#ifdef CYGFUN_KERNEL_THREADS_CPU_USAGE
void print_thread_cpu_usage( int do_reset ) 
{
    cyg_scheduler_lock();
    Cyg_Thread* pList = Cyg_Thread::get_list_head();

    // sum cpu usage
    cyg_tick_count_t total_time = 0;
    
    while( pList ) {
        total_time += pList->get_cpu_usage();
        pList = pList->get_list_next();
    }
    

    pList = Cyg_Thread::get_list_head();
    while( pList ) {
        cyg_tick_count u = pList->get_cpu_usage();
        
        DEBUGP( DIAG, DBGLEV_INFO, "Thread name %s cpu usage %llu percent %llu%%\n",
                     ( pList->get_name() ? pList->get_name() : "unnammed" ),
                     u,
                     (100*u)/total_time );

        if( do_reset ) {
            pList->reset_cpu_usage();
        }
        
        pList = pList->get_list_next();
    }

    cyg_scheduler_unlock();
}

void reset_thread_cpu_usage( void ) 
{
    cyg_scheduler_lock();
    
    Cyg_Thread* pList = Cyg_Thread::get_list_head();

    while( pList ) {
        pList->reset_cpu_usage();
        pList = pList->get_list_next();
    }
    
    cyg_scheduler_unlock();
}
#endif // CYGFUN_KERNEL_THREADS_CPU_USAGE

void print_mem_usage( void ) 
{
    struct mallinfo mem_info;
    DEBUGP( DIAG, DBGLEV_INFO, "Memory system statistics\n");
    mem_info = mallinfo();
    DEBUGP( DIAG, DBGLEV_INFO, "Total=0x%08x Used = 0x%08x Free=0x%08x Max=0x%08x\n",
                mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
                mem_info.maxfree);
}

static const char* sleep_reasons[] =
{
    "none",
    "wait",
    "delay",
    "timeout",
    "break",
    "destruct",
    "exit",
    "done",
};
void print_thread_states( void )
{
    cyg_scheduler_lock();
    Cyg_Thread* pList = Cyg_Thread::get_list_head();
    DEBUGP(DIAG,DBGLEV_INFO,"Thread state dump\n");
    DEBUGP(DIAG,DBGLEV_INFO,"id\tprio\tstate\treason\tname\n");
    while( pList ) {
        const char* sleep_reason = sleep_reasons[pList->get_sleep_reason()];
        const char* thread_state = 0;
        const char* name = "unknown";
#ifdef CYGVAR_KERNEL_THREADS_NAME
        name = pList->get_name();
#endif
        switch( pList->get_state() )
        {
            case Cyg_Thread::RUNNING:
            { thread_state = "run";   break; }
            case Cyg_Thread::SLEEPING:
            { thread_state = "sleep"; break; }
            case Cyg_Thread::COUNTSLEEP:
            { thread_state = "delay"; break; }
            case Cyg_Thread::SUSPENDED:
            { thread_state = "suspend";break;}
            case Cyg_Thread::CREATING:
            { thread_state = "create"; break;}
            case Cyg_Thread::EXITED:
            { thread_state = "exited"; break;}
            default: thread_state = "oops";
        };
        DEBUGP(DIAG,DBGLEV_INFO,"%3d\t%3d\t%s\t%s\t%s\n",
            pList->get_unique_id(), pList->get_priority(), thread_state, sleep_reason, name );
#ifdef DISPLAY_REGISTERS
        const HAL_SavedRegisters* pReg = pList->get_saved_context();
        DEBUGP(DIAG,DBGLEV_INFO,"\t\tpc(0x%08x) sp(0x%08x:0x%08x) fp(0x%08x) vec(0x%08x)\n",
            pReg->pc, pReg->sp,pList->get_stack_base(), pReg->fp, pReg->vector );
#endif
        pList = pList->get_list_next();
    }
    
    cyg_scheduler_unlock();
}

void dump_thread_stack( int limit )
{
#if 0
    unsigned int _sp;
    // Let the symtable code find its own symtable
    if( symtable_init( 0 ) < 0 ) {
        return ;
    }

    __asm__ __volatile__( "mov %0, sp" : "=r"(_sp) );
    
    dump_stack( limit, _sp );
#endif
}

