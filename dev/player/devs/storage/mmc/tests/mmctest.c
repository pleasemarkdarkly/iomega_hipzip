#include <stdio.h>
#include <cyg/infra/diag.h>
#include <cyg/infra/testcase.h>
#include <cyg/io/io.h>//for cyg_lookup
#include <cyg/kernel/kapi.h>//for Cygthread..
#include <cyg/hal/hal_edb7xxx.h>//for PDDR
#include <pkgconf/kernel.h> // for CYGNUM_HAL_RTC_DENOMINATOR

#include <io/storage/blk_dev.h>
#include <devs/storage/mmc/_mmc_dev.h>

#ifdef BLOCK_DEV_HDA_NAME
#define USE_DRIVE_A
#endif // BLOCK_DEV_HDA_NAME

#ifdef BLOCK_DEV_HDB_NAME
#define USE_DRIVE_B
#endif // BLOCK_DEV_HDB_NAME

#define CHECK_MEDIA_PRESENT
#define CHECK_DRIVE_GEOMETRY

//#define DO_RANDOM_RW_COMPARE_TEST
#define DO_READ_THROUGHPUT_TEST
#define DO_WRITE_THROUGHPUT_TEST
//#define MASK_INTERRUPTS_FOR_THROUGHPUT

#ifdef MASK_INTERRUPTS_FOR_THROUGHPUT
extern void hal_interrupt_mask( int );
extern void hal_interrupt_unmask( int );
#endif

#if !defined(USE_DRIVE_A) && !defined(USE_DRIVE_B)
#error "You must define a drive to test"
#endif

//#include <dadio/io/pm.h>//key use

#define KByte 1024
#define MAX_SECTORS 16  /* this is malloc'd so be really careful on the ijam device */
#define MAX_LBA 0x3800 /* set to 7mb worth of sectors->7m/512=0x3800 */

//int  drive_open_flag[TOTAL_DRIVES];
//DRV_GEOMETRY_DESC  idDrv[TOTAL_DRIVES];

void func(unsigned int);
void CompareData(cyg_uint8 * read_buffer, cyg_uint8 * write_buffer, cyg_uint16 length);
void TestMediaPresent( cyg_io_handle_t drv );
void GetDriveGeometry( cyg_io_handle_t drv );

#define NTHREADS 1
#define STACKSIZE ((1 * 4096))

