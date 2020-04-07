#include <stdlib.h>   /* rand, RAND_MAX*/
#include <string.h>   /* memset */

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

/* DEFINES */

#define NTHREADS 1
#define STACKSIZE ( (8 * 4096) )

// Mutually exclusive code switches
#define PERFORM_RANDOM_READWRITE_TEST
#define PERFORM_TIME_ANALYSIS
//#define WRITE_LBA_0_TO_FFFF0000			// write striped data to lba 0
//#define READ_LBA_0						// read in and display contents of lba 0

#if defined(PERFORM_TIME_ANALYSIS)
#include <pkgconf/kernel.h>
#define _1NS 1000000000
#endif

/* STATICS */

static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char _stack[NTHREADS][STACKSIZE];

#define MAX_SECTORS 64

#define SECTOR_COUNT 4096

extern "C" 
{
void atapi_thread(unsigned int);
  void cyg_user_start(void);
};

void
cyg_user_start(void)
{
  cyg_thread_create(10, atapi_thread, (cyg_addrword_t) 0, "atapi_thread",
		    (void *)_stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
  cyg_thread_resume(thread[0]);
}

/* Globals */
void CompareData(char * read_buffer, char * write_buffer, cyg_uint16 length);

void
open_cf(void)
{
    const char * dev_name = "/dev/hdb/";
    cyg_io_handle_t blk_devH;
    cyg_uint32 len;
    drive_geometry_t dg;
    
    if (cyg_io_lookup(dev_name, &blk_devH) != ENOERR) {
	diag_printf("Could not get handle to dev");
	return;
    }
    diag_printf("got handle to %s\n", dev_name);
    len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device\n");
    }
    diag_printf("powered up %s\n", dev_name);

    len = sizeof(dg);
    if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
	diag_printf("Could not get geometry");
	return;
    }
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);
}
    
void
atapi_thread(unsigned int ignored)
{
    char * read_buffer, * write_buffer;
    cyg_uint16 length;
    cyg_uint32 lba;
    unsigned int i;
    cyg_uint32 len;
    Cyg_ErrNo err;
    const char * dev_name = block_drive_names[0];
    cyg_io_handle_t blk_devH;
    drive_geometry_t dg;
#if defined(PERFORM_TIME_ANALYSIS)
    cyg_tick_count_t wtotal_time, wstart_time, rtotal_time, rstart_time;
    cyg_uint32 tick_length = _1NS/(CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR);
#endif
    
    srand(0x5834971);
    
    /* Initialize data buffers */
    read_buffer  = new char[(MAX_SECTORS * 512)];  
    write_buffer = new char[(MAX_SECTORS * 512)];
    diag_printf("read_buffer %x\nwrite_buffer %x\n", read_buffer, write_buffer);
    
    /* Initialize Write/Read Buffers */
    diag_printf("initializing write/read buffers\n");
    for (i=0; i < (MAX_SECTORS * 512); i++) {
	write_buffer[i] = (cyg_uint8) (i&0xff);
    }
    memset(read_buffer,0,MAX_SECTORS*512);
    diag_printf("done initializing\n");

    //open_cf();
    
    if (cyg_io_lookup(dev_name, &blk_devH) != ENOERR) {
	CYG_TEST_EXIT("Could not get handle to dev");
    }
    diag_printf("got handle to %s\n", dev_name);
    len = sizeof(len);

    while (cyg_io_set_config(blk_devH, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
	diag_printf("Could not power up device");
    }
    diag_printf("powered up %s\n", dev_name);

    len = sizeof(dg);
    if (cyg_io_get_config(blk_devH, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
	CYG_TEST_EXIT("Could not get geometry");
    }
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);

    cyg_uint32 max_lba = dg.num_blks - (MAX_SECTORS - 1);
    
    /* perform write/read tests */
    wtotal_time = rtotal_time = 0;
    int count;
    for(count = 0; count < SECTOR_COUNT; )
    {
	lba = rand() / (RAND_MAX/max_lba);
	length = (cyg_uint16)(((cyg_uint32)rand()) / (RAND_MAX/MAX_SECTORS));
	length += 1; // do at least one sector
	if (length > MAX_SECTORS) {
	    length = MAX_SECTORS;
	}
	lba += 50;
	if( lba > max_lba ) {
	  lba = max_lba;
	}

	count += length;
	diag_printf("LBA: %06x Sectors: %04x (%06x total)\n", lba, length, count);

	cyg_uint32 tmp_length = length * 512;
#if defined(PERFORM_TIME_ANALYSIS)
	wstart_time = cyg_current_time();
#endif
	err = cyg_io_bwrite(blk_devH, write_buffer, &tmp_length, lba);
#if defined(PERFORM_TIME_ANALYSIS)
	wtotal_time += (cyg_current_time() - wstart_time);
#endif
	
	if( err != ENOERR ) {
	    diag_printf("Error writing %d\n", err);
	    continue;
	}
	diag_printf("w");
            
	memset(read_buffer, 0, 512*MAX_SECTORS);

	tmp_length = length * 512;
#if defined(PERFORM_TIME_ANALYSIS)
	rstart_time = cyg_current_time();
#endif
	err = cyg_io_bread(blk_devH, read_buffer, &tmp_length, lba);
#if defined(PERFORM_TIME_ANALYSIS)
	rtotal_time += (cyg_current_time() - rstart_time);
#endif
	
	if( err != ENOERR ) {
	    diag_printf("Error reading %d\n", err);
	    continue;
	}
	diag_printf("r");
            
	CompareData(read_buffer, write_buffer, length);
	diag_printf("\n");
    }  
    diag_printf("\nrandom write read compares done\n");
#if defined(PERFORM_TIME_ANALYSIS)
    diag_printf("\n%d sectors read in %d time ", count, rtotal_time);
    diag_printf("(%6d bytes/sec)\n",(count*512*tick_length)/rtotal_time);
    diag_printf("\n%d sectors written in %d time ", count, wtotal_time );
    diag_printf("(%6d bytes/sec)\n",(count*512*tick_length)/wtotal_time);
#endif
    for(;;);
}                                     


void CompareData(char * read_buffer, char * write_buffer, cyg_uint16 length)
{
    cyg_uint32 i;
    
    for (i=0, length *= 512; i < (cyg_uint32)length; i++)
    {
	if (write_buffer[i] != read_buffer[i])
	    break;
    }
	
    if( i == (cyg_uint32)length )
	diag_printf("c");
    else {
	diag_printf(" MISCOMPARE @[%d] W%04X R=%04X\n", i, write_buffer[i], read_buffer[i]);
	for (;;)
	    ;
    }
}
