// copyc_test.cpp: copy from one physical device to the other, compare written data
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

#include <devs/storage/ata/atadrv.h>

#include <util/debug/debug.h>

DEBUG_MODULE_S(COPYC, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(COPYC);

/* DEFINES */

//#define PERFORM_SEQUENTIAL_READWRITE_TEST
//#define PERFORM_RANDOM_READWRITE_TEST
#define PERFORM_CDA_COPY_TEST

#define PERFORM_COMPARE_STEP
#define PERFORM_TIME_ANALYSIS

//#define DYNAMIC_BUFFERS
//#define UNALIGN_STATIC_BUFFERS

#if defined(PERFORM_TIME_ANALYSIS)
#include <pkgconf/kernel.h>
#define _1NS 1000000000
#endif

#define MAX_SECTORS 256
//#define MAX_LBA ((1048576 - MAX_SECTORS) / 2)
#define MAX_LBA 4096

#define SECTOR_COUNT 4096

#ifdef PERFORM_CDA_COPY_TEST
#define BUFFER_SIZE 225792*2
#else
#define BUFFER_SIZE 512*MAX_SECTORS
#endif

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
    DBEN(COPYC);
    cyg_thread_create(10, atapi_thread, (cyg_addrword_t) 0, "atapi_thread",
                      (void *)_stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
    cyg_thread_resume(thread[0]);
    DBEX(COPYC);
}

/* Globals */
void CompareData(char * read_buffer, char * write_buffer, cyg_uint16 length);
static int CDARead(cyg_io_handle_t handle, long lba, long sectors, void* buffer );

void
atapi_thread(unsigned int ignored)
{
    cyg_uint16 length;
    cyg_uint32 lba;
    cyg_uint32 len;
    Cyg_ErrNo err;
    cyg_io_handle_t blk_devA, blk_devB;
    
#if defined(PERFORM_TIME_ANALYSIS)
    cyg_tick_count_t wtotal_time, wstart_time, rtotal_time, rstart_time;
    cyg_uint32 tick_length = _1NS/(CYGNUM_HAL_RTC_NUMERATOR/CYGNUM_HAL_RTC_DENOMINATOR);
#endif
    
    srand(0x5834971);
    
    /* Initialize data buffers */
#if defined(DYNAMIC_BUFFERS)
    char* read_buffer  = new char[BUFFER_SIZE];  
    char* write_buffer = new char[BUFFER_SIZE];
#elif defined(UNALIGN_STATIC_BUFFERS)
    static char rbuffer[ 3 + BUFFER_SIZE ];
    static char wbuffer[ 3 + BUFFER_SIZE ];

    char* read_buffer  = rbuffer + 1;
    char* write_buffer = wbuffer + 1;
#else
    static char read_buffer[ BUFFER_SIZE ];
    static char write_buffer[ BUFFER_SIZE ];
#endif

    DEBUGP(COPYC,DBGLEV_INFO,"buffer size %d\n", BUFFER_SIZE );
    DEBUGP(COPYC,DBGLEV_INFO,"read_buffer %x\nwrite_buffer %x\n", read_buffer, write_buffer);
    
    memset(read_buffer,0,BUFFER_SIZE);
    DEBUG(COPYC,DBGLEV_INFO,"done initializing\n");

    const char * dev_name = "/dev/hda/";
    DEBUG(COPYC,DBGLEV_INFO,"looking up %s\n", dev_name);
    if (cyg_io_lookup(dev_name, &blk_devA) != ENOERR) {
        CYG_TEST_EXIT("Could not get handle to dev");
    }
    DEBUG(COPYC,DBGLEV_INFO,"got handle to %s\n", dev_name);
    
    dev_name = "/dev/cda/";
    DEBUG(COPYC,DBGLEV_INFO,"looking up %s\n", dev_name);
    if( cyg_io_lookup(dev_name, &blk_devB) != ENOERR) {
        CYG_TEST_EXIT("Could not get handle to dev");
    }
    DEBUG(COPYC,DBGLEV_INFO,"got handle to %s\n", dev_name);
    
    len = sizeof(len);

    if (cyg_io_set_config(blk_devA, IO_PM_SET_CONFIG_REGISTER, 0, &len) != ENOERR) {
        CYG_TEST_EXIT("Could not power up device");
    }

    if (cyg_io_set_config(blk_devB, IO_PM_SET_CONFIG_REGISTER, 0, &len) != ENOERR) {
        CYG_TEST_EXIT("Could not power up device");
    }
    
    int count;
    
#if defined(PERFORM_RANDOM_READWRITE_TEST) || defined(PERFORM_SEQUENTIAL_READWRITE_TEST)
    /* perform write/read tests */
#if defined(PERFORM_TIME_ANALYSIS)
    wtotal_time = rtotal_time = 0;
#endif
    lba = 100;
    for(count = 0; count < SECTOR_COUNT; )
    {
#if defined(PERFORM_SEQUENTIAL_READWRITE_TEST)
        length = 32;
        lba += length;
#else
        lba = rand() / (RAND_MAX/MAX_LBA);
        length = (cyg_uint16)(((cyg_uint32)rand()) / (RAND_MAX/MAX_SECTORS));
        length += 4; // do at least one sector
        if (length > MAX_SECTORS) {
            length = MAX_SECTORS;
        }
        length -= (length % 4);
        lba += 50;
        if( lba > MAX_LBA ) {
            lba = MAX_LBA;
        }
#endif

        count += length;
        DEBUGP(COPYC,DBGLEV_INFO,"LBA: %06x Sectors: %04x (%06x total)\n", lba, length, count);

        cyg_uint32 tmp_length = length * 512;

#if defined(PERFORM_TIME_ANALYSIS)
        rstart_time = cyg_current_time();
#endif
        
        err = cyg_io_bread(blk_devB, write_buffer, &tmp_length, lba);
        
#if defined(PERFORM_TIME_ANALYSIS)
        rtotal_time += (cyg_current_time() - rstart_time);
#endif
        if( err != ENOERR ) {
            DEBUG(COPYC,DBGLEV_ERROR,"Error reading %d\n", err);
            continue;
        }
        DEBUGP(COPYC,DBGLEV_INFO,"r");
	
#if defined(PERFORM_TIME_ANALYSIS)
        wstart_time = cyg_current_time();
#endif
        err = cyg_io_bwrite(blk_devA, write_buffer, &tmp_length, lba);
#if defined(PERFORM_TIME_ANALYSIS)
        wtotal_time += (cyg_current_time() - wstart_time);
#endif

        if( err != ENOERR ) {
            DEBUG(COPYC,DBGLEV_ERROR,"Error writing %d\n", err);
            continue;
        }
        DEBUGP(COPYC,DBGLEV_INFO,"w");

        // now read the data back from B
#ifdef PERFORM_COMPARE_STEP
        tmp_length = length * 512;

        err = cyg_io_bread(blk_devA, read_buffer, &tmp_length, lba );
        if( err != ENOERR ) {
            DEBUG(COPYC,DBGLEV_ERROR,"Error reading %d\n", err);
            continue;
        }
        DEBUGP(COPYC,DBGLEV_INFO,"r");
            
        CompareData(read_buffer, write_buffer, length);
#endif
        DEBUGP(COPYC,DBGLEV_INFO,"\n");
    }
#if defined(PERFORM_SEQUENTIAL_READWRITE_TEST)
    DEBUGP(COPYC,DBGLEV_INFO,"\nsequential write read compares done\n");
#else
    DEBUGP(COPYC,DBGLEV_INFO,"\nrandom write read compares done\n");
#endif
#if defined(PERFORM_TIME_ANALYSIS)
    DEBUGP(COPYC,DBGLEV_INFO,"\n%d sectors read in %d time ", count, rtotal_time);
    DEBUGP(COPYC,DBGLEV_INFO,"(%6d bytes/sec)\n",(count*512*tick_length)/rtotal_time);
    DEBUGP(COPYC,DBGLEV_INFO,"\n%d sectors written in %d time ", count, wtotal_time );
    DEBUGP(COPYC,DBGLEV_INFO,"(%6d bytes/sec)\n",(count*512*tick_length)/wtotal_time);
#endif
#endif
    
#if defined(PERFORM_CDA_COPY_TEST)
    DEBUGP(COPYC,DBGLEV_INFO,"\nPerforming raw cda copy test\n");
    DEBUGP(COPYC,DBGLEV_INFO,"Sequential LBA, CDA copy\n");

    wtotal_time = 0;
    int cda_lba = 23091;     // arbitrary ?
    int hda_lba  = 512;
    for( count = 0; count < SECTOR_COUNT*4; ) {
        int secs = 128;
        cda_lba += secs;   // 128 * 2352 = 301056
        hda_lba += 588;
        length = secs * 2352;
        
        count += (secs);
        //        DEBUGP(COPYC,DBGLEV_INFO,"LBA: %06x Sectors: %04x (%06x total)\n", lba, secs, count);

        cyg_uint32 cda_length = secs;
        cyg_uint32 sec_length = length;// /  512;
        
#if defined(PERFORM_TIME_ANALYSIS)
        rstart_time = cyg_current_time();
#endif
        if( CDARead( blk_devB, cda_lba, cda_length, write_buffer ) != cda_length ) {
            DEBUG(COPYC, DBGLEV_ERROR, "CDARead returned less than tmp_length sectors read\n");
            continue;
        }
        
        err = cyg_io_bwrite(blk_devA, write_buffer, &sec_length, hda_lba);
        if( err != ENOERR ) {
            DEBUG(COPYC, DBGLEV_ERROR, "Error writing %d\n", err );
            continue;
        }
#if defined(PERFORM_TIME_ANALYSIS)
        wtotal_time += (cyg_current_time() - rstart_time);
#endif
#if defined(PERFORM_COMPARE_STEP)
        // perform compare step - this might hose drive caching?
        err = cyg_io_bread(blk_devA, read_buffer, &sec_length, hda_lba );
        if( err != ENOERR ) {
            DEBUG(COPYC, DBGLEV_ERROR, "Error rereading %d\n", err );
            continue;
        }
        CompareData( read_buffer, write_buffer, length );
        DEBUGP(COPYC,DBGLEV_INFO,"\n");
#endif
    }
#if defined(PERFORM_TIME_ANALYSIS)
    DEBUGP(COPYC, DBGLEV_INFO, "\n%d sectors (2352 bytes each) copied in %d time ", count, wtotal_time );
    DEBUGP(COPYC, DBGLEV_INFO, "(%6d bytes/sec)\n",(count*2352*tick_length)/wtotal_time );
#endif
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
        DEBUGP(COPYC,DBGLEV_INFO,"c");
    else {
        DEBUG(COPYC,DBGLEV_ERROR," MISCOMPARE @[%d] W%04X R=%04X\n", i, write_buffer[i], read_buffer[i]);
        for (;;)
            ;
    }
}

static Cyg_ErrNo DoPacket(cyg_io_handle_t handle, unsigned char *command, unsigned char *data, int iDataLength,int dir);


static int CDARead(cyg_io_handle_t handle, long lba, long sectors, void* buffer ) 
{
    Cyg_ErrNo ret=0;
    unsigned char packet_command[12];
    unsigned char packet_command_READMMC[12]={0xbe,0,0,0,0,0,0,0,0,0xf8,0,0};
    int iErrCount = 0;

    while(sectors)
    {
        memcpy(packet_command,packet_command_READMMC,12);
        packet_command[3]= (lba >>16) & 0xff;
        packet_command[4]= (lba >>8) & 0xff;
        packet_command[5]= lba & 0xff;
        packet_command[8]= sectors;
        ret=DoPacket(handle, packet_command,(unsigned char *)buffer,sectors*2352,ATAPI_DATA_IN);
        if(ret==ENOERR){
            return(sectors);
        }
                
        if (++iErrCount > 5)
        {
            sectors>>=1;
            iErrCount = 0;
        }
    }
    if(ret<0){
        return(ret);
    }
        
    return(-EIO);
}

static Cyg_ErrNo DoPacket(cyg_io_handle_t m_hCD, unsigned char *command, unsigned char *data, int iDataLength,int dir)
{
    ATAPICommand_T Cmd;
    cyg_uint32 Length;
    int Status;

    memset(&Cmd, 0, sizeof(Cmd));
    Cmd.Flags = dir; //(dir | ATAPI_AUTO_SENSE);
    Cmd.Data = (char *)data;
    Cmd.DataLength = iDataLength;
    Cmd.Timeout = 100 * 30; /* 30s */
	
    memcpy(&Cmd._SCSICmd, command, 12);
    Cmd.SCSICmd = &Cmd._SCSICmd;
    Cmd.SCSICmdLength = 12;
	
    Length = sizeof(Cmd);
    Status = cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &Cmd, &Length);
    if (Status) {
        unsigned char RequestSenseData[14]={0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        unsigned char packet_command_RequestSense[12]={0x03,0,0,0,14,0,0,0,0,0,0,0};	
		

        memset(&Cmd, 0, sizeof(Cmd));
        Cmd.Flags = ATAPI_DATA_IN;
        Cmd.Data = (char *)RequestSenseData;
        Cmd.DataLength = sizeof(RequestSenseData);
        Cmd.Timeout = 100 * 30; /* 30s */

        memcpy(&Cmd._SCSICmd, packet_command_RequestSense, sizeof(packet_command_RequestSense));
        Cmd.SCSICmd = &Cmd._SCSICmd;
        Cmd.SCSICmdLength = 12;

        Length = sizeof(Cmd);
        int RSStatus = cyg_io_set_config(m_hCD, IO_ATAPI_SET_CONFIG_EXEC_COMMAND, &Cmd, &Length);
        if (RSStatus) {
            //diag_printf("Request Sense failed %x\n", RSStatus);
        }
        Status = RequestSenseData[12];
    }
    return Status;
}
