// copyc_test.cpp: copy from one physical device to the other, compare written data
// assumption: source media has data on it
// danc@iobjects.com 07/04/01
// (c) Interactive Objects

#include <stdlib.h>   /* rand, RAND_MAX*/
#include <string.h>   /* memset */

#include <cyg/kernel/kapi.h>
#include <cyg/io/io.h>
#include <cyg/infra/testcase.h>
#include <cyg/kernel/thread.inl>

#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <devs/storage/ata/atadrv.h>

#include <util/debug/debug.h>

//DEBUG_MODULE_S(COPYC, DBGLEV_DEFAULT | DBGLEV_INFO);
//DEBUG_USE_MODULE(COPYC);


#define MAX_SECTORS 256
//#define MAX_LBA ((1048576 - MAX_SECTORS) / 2)
#define MAX_LBA 2048

#define CDA_START 0x1e20
#define CDA_TEST 128

#define HDA_START 0
#define HDA_TEST 128

#define SECTOR_COUNT 2048


// should hold 0xF0 CDA sectors
#define BUFFER_SIZE 600000

#define NTHREADS 1
#define STACKSIZE ( (16 * 4096) )

/* STATICS */
static cyg_handle_t thread[NTHREADS];
static cyg_thread thread_obj[NTHREADS];
static char _stack[NTHREADS][STACKSIZE];


extern "C" 
{    
	#include "parser.h"
	#include "cmds.h"
	#include "io.h"

	int test_stress(char param_strs[][MAX_STRING_LEN],int* param_nums);
};

/* Globals */
bool CompareData(char * read_buffer, char * write_buffer, cyg_uint32 length);
int CDARead(cyg_io_handle_t handle, long lba, long sectors, void* buffer );
int stress(char param_strs[][MAX_STRING_LEN],int* param_nums,bool bCD, bool bHD, int cdlen, int hdlen);
static volatile bool bRunning;

void mem_stress(unsigned int ignored)
{
	// test 8mb of ram
	
	while(bRunning)
	{
		memtest(1024*1024);
	}

	DEBUG3("memtest thread done\n");
}


int
test_hdstress(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	if(param_nums[0] > 0)
	{
		return stress(param_strs,param_nums,false,true,0,param_nums[0]);
	}
	else
	{
		DEBUG3("invalid params, must enter length\n");
		return TEST_ERR_PARAMS;
	}

}


int
test_cdstress(char param_strs[][MAX_STRING_LEN],int* param_nums)
{

	if(param_nums[0] > 0)
	{
		return stress(param_strs,param_nums,true,false,param_nums[0],0);
	}
	else
	{
		DEBUG3("invalid params, must enter length\n");
		return TEST_ERR_PARAMS;
	}

}

int
test_stress(char param_strs[][MAX_STRING_LEN],int* param_nums)
{ 

	return stress(param_strs,param_nums,true,true,CDA_TEST,HDA_TEST);

}

