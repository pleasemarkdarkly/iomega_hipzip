//
// ata.cpp
//
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//
// ATA stress routines for EMI testing

#include "ata.h"

#include <cyg/infra/diag.h>
#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>
#include <stdio.h>
#include <stdlib.h>

#include <devs/storage/ata/atadrv.h>

// thread data etc
#define NTHREADS       1
#define STACKSIZE   8192*4

static cyg_handle_t threadh[NTHREADS];
static cyg_thread   thread[ NTHREADS];
static char         tstack[NTHREADS][STACKSIZE];

// loser sync
static bool g_bAudioCD;
static bool g_bRunning;
volatile int iEMICDType;
cyg_cond_t atawake,atadone,ataui;
cyg_mutex_t atawakem,atadonem,atauim;

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
		diag_printf("warning - cd not ready\n");
                
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

void InitATA()
{
	diag_printf("+InitATA\n");

	g_bAudioCD = false;
	g_bRunning = false;

	// init the conditions
	cyg_mutex_init(&atawakem);
	cyg_mutex_lock(&atawakem);
	cyg_cond_init(&atawake, &atawakem);
	cyg_mutex_unlock(&atawakem);
	

	cyg_mutex_init(&atauim);
	cyg_mutex_lock(&atauim);
	cyg_cond_init(&ataui, &atauim);
	cyg_mutex_unlock(&atauim);


	cyg_mutex_init(&atadonem);
	cyg_mutex_lock(&atadonem);
	cyg_cond_init(&atadone, &atadonem);
	cyg_mutex_unlock(&atadonem);

	// create the thread
	cyg_thread_create( 8, ata_thread, 0, "ata thread",
                       (void*)tstack[0], STACKSIZE, &threadh[0], &thread[0]);
    cyg_thread_resume( threadh[0] );

	diag_printf("-InitATA\n");

}

void StartAudioATA()
{
	if(g_bRunning == true)
	{
		StopATA();
	}

	
	g_bRunning = true;
	g_bAudioCD = true;

//	cyg_mutex_lock(&atawakem);
	cyg_cond_broadcast(&atawake);
//	cyg_mutex_unlock(&atawakem);

}

void StartDataATA()
{

	if(g_bRunning == true)
	{
		StopATA();
	}

	g_bRunning = true;
	g_bAudioCD = false;
//	cyg_mutex_lock(&atawakem);
	cyg_cond_broadcast(&atawake);
//	cyg_mutex_unlock(&atawakem);


}

void StopATA()
{

	if(g_bRunning)
	{
		g_bRunning = false;
		cyg_mutex_lock(&atadonem);
		cyg_cond_wait(&atadone);
		cyg_mutex_unlock(&atadonem);
	}

}

void ATAUIWait()
{
	cyg_mutex_lock(&atauim);
	cyg_cond_wait(&ataui);
	cyg_mutex_unlock(&atauim);
}


void ATAUIAlive()
{

	cyg_mutex_lock(&atauim);
	cyg_cond_broadcast(&ataui);
	cyg_mutex_unlock(&atauim);

}

void ata_thread(cyg_uint32 ignored)
{


	bool bFailFast = false;
	int cda_lba;
    int hda_lba;
  
    cyg_uint16 length;
    cyg_uint32 lba;
    cyg_uint32 len;
    Cyg_ErrNo err;
    cyg_io_handle_t blk_devA, blk_devB;

    cyg_uint32 cd_sec,cd_start,hda_length,cd_max,cd_len,hd_sec,hda_loc,cd_loc,hd_start;
	
	/* Initialize data buffers */
    char* read_buffer = new char[128*1024];  
      
    memset(read_buffer,0,128*1024);   

    const char * dev_name = "/dev/hda/";
    //  DEBUG(COPYC,DBGLEV_INFO,"looking up %s\n", dev_name);
    if (cyg_io_lookup(dev_name, &blk_devA) != ENOERR) {
        diag_printf("STRESS ERROR: Could not get handle to dev");
		goto fail;
    }

    diag_printf("got handle to %s\n", dev_name);
    
    dev_name = "/dev/cda/";
    diag_printf("looking up %s\n", dev_name);
    if( cyg_io_lookup(dev_name, &blk_devB) != ENOERR) {
        diag_printf("STRESS ERROR: Could not get handle to dev");
		goto fail;
    }
    diag_printf("got handle to %s\n", dev_name);
    
    len = sizeof(len);

#if 0
    if (cyg_io_set_config(blk_devA, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
        diag_printf("STRESS ERROR: Could not power up a device");
		goto fail;

    }

    if (cyg_io_set_config(blk_devB, IO_BLK_SET_CONFIG_POWER_UP, 0, &len) != ENOERR) {
        diag_printf("STRESS ERROR: Could not power up b device");
		goto fail;

    }
#endif
    
 
	hd_sec = 512;
	hd_start = 0;


	while(1)
	{

		cyg_mutex_lock(&atawakem);

		while(!g_bRunning)
		{
			cyg_cond_wait(&atawake);
		}

		cyg_mutex_unlock(&atawakem);

		while(g_bRunning)
		{
			if(g_bAudioCD)
			{
				cd_sec = 2352;
				cd_start = 1000;
				hda_length = 128*512;
				cd_max = 2000;
				cd_len = 28;

			}
			else
			{

				cd_sec = 512;
				cd_start = 1000;		
				cd_len = 128;
				hda_length = cd_len*512;
				cd_max = 8000;
			}


			// read ~64k in each case
			hda_loc = hd_start;

			if(g_bAudioCD)
			{
				
				
				for(cd_loc = cd_start; cd_loc < cd_max && g_bRunning; cd_loc++)
				{
					while(-EIO == CDARead( blk_devB, cd_loc, cd_len, read_buffer ));

					err = cyg_io_bwrite(blk_devA, read_buffer, &hda_length, hda_loc);
					hda_loc += (hda_length / 512);
					ATAUIAlive();
					
				}

			}
			else
			{	
				
				cyg_uint32 tmp_length;

				for(cd_loc = cd_start; cd_loc < cd_max && g_bRunning; cd_loc+=cd_len)
				{			
					err = cyg_io_bread(blk_devB, read_buffer, &hda_length, cd_loc);
					err = cyg_io_bwrite(blk_devA, read_buffer, &hda_length, hda_loc);
					hda_loc += (hda_length / 512);
					ATAUIAlive();
				}

			}


		}
		
		cyg_mutex_lock(&atadonem);
		cyg_cond_broadcast(&atadone);
		cyg_mutex_unlock(&atadonem);

	}


fail:
	// free allocations, stop thread
	delete[] read_buffer;

}
	
