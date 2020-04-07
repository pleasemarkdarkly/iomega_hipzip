// main.c
// fooo!
// temancl@fullplaymedia.com 05/14/02
// (c) fullplay media 
// 
// description:
// hardware functionality testing entry point
#include <stdio.h>
#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>


#include "parser.h"


// thread data etc
#define NTHREADS       1
#define STACKSIZE   8192*16

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

void thread_entry()
{
	shell();
}

void cyg_user_start(void)
{
   cyg_thread_create( 8, thread_entry, 0, "main thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );
}

