#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/io/io.h>
#include <io/storage/blk_dev.h>
#include <fs/fat/sdapi.h>

#include <stdio.h>
#include <errno.h>

#define STACKSIZE (32 * 4096)

static cyg_handle_t thread;
static cyg_thread thread_obj;
static char stack[STACKSIZE];

void
fat_thread(cyg_addrword_t data)
{
    DSTAT _dstat;		// Can't read in debugger if name is same as type
    STAT stat;
    char * path = "A:\\*.*";
    char dirstr[6];
    INT16 status;
    SDBOOL bstatus;
    unsigned int i, j;
    PCFD fd;
    char fname[10];
    char buf[512];
    cyg_io_handle_t blk_devH;
    cyg_uint32 len;

    do {
	/* this can be tested with no media */
	bstatus = pc_system_init(0);
	if (!bstatus) {
	    printf("pc_system_init failed, errno = %d\n", errno);
	}
	else {
	    printf("pc_system_init passed\n");
	}
    } while (!bstatus);

    for (;;) {
	
    if (pc_gfirst(&_dstat, path)) {
	for (i = 0;;) {
	    ++i;
	    if (_dstat.fattribute & AVOLUME) {
		strcpy(dirstr, "<VOL>");
	    }
	    else if (_dstat.fattribute & ADIRENT) {
		strcpy(dirstr, "<DIR>");
	    }
	    else {
		strcpy(dirstr, "     ");
	    }
	    printf("%s.%s %d %s %d-%d-%d %d:%d %s\n",
			&(_dstat.fname[0]),
			&(_dstat.fext[0]),
			_dstat.fsize,
			dirstr,
			(_dstat.fdate >> 5) & 0xf,
			(_dstat.fdate & 0x1f),
			(80 + (_dstat.fdate >> 9)) & 0xff,
			(_dstat.ftime >> 11) & 0x1f,
			(_dstat.ftime >> 5) & 0x3f,
			&(_dstat.longFileName[0]));
	    
	    if (!pc_gnext(&_dstat))
		break;
	}
	pc_gdone(&_dstat);
    }
    }
    
    printf("LFN test done\n");
}

extern "C" 
{    
void
cyg_user_start(void)
{
    cyg_thread_create(10, fat_thread, (cyg_addrword_t)0, "fat_thread",
		      (void *)stack, STACKSIZE, &thread, &thread_obj);
    cyg_thread_resume(thread);
}
};