int
stress(char param_strs[][MAX_STRING_LEN],int* param_nums,bool bCD, bool bHD, int cdlen, int hdlen)
{ 
	int ret;

	bool bFailfast;
	bRunning = true;
	if(param_strs[1][0] != '\0')
	{
		if(strncmpci(param_strs[1],"false",MAX_STRING_LEN) == 0)
		{
			bFailfast = false;
		}
		else if(strncmpci(param_strs[1],"true",MAX_STRING_LEN) == 0)
		{
			bFailfast = true;
		}
		else
		{
			DEBUG2("invalid params");
			return TEST_ERR_PARAMS;
		}
  

	}
	else
	{
		// fail fast by default
		bFailfast = true;
	}



	// start memory stress operations, same priority as main thread (8)

	cyg_thread_create(8, mem_stress, (cyg_addrword_t) 0, "stress",
                      (void *)_stack[0], STACKSIZE, &thread[0], &thread_obj[0]);
    cyg_thread_resume(thread[0]);

	int cda_lba;     // arbitrary ?
    int hda_lba;
  
    cyg_uint16 length;
    cyg_uint32 lba;
    cyg_uint32 len;
    Cyg_ErrNo err;
    cyg_io_handle_t blk_devA, blk_devB;
        

	int secs = 0xF0;
	cyg_uint32 hda_length = 512*secs; 
	cyg_uint32 cda_length = secs;

    srand(0x5834971);
    
    /* Initialize data buffers */
    char* read_buffer  = new char[BUFFER_SIZE];  
    char* write_buffer = new char[BUFFER_SIZE];

    DEBUG3("buffer size %d\n", BUFFER_SIZE );
    DEBUG3("read_buffer %x\nwrite_buffer %x\n", read_buffer, write_buffer);
    
    memset(read_buffer,0,BUFFER_SIZE);
    DEBUG3("done initializing\n");

    const char * dev_name = "/dev/hda/";
    //  DEBUG(COPYC,DBGLEV_INFO,"looking up %s\n", dev_name);
    if (cyg_io_lookup(dev_name, &blk_devA) != ENOERR) {
        DEBUG2("STRESS ERROR: Could not get handle to dev");
		goto fail;
    }

    DEBUG3("got handle to %s\n", dev_name);
    
    dev_name = "/dev/cda/";
    DEBUG3("looking up %s\n", dev_name);
    if( cyg_io_lookup(dev_name, &blk_devB) != ENOERR) {
        DEBUG3("STRESS ERROR: Could not get handle to dev");
		goto fail;
    }
    DEBUG3("got handle to %s\n", dev_name);
    
    len = sizeof(len);

    if (cyg_io_set_config(blk_devA, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
        DEBUG3("STRESS ERROR: Could not power up a device");
		goto fail;

    }

    if (cyg_io_set_config(blk_devB, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
        DEBUG3("STRESS ERROR: Could not power up b device");
		goto fail;

    }
    
     if (cyg_io_set_config(blk_devA, IO_ATA_SET_CONFIG_FEATURES, 0, &len) != ENOERR) {
        DEBUG3("STRESS ERROR: Could not power up a device");
		goto fail;

    }

    if (cyg_io_set_config(blk_devB, IO_ATAPI_SET_CONFIG_FEATURES, 0, &len) != ENOERR) {
        DEBUG3("STRESS ERROR: Could not power up b device");
		goto fail;

    }
	if(bHD)
	{

		DEBUG2("HD R/W Verify Test\n");

		for(hda_lba = HDA_START; hda_lba < (HDA_START + hdlen); hda_lba++)
		{

		
			DEBUG3("HD: Testing %d sectors at LBA %d\n", secs, hda_lba);

			// read a sector
			err = cyg_io_bread(blk_devA, read_buffer, &hda_length, hda_lba );
			if( err != ENOERR ) {
				DEBUG1("STRESS ERROR: reading %d\n", err );
				goto fail;
			}

			// copy it
			memcpy(write_buffer,read_buffer,hda_length);

			// write it back
			err = cyg_io_bwrite(blk_devA, write_buffer, &hda_length, hda_lba);
			if( err != ENOERR ) {
				DEBUG1("STRESS ERROR: writing %d\n", err );				
				goto fail;
			}

			// read it again
			err = cyg_io_bread(blk_devA, read_buffer, &hda_length, hda_lba );
			if( err != ENOERR ) {
				DEBUG1("STRESS ERROR: rereading %d\n", err );
				goto fail;
			}

			// compare results
			if(!CompareData( read_buffer, write_buffer, hda_length ))
			{
				DEBUG1("STRESS ERROR: HD integrity failure\n");
				if(bFailfast)
					goto fail;
			} 

		} 
	}

	if(bCD)
	{
		DEBUG2("CD Read Verify Test\n");

#if 0 // old CDA test
		for(cda_lba = CDA_START; cda_lba < (CDA_START + cdlen); cda_lba++)
		{
		
			DEBUG3("CD: Testing %d sectors at LBA %d\n", secs, cda_lba);
			
			// spin-up issues?
			while(-EIO == CDARead( blk_devB, cda_lba, cda_length, write_buffer ));

			CDARead( blk_devB, cda_lba, cda_length, read_buffer );

			if(!CompareData( read_buffer, write_buffer, cda_length*2352 ))
			{
				DEBUG1("STRESS ERROR: CD integrity failure\n");
				if(bFailfast)
					goto fail;
			}
		}
#else
		lba = 100;
		unsigned long count;
		for(count = 0; count < cdlen; )
		{
			cyg_uint32 tmp_length;
			length = 32;
			lba += length;

			count += length;
			DEBUG3("LBA: %06x Sectors: %04x (%06x total)\n", lba, length, count);

			tmp_length = length * 512;

		    err = cyg_io_bread(blk_devB, read_buffer, &tmp_length, lba);

			if( err != ENOERR ) 
			{
					DEBUG1("CD read error\n");
					continue;
			}
			
			err = cyg_io_bread(blk_devB, write_buffer, &tmp_length, lba);

			if( err != ENOERR ) 
			{
					DEBUG1("CD read error\n");
					continue;
			}

			if(!CompareData( read_buffer, write_buffer, tmp_length ))
			{
				DEBUG1("STRESS ERROR: CD integrity failure\n");
				if(bFailfast)
					goto fail;
			}
		}
#endif

	}

	DEBUG3("stress test pass\n");

	ret = TEST_OK_PASS;
	goto pass;
fail:

	ret = TEST_ERR_FAIL;
pass:
	// free allocations, stop thread
	delete[] read_buffer;
	delete[] write_buffer;


	bRunning = false;


	Cyg_Thread* pThread = (Cyg_Thread*)thread[0];

	while(pThread->get_state() == Cyg_Thread::RUNNING)
	{
		// waiting for thread to die
		DEBUG3("waiting for memtest to finish\n");
		cyg_thread_delay(100);
	}


	DEBUG3("stress done\n");

	return ret;
	

}                                     


bool CompareData(char * read_buffer, char * write_buffer, cyg_uint32 length)
{
    cyg_uint32 i,errcount = 0;

	for (i=0; i < length; i++)
    {
        if (write_buffer[i] != read_buffer[i])
		{
			DEBUG1("STRESS ERROR: MISCOMP @[%d] W%04X R=%04X\n", i, write_buffer[i], read_buffer[i]);

			if(errcount > 8)
				return false;

			errcount++;
		}

    }

	if(errcount > 0)
		return false;
	
	return true;
}

Cyg_ErrNo DoPacket(cyg_io_handle_t handle, unsigned char *command, unsigned char *data, int iDataLength,int dir);


int CDARead(cyg_io_handle_t handle, long lba, long sectors, void* buffer ) 
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
		DEBUG1("warning - cd not ready\n");
                
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

Cyg_ErrNo DoPacket(cyg_io_handle_t m_hCD, unsigned char *command, unsigned char *data, int iDataLength,int dir)
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
