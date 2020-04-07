//........................................................................................
//........................................................................................
//.. File Name: fat_test.h																..
//.. Date: 7/25/2000																	..
//.. Author(s): Todd Malsbary															..
//.. Description of content: fat file test								 				..
//.. Last Modified By: Eric Gibbs	ericg@iobjects.com									..	
//.. Modification date: 8/16/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include "sdapi.h"

#include <stdio.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

void
fat_thread(cyg_addrword_t data)
{
    DSTAT stat;
    char * path = "A:\\*.*";
    char dirstr[6];
    unsigned int status;
    unsigned int i;
    
    status = pc_system_init(0);
    if (!status) {
	diag_printf("Could not initialize drive 0\n");
	return;
    }
    diag_printf("Drive 0 successfully initialized\n");

    if (pc_gfirst(&stat, path)) {
	for (i = 0;;) {
	    ++i;
	    if (stat.fattribute & AVOLUME) {
		strcpy(dirstr, "<VOL>");
	    }
	    else if (stat.fattribute & ADIRENT) {
		strcpy(dirstr, "<DIR>");
	    }
	    else {
		strcpy(dirstr, "     ");
	    }
	    printf("%-8s.%-3s %7ld %5s %02d-%02d-%02d %02d:%02d\n",
		   &(stat.fname[0]),
		   &(stat.fext[0]),
		   stat.fsize,
		   dirstr,
		   (stat.fdate >> 5) & 0xf,
		   (stat.fdate & 0x1f),
		   (80 + (stat.fdate >> 9)) & 0xff,
		   (stat.ftime >> 11) & 0x1f,
		   (stat.ftime >> 5) & 0x3f);
	    if (!pc_gnext(&stat))
		break;
	}
	pc_gdone(&stat);
    }
}


void
cyg_user_start(void)
{
    cyg_thread_create(10, fat_thread, (cyg_addrword_t)0, "fat_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}
