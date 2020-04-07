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
#include <fs/fat/sdapi.h>

#include <stdio.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

void
test_file( const char* filename ) 
{
    PCFD fd = po_open( (char*)filename, PO_RDONLY, PS_IREAD );
    char buf1[20], buf2[20];
    short err;
    
    if( fd < 0 ) {
        diag_printf("Unable to open %s\n", filename );
        return ;
    }

    po_read( fd, buf1, 20 );
    
    po_close( fd );

    fd = po_open( (char*)filename, PO_RDONLY, PS_IREAD );

#if 0
    po_read( fd, buf2,    5 );
    po_read( fd, buf2+5,  5 );
    po_read( fd, buf2+10, 5 );
    po_read( fd, buf2+15, 5 );
#else
    po_lseek( fd, 15, PSEEK_SET, &err );
    po_read( fd, buf2+15, 5 );
    po_lseek( fd, 10, PSEEK_SET, &err );
    po_read( fd, buf2+10, 5 );
    po_lseek( fd, 5, PSEEK_SET, &err );
    po_read( fd, buf2+5, 5 );
    po_lseek( fd, 0, PSEEK_SET, &err );
    po_read( fd, buf2, 5 );
#endif
    po_close( fd );
    
    if( memcmp( buf1, buf2, 20 ) != 0 ) {
        diag_printf("File cmp failed\n");
    }
    else {
        diag_printf("File cmp passed\n");
    }
}

void
fat_thread(cyg_addrword_t data)
{
    DSTAT stat;
    char * path = "A:\\*.*";
    char dirstr[6];
    unsigned int status;
    unsigned int i;

    char filename[15];
    filename[0] = 0;
    
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

            if( filename[0] == 0 ) {
                strcat( filename, "A:\\" );
                strcat( filename, stat.fname );
                strcat( filename, "." );
                strcat( filename, stat.fext );
            }
            
	    if (!pc_gnext(&stat))
		break;
	}
	pc_gdone(&stat);
    }

    test_file( filename );
}


void
cyg_user_start(void)
{
    cyg_thread_create(10, fat_thread, (cyg_addrword_t)0, "fat_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}