static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char stack[NTHREADS][STACKSIZE];

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void cyg_user_start(void) 
{
    cyg_thread_create(10,func,(cyg_addrword_t)0,"func",(void*)stack[0],STACKSIZE,&thread[0],&thread_obj[0]);  
    cyg_thread_resume(thread[0]);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define _1NS 1000000000
void func(unsigned int cheese)
{  
    static cyg_uint8 read_buffer[MAX_SECTORS * 512],
                     write_buffer[MAX_SECTORS * 512];

    // calculate the tick length
    cyg_uint32 tick_length = _1NS/(CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR);
    cyg_uint32 length;
    unsigned int i;
    Cyg_ErrNo err;
#ifdef USE_DRIVE_A
    cyg_io_handle_t blk_deva;
#endif
#ifdef USE_DRIVE_B
    cyg_io_handle_t blk_devb;
#endif

    /* Initialize Write/Read Buffers */
    diag_printf("system tick measured as %d Hz\n", tick_length );
    diag_printf("initializing write/read buffers\n");
    for(i=0;i<(MAX_SECTORS*512);i++) write_buffer[i]=(cyg_uint8)(i&0xff);
    memset(read_buffer,0,MAX_SECTORS*512);
    diag_printf("done initializing\n");

#ifdef USE_DRIVE_A
    diag_printf("****************looking up hda*****************\n");
    if (cyg_io_lookup(BLOCK_DEV_HDA_NAME, &blk_deva) != ENOERR) CYG_TEST_EXIT("Could not get handle to drive A");
    diag_printf("got handle to drive A (%08x)\n", blk_deva);
    
    TestMediaPresent( blk_deva );
    GetDriveGeometry( blk_deva );
#endif
    
#ifdef USE_DRIVE_B
    diag_printf("****************looking up hdb*****************\n");
    if (cyg_io_lookup(BLOCK_DEV_HDB_NAME, &blk_devb) != ENOERR) CYG_TEST_EXIT("Could not get handle to drive B");   
    diag_printf("got handle to drive B (%08x)\n", blk_devb);
    
    TestMediaPresent( blk_devb );
    GetDriveGeometry( blk_devb );
#endif
    
    diag_printf("******************look up done*****************\n");

#ifdef DO_RANDOM_RW_COMPARE_TEST
    /* Test random read/write/compare */
    diag_printf("Testing random read/write compares\n");
    diag_printf(" write_buffer = %08x, read_buffer = %08x\n", write_buffer, read_buffer );
    lba = 60;
    for(;;)
    {
      //	lba = rand() / (RAND_MAX/MAX_LBA);
      //	length = (cyg_uint16)(((cyg_uint32)rand()) / (RAND_MAX/MAX_SECTORS));
      lba++;
      length = 40;
	if (length > MAX_SECTORS) length = MAX_SECTORS;
	if( lba == GeomDesc.num_blks - 1 ) lba = 60;
	diag_printf("LBA: %06x Sectors: %04x\n", lba, length);

#ifdef USE_DRIVE_A
	diag_printf("testing card a\n");
	
	diag_printf("w");
	if((err=cyg_io_bwrite(blk_deva, write_buffer, &length, lba))!=ENOERR) 
	{
	    diag_printf("Error writing %d\n", err);
	    while( 1 > 0);
	    continue;
	}
				
	memset(read_buffer, 0, 512*MAX_SECTORS);	   
	diag_printf("r");
	if( (err=cyg_io_bread(blk_deva, read_buffer, &length, lba) ) != ENOERR)
	{
	    diag_printf("Error reading %d\n", err);
	    while( 1 > 0);
	    continue;
	}
	
	CompareData(read_buffer, write_buffer,length);
#endif // USE_DRIVE_A
#ifdef USE_DRIVE_B

	diag_printf("testing card b\n");
	if((err = cyg_io_bwrite( blk_devb, write_buffer, &length, lba)) != ENOERR )
	{
	  diag_printf("Error writing %d\n", err );
	  while( 1 > 0 );
	}

	memset( read_buffer, 0, 512*MAX_SECTORS);
	diag_printf("r");
	if( (err=cyg_io_bread(blk_devb, read_buffer, &length, lba) ) != ENOERR)
	{
	  diag_printf("Error reading %d\n", err );
	  while( 1 > 0 );
	  continue;
	}
	CompareData( read_buffer, write_buffer, length );
#endif // USE_DRIVE_B
	diag_printf("\n");
    }
#endif // test r/w

#if defined(DO_READ_THROUGHPUT_TEST) || defined(DO_WRITE_THROUGHPUT_TEST) // test throughput
    {
      cyg_tick_count_t start_time,end_time;
      int one = 5;
#ifdef MASK_INTERRUPTS_FOR_THROUGHPUT
      diag_printf("masking all but timer interrupts\n");
      for( i = 0; i < 9; i++ ){
	hal_interrupt_mask( i );
      }
      for( i = 13; i < 22; i++ ) {
	hal_interrupt_mask( i );
      }
#endif // MASK_ ...
#if defined(DO_READ_THROUGHPUT_TEST) && defined(USE_DRIVE_A)
      diag_printf(" test read throughput card a\n");
      length = 0x500;
      start_time = cyg_current_time();
      for( i = 0; i < length; i++ ) {
	err = cyg_io_bread( blk_deva, read_buffer, &one, i );
	if( err != ENOERR ) {
	  diag_printf("error reading %d\n", err );
	  return ;
	  //	  while( 1 > 0 );
	}
      }
      end_time = cyg_current_time();
      diag_printf(" read %d sectors in %5d time", length*one, (end_time-start_time) );
      diag_printf("(%7d bytes/sec)\n", (length*one*tick_length*512)/(end_time-start_time));
#endif
#if defined(DO_WRITE_THROUGHPUT_TEST) && defined(USE_DRIVE_A)
      diag_printf(" test write throughput card a\n");
      length = 0x500;
      start_time = cyg_current_time();
      for(i = 0; i < length; i++ ) {
	err = cyg_io_bwrite( blk_deva, write_buffer, &one, i );
	if( err != ENOERR ) {
	  diag_printf("error writing %d\n", err );
	  return ;
	}
      }
      end_time = cyg_current_time();
      diag_printf(" wrote %d sectors in %5d time", length*one, (end_time-start_time) );
      diag_printf("(%7d bytes/sec)\n", (length*one*tick_length*512)/(end_time-start_time));
#endif
#if defined(DO_READ_THROUGHPUT_TEST) && defined(USE_DRIVE_B)
      diag_printf(" test read throughput card b\n");
      length = 0x500;
      start_time = cyg_current_time();
      for( i = 0; i < length; i++ ) {
	err = cyg_io_bread( blk_devb, read_buffer, &one, i );
	if( err != ENOERR ) {
	  diag_printf("error reading %d\n", err );
	  return ;
	  //	  while( 1 > 0 );
	}
      }
      end_time = cyg_current_time();
      diag_printf(" read %d sectors in %5d time", length*one, (end_time-start_time) );
      diag_printf("(%7d bytes/sec)\n", (length*one*tick_length*512)/(end_time-start_time));
#endif
#if defined(DO_WRITE_THROUGHPUT_TEST) && defined(USE_DRIVE_B)
      diag_printf(" test write throughput card b\n");
      length = 0x500;
      start_time = cyg_current_time();
      for(i = 0; i < length; i++ ) {
	err = cyg_io_bwrite( blk_devb, write_buffer, &one, i );
	if( err != ENOERR ) {
	  diag_printf("error writing %d\n", err );
	  return ;
	}
      }
      end_time = cyg_current_time();
      diag_printf(" wrote %d sectors in %5d time", length*one, (end_time-start_time) );
      diag_printf("(%7d bytes/sec)\n", (length*one*tick_length*512)/(end_time-start_time));
#endif
    }
    
#endif // read or write throughput test
    diag_printf("******************** done *****************\n");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CompareData(cyg_uint8 * read_buffer, cyg_uint8 * write_buffer, cyg_uint16 length)
{    
    cyg_uint32 i;
    int err = 0;
    
    for (i=0; i < length * 512; i++)    
    {
	if(write_buffer[i]!=read_buffer[i]) {
	  diag_printf(" MISCOMPARE @[%d] W%04X R=%04X\n", i, write_buffer[i], read_buffer[i]);
	  err = 1;
	}
    }
    while( err > 0 ) ;
	
    if(i==length*(512)) diag_printf("c");	
    else 
    {
	for (;;) ;
    }
}

void TestMediaPresent( cyg_io_handle_t drv ) 
{
#ifdef CHECK_MEDIA_PRESENT
  cyg_uint32 len, media_present;
  Cyg_ErrNo err;
  for(;;) {
    len = sizeof(media_present);
    if(( err = cyg_io_get_config( drv, IO_BLK_GET_CONFIG_MEDIA_STATUS, &media_present, &len) != ENOERR)) {
      diag_printf( "handle %08x media status failed\n", drv );
      while( 1 > 0 ); // halt
    } else {
      diag_printf( "handle %08x media status succeeded\n", drv );
      break;
    }
  }
#endif
}

void GetDriveGeometry( cyg_io_handle_t drv ) 
{
#ifdef CHECK_DRIVE_GEOMETRY
  drive_geometry_t GeomDesc;
  cyg_uint32 len;
  Cyg_ErrNo err;
  
  diag_printf( "Getting drive geometry for handle %08x\n", drv );
  len = sizeof(GeomDesc);
  if((err=cyg_io_get_config( drv , IO_BLK_GET_CONFIG_GEOMETRY, &GeomDesc, &len))!=ENOERR)
  {
    diag_printf("Error getting geometry %d\n", err);
  }
  else
  {
    int z;
    diag_printf("Get geometry succeeded\n");
    diag_printf("Total LBA %d\n", GeomDesc.num_blks);
    diag_printf("C/H/S %d/%d/%d\n", GeomDesc.cyl, GeomDesc.hd, GeomDesc.sec);
    //    diag_printf("Serial #%s, Model #%s\n", GeomDesc.serial_num, GeomDesc.model_num);
    diag_printf("Serial # (40 bytes hex, 16 data 24 zero pad):\n");
    for( z = 0; z < 40; z++ ) {
      diag_printf("%02x", GeomDesc.serial_num[z]);
      if( z == 19 ) diag_printf("\n");
    }
    diag_printf("\nModel # (hex):\n");
    for( z = 0; z < 20; z++ ) {
      diag_printf("%02x", GeomDesc.model_num[z]);
    }
    diag_printf("\n");
  }
#endif
}
