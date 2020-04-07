// align_test.cpp: test buffer alignment impacts on data read
// assumption: source media has data on it
// danc@iobjects.com 07/04/01
// (c) Interactive Objects

#include <stdlib.h>   /* rand, RAND_MAX*/
#include <string.h>   /* memset */

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/testcase.h>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <util/debug/debug.h>

DEBUG_MODULE_S(ALIGN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(ALIGN);

/* DEFINES */

#define PERFORM_RANDOM_READWRITE_TEST
#define PERFORM_TIME_ANALYSIS

#define MAX_SECTORS 64

#define SECTOR_COUNT 4096

#define NTHREADS 1
#define STACKSIZE ( (8 * 4096) )

/* STATICS */
static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char _stack[NTHREADS][STACKSIZE];


extern "C" 
{
  void atapi_thread(unsigned int);
  void cyg_user_start(void);
};

void
cyg_user_start(void)
{
  DBEN(ALIGN);
  cyg_thread_create(10, atapi_thread, (cyg_addrword_t) 0, "atapi_thread",
		    (void *)_stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
  cyg_thread_resume(thread[0]);
  DBEX(ALIGN);
}

/* Globals */
void CompareData(char * read_buffer, char * write_buffer, cyg_uint16 length);

void
atapi_thread(unsigned int ignored)
{
    cyg_uint16 length;
    cyg_uint32 lba;
    cyg_uint32 len;
    Cyg_ErrNo err;
    cyg_io_handle_t blk_devA;
    drive_geometry_t dg;
    
    srand(0x5834971);
    
    /* Initialize data buffers */
    char* aligned_read_buffer  = new char[(MAX_SECTORS * 512)];  
    char* aligned_write_buffer = new char[(MAX_SECTORS * 512)];
    static char rbuffer[ 3 + (MAX_SECTORS * 512) ];
    static char wbuffer[ 3 + (MAX_SECTORS * 512) ];

    char* unaligned_read_buffer  = rbuffer + 1;
    char* unaligned_write_buffer = wbuffer + 1;
    
    DEBUGP(ALIGN,DBGLEV_INFO,"aligned_read_buffer %x\naligned_write_buffer %x\n", aligned_read_buffer, aligned_write_buffer);
    DEBUGP(ALIGN,DBGLEV_INFO,"unaligned_read_buffer %x\nunaligned_write_buffer %x\n",unaligned_read_buffer,unaligned_write_buffer);
    
    memset(aligned_read_buffer,0,MAX_SECTORS*512);
    //    memset(aligned_write_buffer,0,MAX_SECTORS*512);
    memset(unaligned_read_buffer,0,MAX_SECTORS*512);
    //    memset(unaligned_write_buffer,0,MAX_SECTORS*512);

    for( int i = 0; i < (MAX_SECTORS*512); i++ ) {
        aligned_write_buffer[i] = (char)(i&0xff);
        unaligned_write_buffer[i] = (char)(i&0xff);
    }
    
    DEBUGP(ALIGN,DBGLEV_INFO,"done initializing\n");
    
    const char * dev_name = "/dev/hda/";
    DEBUGP(ALIGN,DBGLEV_INFO,"looking up %s\n", dev_name);
    if (cyg_io_lookup(dev_name, &blk_devA) != ENOERR) {
	CYG_TEST_EXIT("Could not get handle to dev");
    }
    DEBUGP(ALIGN,DBGLEV_INFO,"got handle to %s\n", dev_name);
    
    len = sizeof(len);

    if (cyg_io_set_config(blk_devA, IO_PM_SET_CONFIG_REGISTER, 0, &len) != ENOERR) {
	CYG_TEST_EXIT("Could not power up device");
    }

    len = sizeof(dg);
    if (cyg_io_get_config(blk_devA, IO_BLK_GET_CONFIG_GEOMETRY, &dg, &len) != ENOERR) {
	CYG_TEST_EXIT("Could not get geometry");
    }
    diag_printf("C/H/S: %d/%d/%d\n", dg.cyl, dg.hd, dg.sec);
    diag_printf("Sector Size: %d\n", dg.bytes_p_sec);
    diag_printf("Total Sectors: %d\n", dg.num_blks);
    dg.serial_num[40] = 0; dg.model_num[40] = 0;
    diag_printf("SN: %s MN: %s\n", dg.serial_num, dg.model_num);

    cyg_uint32 max_lba = dg.num_blks - (MAX_SECTORS - 1);
        
    /* perform write/read tests */
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
	DEBUGP(ALIGN,DBGLEV_INFO,"LBA: %06x Sectors: %04x (%06x total)\n", lba, length, count);

        DEBUGP(ALIGN,DBGLEV_INFO,"\n--------Comparing aligned write/read--------\n");
	cyg_uint32 tmp_length = length * 512;

        // Write the data from the aligned write buffer
        err = cyg_io_bwrite( blk_devA, aligned_write_buffer, &tmp_length, lba );

	if( err != ENOERR ) {
	    DEBUG(ALIGN,DBGLEV_ERROR,"Error reading %d\n", err);
	    continue;
	}
	DEBUGP(ALIGN,DBGLEV_INFO,"wa");

        // Read the data into the aligned read buffer
	err = cyg_io_bread(blk_devA, aligned_read_buffer, &tmp_length, lba);

	if( err != ENOERR ) {
	    DEBUG(ALIGN,DBGLEV_ERROR,"Error writing %d\n", err);
	    continue;
	}
	DEBUGP(ALIGN,DBGLEV_INFO,"ra");

        // Compare the read back data to the original
        CompareData( aligned_read_buffer, aligned_write_buffer, length );


        DEBUGP(ALIGN,DBGLEV_INFO,"\n--------Comparing aligned write/unaligned read--------\n");
        // Read the data into the unaligned read buffer
        err = cyg_io_bread(blk_devA, unaligned_read_buffer, &tmp_length, lba );
	if( err != ENOERR ) {
	    DEBUG(ALIGN,DBGLEV_ERROR,"Error reading %d\n", err);
	    continue;
	}
	DEBUGP(ALIGN,DBGLEV_INFO,"ru");

        // Compare the results
        CompareData( unaligned_read_buffer, aligned_write_buffer, length );

        DEBUGP(ALIGN,DBGLEV_INFO,"\n--------Comparing unaligned write/aligned read--------\n");
        // Write the data from the unaligned buffer
        err = cyg_io_bwrite( blk_devA, unaligned_write_buffer, &tmp_length, lba );

	if( err != ENOERR ) {
	    DEBUG(ALIGN,DBGLEV_ERROR,"Error reading %d\n", err);
	    continue;
	}
	DEBUGP(ALIGN,DBGLEV_INFO,"wu");
        
        
        // Read the data into the aligned read buffer
	err = cyg_io_bread(blk_devA, aligned_read_buffer, &tmp_length, lba);

	if( err != ENOERR ) {
	    DEBUG(ALIGN,DBGLEV_ERROR,"Error writing %d\n", err);
	    continue;
	}
	DEBUGP(ALIGN,DBGLEV_INFO,"ra");

        // Compare the results
        CompareData( aligned_read_buffer, unaligned_write_buffer, length );
        
	DEBUGP(ALIGN,DBGLEV_INFO,"\n");
    }  
    DEBUGP(ALIGN,DBGLEV_INFO,"\naligned/unaligned rwc tests done\n");
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
	DEBUGP(ALIGN,DBGLEV_INFO,"c");
    else {
	DEBUGP(ALIGN,DBGLEV_ERROR,"\n***MISCOMPARE @[%d] W%04X R=%04X\n", i, write_buffer[i], read_buffer[i]);
	for (;;)
	    ;
    }
}
