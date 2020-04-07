// ataproto.c: protocol layer for ATA and ATAPI driver
// danc@iobjects.com 07/04/01
// (c) Interactive Objects

// portions of this driver have received modification according to the following resource:
//  http://www.stanford.edu/~csapuntz/blackmagic.html

#include <cyg/hal/hal_diag.h>   // HAL_DELAY_US
#include <util/debug/debug.h>

#include <devs/storage/ata/atadrv.h>
#include "atabus.h"
#include "busctrl.h"

DEBUG_USE_MODULE(ATA);

// HAL_DELAY_US

#ifndef HAL_DELAY_US
#define HAL_DELAY_US _Wait
static void 
_Wait(unsigned int Delay)
{
    volatile unsigned int Count = Delay * 0xb;
    
    while(Count--)
        ;
}
#endif

//
// ATA
//

// wrappers for polling mode (so cyg_* can be unlinked)
// significantly reduces #ifndef madness
// temancl 10/18/01

void BusPeek(BusSpace_T * Bus,int *z)
{
#ifdef NOKERNEL
    *z = 0;
#else
    cyg_semaphore_peek(&Bus->IRQSem, (void*) z);
#endif
}

unsigned long BusCurrentTime()
{

    // hal_clock_read comes from edb7xxx_misc.c in the kernel
#ifdef NOKERNEL
    unsigned long res;
    hal_clock_read( &res );
    // scale this, since the system clock is a 512khz yatch
    return res / 5120;
#else
    return cyg_current_time();
#endif


}

void BusMask(BusSpace_T * Bus)
{
#ifndef NOKERNEL
    cyg_drv_interrupt_mask(Bus->IRQ);
#endif
}

void BusUnmask(BusSpace_T * Bus)
{

#ifndef NOKERNEL
    cyg_drv_interrupt_unmask(Bus->IRQ);
#endif
}

int BusWait(BusSpace_T * Bus, unsigned long variable)
{

#ifdef NOKERNEL
    return 1; // always succeed
#else
    return cyg_semaphore_timed_wait(&Bus->IRQSem,cyg_current_time() + variable);
#endif

}

void BusLock(BusSpace_T * Bus)
{

#ifndef NOKERNEL
    cyg_mutex_lock(&Bus->Mutex);
#endif
}

void BusUnlock(BusSpace_T * Bus)
{

#ifndef NOKERNEL
    cyg_mutex_unlock(&Bus->Mutex);
#endif

}
#define TIMEOUT_RESET 3100000

void
ATASoftReset(BusSpace_T * Bus)
{
    // Each retry can take up to 60s
    unsigned int RetryCount = 3;
	unsigned int TryCount = 0;
    unsigned int Timeout;
    unsigned char Status;

    // Grab the signature, we might use it someday
    unsigned char DeviceSignature[5];

    DEBUG(ATA, DBGLEV_WARNING, "ata reset on ata bus\n");

    while(1)
	{
         
        /* Wait for busy to clear (31s) */

        // Wait on bus master first
        for (Timeout = 0; Timeout < TIMEOUT_RESET; ++Timeout) {
            // TODO: is it safe to assume a master is on the bus?
            BusWrite8(Bus, Bus->DeviceHeadReg, 0);
            HAL_DELAY_US(100);
            Status = BusRead8(Bus, Bus->StatusReg);

            if ( (Status & STATUS_BSY) == 0) {
                break;
            }
            HAL_DELAY_US(100);
        }
    

		DEBUG(ATA, DBGLEV_WARNING, "waited %d on ATA master\n",Timeout);

        if( Timeout == TIMEOUT_RESET ) {
            DEBUG( ATA, DBGLEV_ERROR, "Timed out waiting for ata device 0 to leave BUSY state\n");
        }
        
        if( (Timeout < TIMEOUT_RESET) && (Bus->DeviceCount == 2) ) {
            // Wait for slave to clear busy
            // Since we already waited a certain period above, resetting the Timeout
            //  here is probably extraneous
            for( Timeout = 0; Timeout < TIMEOUT_RESET; ++Timeout ) {
                BusWrite8(Bus, Bus->DeviceHeadReg, 0x01 << 4 );
                HAL_DELAY_US(100);
                Status = BusRead8(Bus, Bus->StatusReg);
                
                if( (Status & STATUS_BSY) == 0 ) {
                    break;
                }
                HAL_DELAY_US(100);
            }

			DEBUG(ATA, DBGLEV_WARNING, "waited %d on ATA slave\n",Timeout);


            if( Timeout == TIMEOUT_RESET ) {
                DEBUG( ATA, DBGLEV_ERROR, "Timed out waiting for ata device 1 to leave BUSY state\n");
            }
        }

		// must wait once before sending reset command
		if(Timeout != TIMEOUT_RESET && TryCount > 0 || ((RetryCount--) == 0))
			break;
	
	     // select device
        BusWrite8(Bus, Bus->DeviceHeadReg, 0);
        HAL_DELAY_US(100);
        // state HSR0, set SRST on device for 5us
        BusWrite8(Bus, Bus->ControlReg, (CONTROL_SRST | CONTROL_POLLED));
        HAL_DELAY_US(100);
        // state HSR1, clear SRST from device for 2ms
        BusWrite8(Bus, Bus->ControlReg, CONTROL_POLLED);
        HAL_DELAY_US(4000);
        // state HSR2, wait until busy has cleared

		TryCount++;
  

    } 
    // HSR2->HIO, read error register and signature, enable IRQ from device
    BusRead8(Bus, Bus->ErrorReg);

    DeviceSignature[0] = BusRead8(Bus, Bus->SectorCountReg);
    DeviceSignature[1] = BusRead8(Bus, Bus->SectorNumberReg);
    DeviceSignature[2] = BusRead8(Bus, Bus->CylinderLowReg);
    DeviceSignature[3] = BusRead8(Bus, Bus->CylinderHighReg);
    DeviceSignature[4] = BusRead8(Bus, Bus->DeviceHeadReg);
    
    BusWrite8(Bus, Bus->ControlReg, CONTROL_IRQ);
    HAL_DELAY_US(100);
    // state HIO

    // set reset flag on bus
    Bus->IsReset = 1;
}

int
ATAAttach(ATADeviceInfo_T * DeviceInfo)
{
	int i;
	char* chBuff;
    ATACommand_T Cmd;
    unsigned short IdentifyData[256];
    int Status;

    DBTR(ATA);

    BusMask(DeviceInfo->Bus);
    DeviceInfo->Bus->Reset( DeviceInfo->Bus );
    BusUnmask(DeviceInfo->Bus);
   
    memset(&Cmd, 0, sizeof(Cmd));    
    Cmd.Command = ATA_IDENTIFY;
    Cmd.StatusBefore = STATUS_DRDY;
    Cmd.StatusAfter = STATUS_DRQ;
    Cmd.Timeout = ATA_DELAY;
    Cmd.Flags |= (ATA_CMD_READ);
    Cmd.Data = IdentifyData;
    Cmd.ByteCount = sizeof(IdentifyData);
    
    Status = ATAExecuteCommand(DeviceInfo, &Cmd);
    if (Status == COMPLETE) {
        /* Set multiple sector */
        DeviceInfo->Multi = IdentifyData[47] & 0xFF;
        memset(&Cmd, 0, sizeof(Cmd));    
        Cmd.Command = ATA_SETMULTI;
        Cmd.SectorCount = DeviceInfo->Multi;
        Cmd.StatusBefore = STATUS_DRDY;
        Cmd.StatusAfter = STATUS_DRDY;
        Cmd.Timeout = ATA_DELAY;
        Status = ATAExecuteCommand(DeviceInfo, &Cmd);
        /* TODO Status is ignored right now, since queueing is not supported */
        if (Cmd.Error & ERROR_ABRT) {
            DeviceInfo->Multi = 1;
        }

        /* Get geometry information */
        DeviceInfo->Cylinders = IdentifyData[1];
        DeviceInfo->Heads = IdentifyData[3];
        DeviceInfo->SectorsPerTrack = IdentifyData[6];
        DeviceInfo->TotalSectors = (IdentifyData[61] << 16) | IdentifyData[60];
        DEBUG( ATA, DBGLEV_INFO, "C/H/S: %d/%d/%d Total Sectors %d\n",
               DeviceInfo->Cylinders, DeviceInfo->Heads, DeviceInfo->SectorsPerTrack,
               DeviceInfo->TotalSectors );
        DEBUG( ATA, DBGLEV_INFO, "Multiple mode sector limit: %d\n",
               DeviceInfo->Multi );
	
        /* Byte swap these strings */

		
		chBuff = (char*)&IdentifyData[10];
		for(i = 0; i < 20; i+=2)
		{
			DeviceInfo->SerialNumber[i] = chBuff[i + 1];
			DeviceInfo->SerialNumber[i+1] = chBuff[i];
		}

		DeviceInfo->SerialNumber[20] = '\0';

		chBuff = (char*)&IdentifyData[27];

		for(i = 0; i < 40; i+=2)
		{
			DeviceInfo->ModelNumber[i] = chBuff[i + 1];
			DeviceInfo->ModelNumber[i+1] = chBuff[i];
		}

		DeviceInfo->ModelNumber[40] = '\0';
		// old, non-byte swap copies
        // memcpy(DeviceInfo->SerialNumber, &IdentifyData[10], 20);
        // memcpy(DeviceInfo->ModelNumber, &IdentifyData[27], 40);

        DEBUG( ATA, DBGLEV_INFO, "SN: %s MN: %s\n", DeviceInfo->SerialNumber, DeviceInfo->ModelNumber );
        DeviceInfo->Bus->IsReset = true;


#if (DEBUG_LEVEL != 0)
		diag_printf("AAM ID Word = %x\n",IdentifyData[94]);
#endif


    }
    else {
        /* TODO Identify command failed */
        DEBUG(ATA,DBGLEV_ERROR, "identify drive failed\n" );
        DeviceInfo->Bus->IsReset = false;
    }
    /* TODO Return real status */
    return Status;
}

int
ATAExecuteCommand(ATADeviceInfo_T * DeviceInfo, ATACommand_T * Cmd)
{
    BusSpace_T * Bus;
    Transfer_T Transfer;
    int Status;

    DBTR(ATA);
    
    Bus = DeviceInfo->Bus;

    memset(&Transfer, 0, sizeof(Transfer));
    Transfer.Drive = DeviceInfo->Drive;
    Transfer.Cmd = Cmd;
    
    ATACommandStart(Bus, &Transfer);
    /* Command queueing is not supported yet, so this block is ignored. */
    if( (Cmd->Flags & ATA_CMD_ERROR) ) {
        Status = ERRORS;
    }
    else if ((Cmd->Flags & ATA_CMD_DONE) == 0) {
        if (Cmd->Flags & ATA_CMD_WAIT) {
            /* TODO Sleep until command is done */
            Status = COMPLETE;
        }
        else {
            Status = QUEUED;
        }
    }
    else { // (Cmd->Flags & ATA_CMD_DONE) == ATA_CMD_DONE
        Status = COMPLETE;
    }
    return Status;
}

void
ATACommandStart(BusSpace_T * Bus, Transfer_T * Transfer)
{
    int Drive;
    ATACommand_T * Cmd;

    DBTR(ATA);
    
    /* Lock the bus */
    BusLock(Bus);

    Drive = Transfer->Drive;
    Cmd = (ATACommand_T *)Transfer->Cmd;
    
    BusWrite8(Bus, Bus->DeviceHeadReg, (Drive << 4));
    if (ATAWait(Bus, Cmd->StatusBefore | STATUS_DRQ, Cmd->StatusBefore, Cmd->Timeout) != 0) {
        DEBUG( ATA, DBGLEV_ERROR, "timed out waiting for device ready\n");
        Cmd->Flags |= ATA_CMD_TIMEOUT;
        ATACommandDone(Bus, Transfer);
        return;
    }
    ATACommand(Bus, Drive, Cmd->Command, Cmd->Cylinder, Cmd->Head, Cmd->SectorNumber,
               Cmd->SectorCount, Cmd->Features);
    if ((Cmd->Flags & ATA_CMD_POLL) == 0) {
        if (!BusWait(Bus,Cmd->Timeout)) {
            /* Can either assume the device is hosed (we actually did timeout), or
               we missed the interrupt.  I will assume that the device is hosed. */
            DEBUG(ATA, DBGLEV_ERROR, "ATACommand timed out\n");
            Cmd->Flags |= ATA_CMD_TIMEOUT;
            ATACommandDone(Bus, Transfer);
            return;
        }
    }
    ATACommandInterrupt(Bus, Transfer);
}

void
ATACommandInterrupt(BusSpace_T * Bus, Transfer_T * Transfer)
{
    ATACommand_T * Cmd;
    int ByteCount;
    char * Data;
    int WaitStatus;
    
    Cmd = (ATACommand_T *)Transfer->Cmd;
    ByteCount = Cmd->ByteCount;
    Data = Cmd->Data;

    WaitStatus = ATAWait(Bus, Cmd->StatusAfter, Cmd->StatusAfter, Cmd->Timeout);
    BusUnmask(Bus);
    if (WaitStatus != 0) {
        DEBUG( ATA, DBGLEV_ERROR, "timed out waiting for device response");
        Cmd->Flags |= ATA_CMD_TIMEOUT;
        ATACommandDone(Bus, Transfer);
        return;
    }
    if (Cmd->Flags & ATA_CMD_READ) {
        if (ByteCount > 0) {
            Bus->Read16Multiple(Bus, Bus->DataReg, (unsigned short *)Data, (ByteCount >> 1));
        }
    }
    else if (Cmd->Flags & ATA_CMD_WRITE) {
        if (ByteCount > 0) {
            Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)Data, (ByteCount >> 1));
        }
    }
#ifndef NOKERNEL
    /* This is bullshit of course, but it seems to happen with the crappy 52x drives.
       Basically, it sends an IRQ after ATAIdentify, even though it shouldn't.  This could
       possible be a bigger problem with this whole command path, I don't know. */
    {
        if (cyg_semaphore_trywait(&Bus->IRQSem)) {
        
			// int z;
			//BusPeek(Bus,&z);
			//if( z ) {
			// just try acknowledging interrupt instead		
			// }

			BusRead8(Bus, Bus->StatusReg);        
			BusUnmask(Bus);
			DEBUG(ATA, DBGLEV_ERROR, "Waited on IRQ that shouldn't have happened\n");
		}
    }
    
#endif
    ATACommandDone(Bus, Transfer);
}

void
ATACommandDone(BusSpace_T * Bus, Transfer_T * Transfer) 
{
    ATACommand_T * Cmd;

    Cmd = (ATACommand_T *)Transfer->Cmd;

    Cmd->Flags |= ATA_CMD_DONE;

    if (Bus->Status & STATUS_ERR) {
        Cmd->Flags &= ~(ATA_CMD_TIMEOUT);
        Cmd->Flags |= ATA_CMD_ERROR;
        Cmd->Error = Bus->Error;
    }
    else if ((Cmd->Flags & ATA_CMD_READ_REG) != 0) {
        Cmd->DeviceHead = BusRead8(Bus, Bus->DeviceHeadReg);
        Cmd->Cylinder = BusRead8(Bus, Bus->CylinderHighReg) << 8;
        Cmd->Cylinder |= BusRead8(Bus, Bus->CylinderLowReg);
        Cmd->SectorNumber = BusRead8(Bus, Bus->SectorNumberReg);
        Cmd->SectorCount = BusRead8(Bus, Bus->SectorCountReg);
        Cmd->Error = BusRead8(Bus, Bus->ErrorReg);
        Cmd->Features = BusRead8(Bus, Bus->FeaturesReg);
    }

    /* Unlock the bus */
    BusUnlock(Bus);

}


void
ATACommand(BusSpace_T * Bus, unsigned char Drive, unsigned char Command,
           unsigned short Cylinder, unsigned char Head, unsigned char SectorNumber,
           unsigned char SectorCount, unsigned char Features)
{

    DBTR(ATA);
  
    BusWrite8(Bus, Bus->DeviceHeadReg, ((Drive << 4) | Head));

    BusWrite8(Bus, Bus->FeaturesReg, Features);
    BusWrite8(Bus, Bus->CylinderLowReg, Cylinder);
    BusWrite8(Bus, Bus->CylinderHighReg, (Cylinder >> 8));
    BusWrite8(Bus, Bus->SectorNumberReg, SectorNumber);
    BusWrite8(Bus, Bus->SectorCountReg, SectorCount);

    BusWrite8(Bus, Bus->CommandReg, Command);
}

int
ATAWait(BusSpace_T * Bus, int Mask, int Bits, int Timeout) 
{
    unsigned char Status;
    int AbsTimeout;

    DEBUG( ATA, DBGLEV_INFO, "Mask = %04x, Bits = %04x\n", Mask, Bits );
    
    AbsTimeout = BusCurrentTime() + Timeout;

    // clear this here so we dont propogate false values
    Bus->Error = 0;
    
    for (;;) {
        Bus->Status = Status = BusRead8(Bus, Bus->StatusReg);
        if ((Status & STATUS_BSY) == 0 && (Status & Mask) == Bits) {
            break;
        }
        if( (Status & STATUS_ERR) ) {
            break;
        }
        if (BusCurrentTime() > AbsTimeout) {
            return -1;
        }
    }
    if (Status & STATUS_ERR) {
        Bus->Error = BusRead8(Bus, Bus->ErrorReg);
    }
    return 0;
}

int
ATABlockIO(ATADeviceInfo_T * DeviceInfo, ATABlockIO_T * Cmd)
{
    BusSpace_T * Bus;
    Transfer_T Transfer;
    static int Retries;

    DBTR(ATA);
    
    Bus = DeviceInfo->Bus;

    Cmd->Partition = DeviceInfo->Partition;
    Cmd->Multi = DeviceInfo->Multi;
    
    Retries = 0;
  Again:
    memset(&Transfer, 0, sizeof(Transfer));
    if (Cmd->Flags & ATA_BIO_POLL) {
        Transfer.Flags |= XFER_POLL;
    }
    Transfer.Drive = DeviceInfo->Drive;
    Transfer.Cmd = Cmd;
    Transfer.Data = Cmd->Data;
    Transfer.ByteCount = Cmd->ByteCount;
    Transfer.Start = ATABlockIOStart;
    Transfer.Interrupt = ATABlockIOInterrupt;
    Transfer.KillTransfer = ATABlockIODone;

    ATABlockIOStart(Bus, &Transfer);
    /* TODO The reset/retry needs to be tested */
    if ((Retries < ATA_MAX_RETRIES) && (Cmd->Flags & ATA_BIO_RESET)) {
        Bus->Reset( Bus );
        ATAAttach(DeviceInfo);
        Cmd->Flags &= ~(ATA_BIO_DONE | ATA_BIO_RESET);
        ++Retries;
        goto Again;
    }
    return (Cmd->Flags & ATA_BIO_DONE) ? COMPLETE : QUEUED;
}

void
ATABlockIOStart(BusSpace_T * Bus, Transfer_T * Transfer)
{
    ATABlockIO_T * Cmd;
    unsigned int LoopCount = 0;	
    unsigned int NumBlks;
	unsigned int SectorCount;
    unsigned short Cylinder = 0; /* Make the compiler happy */
    unsigned char Sector = 0;
    unsigned char Head = 0;
    unsigned char Command;
	unsigned int BlkRemain;

 
    
    Cmd = (ATABlockIO_T *)Transfer->Cmd;
   
	/* set up totals */
    if (Cmd->Flags & ATA_BIO_SINGLE) 
	{
        NumBlks = 1;
    }
    else 
	{
		// get total number of blocks to transfer
        NumBlks = Transfer->ByteCount / Cmd->Partition->SectorSize;
    }

	Cmd->BlkDone = 0;
	/* bulk transfer data, work around 256 sector MULTIPLE limit */
	while(NumBlks > 0)
	{

		/* Lock the bus to issue commands*/	    
		BusLock(Bus);

		/* appropriate command selection */
		if (NumBlks > 1 && (Cmd->Flags & ATA_BIO_SINGLE) == 0) 
		{
            Command = (Cmd->Flags & ATA_BIO_READ) ? ATA_READMULTI : ATA_WRITEMULTI;
        }
        else 
		{
            Command = (Cmd->Flags & ATA_BIO_READ) ? ATA_READ : ATA_WRITE;
        }

		if (Cmd->Flags & ATA_BIO_LBA) 
		{
			Sector = (Cmd->BlkNum >> 0) & 0xff;
			Cylinder = (Cmd->BlkNum >> 8) & 0xffff;
			Head = (Cmd->BlkNum >> 24) & 0x0f;
			Head |= DEVICE_LBA;
		}
		else 
		{ 
			/* we don't support CHS because it sucks */
			DEBUG(ATA, DBGLEV_ERROR, "no chs support\n");
		}

		
		if(NumBlks > 255)
			SectorCount = 256;		
		else		
			SectorCount = NumBlks;


	
        BusWrite8(Bus, Bus->DeviceHeadReg, (Transfer->Drive << 4));

        if (WaitForReady(Bus, ATA_DELAY) < 0) 
		{
            /* TODO Timeout */
            DEBUG(ATA, DBGLEV_ERROR, "timeout while waiting for device ready\n");
        }

		// diag_printf("bio: lba %x, sec %d\n",Cmd->BlkNum,SectorCount);
#if 0
		if(NumBlks != (Transfer->ByteCount / Cmd->Partition->SectorSize))
		{	
			diag_printf("out of sync, %d != %d\n",Transfer->ByteCount,Cmd->Partition->SectorSize);
		}
#endif
    
		// if sector count is 256, set it to 0 for READ/WRITE MULTIPLE
        ATACommand(Bus, Transfer->Drive, Command, Cylinder, Head,
			Sector, (SectorCount == 256) ? 0 : SectorCount, 0);

		/* use to track command completion */
		BlkRemain = SectorCount;

		/* transfer all data for command */
		// diag_printf("bio: blkpos %d blkdone %d\n",BlkPos, Cmd->BlkDone);
		while(BlkRemain > 0)
		{		
			/* determine block count between interrupts */
			Cmd->NumBlks = (BlkRemain < Cmd->Multi) ? BlkRemain : Cmd->Multi;
			Cmd->NumBytes = Cmd->NumBlks * Cmd->Partition->SectorSize;      
			
			// if we are writing, wait for a DRQ
			if ((Cmd->Flags & ATA_BIO_READ) == 0) 
			{
				if (WaitForDRQ(Bus, ATA_DELAY) != 0) 
				{
					Cmd->Error = ATA_BIO_TIMEOUT;
					ATABlockIODone(Bus, Transfer);
					return;
				}

				if (ATAError(Bus, Cmd) < 0) 
				{
					ATABlockIODone(Bus, Transfer);
					return;
				}

				DBTR(ATA);

				Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
									 Cmd->NumBytes >> 1);
			}

			// wait for next data interrupt
			if ((Cmd->Flags & ATA_BIO_POLL) == 0) 
			{
				DBTR(ATA);

				if (!BusWait(Bus,ATA_DELAY)) 
				{
					/* Can either assume the device is hosed (we actually did timeout), or
					   we missed the interrupt.  I will assume that the device is hosed. */
					Cmd->Error = ATA_BIO_TIMEOUT;
					ATABlockIODone(Bus, Transfer);
					return ;
				}
			}

			ATABlockIOInterrupt(Bus, Transfer);

			// adjust remaining blocks in current command context
			BlkRemain -= Cmd->NumBlks;
		}	
		
		/* adjust remaining sectors */
		NumBlks -= SectorCount;
	   
		/* bit of a hack to encapsulate multiple commands in one */
		if (Transfer->ByteCount != 0) 
		{	
			ATABlockIODone(Bus, Transfer);		
		}

	}

}

void
ATABlockIOInterrupt(BusSpace_T * Bus, Transfer_T * Transfer) 
{
    ATABlockIO_T * Cmd;
    int DriveError;
    int WaitStatus;

    DBTR(ATA);
    
    Cmd = (ATABlockIO_T *)Transfer->Cmd;

    WaitStatus = WaitForUnbusy(Bus, ATA_DELAY);
    BusUnmask(Bus);
    if (WaitStatus < 0) {
        Cmd->Error = ATA_BIO_TIMEOUT;
        ATABlockIODone(Bus, Transfer);
        return;
    }

    DriveError = ATAError(Bus, Cmd);

    if (DriveError < 0) {
        ATABlockIODone(Bus, Transfer);
        return;
    }
    
    if ((Cmd->Flags & ATA_BIO_READ) != 0) {
        if ((Bus->Status & STATUS_DRQ) != STATUS_DRQ) {
            Cmd->Error = ATA_BIO_TIMEOUT;
            ATABlockIODone(Bus, Transfer);
            return;
        }
#if 0 // overrun detection
		if((Transfer->Skip + Cmd->NumBytes) > Cmd->ByteCount)
		{
			diag_printf("transfer overrun of %d bytes\n",(Transfer->Skip + Cmd->NumBytes) - Cmd->ByteCount);
			diag_printf("cbc = %d, cnb = %d, tsk = %d\n",Cmd->ByteCount,Cmd->NumBytes,Transfer->Skip);
			diag_printf("tbc = %d, cbn = %d, cbd = %d\n",Transfer->ByteCount,Cmd->BlkNum, Cmd->BlkDone);
		}
#endif

        Bus->Read16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
                            Cmd->NumBytes >> 1);
    }

    Cmd->BlkNum += Cmd->NumBlks;
    Cmd->BlkDone += Cmd->NumBlks;
    Transfer->Skip += Cmd->NumBytes;
    Transfer->ByteCount -= Cmd->NumBytes;

    if (Transfer->ByteCount == 0) {
        Cmd->Error = ATA_BIO_NO_ERROR;
        ATABlockIODone(Bus, Transfer);
    }
}

void
ATABlockIODone(BusSpace_T * Bus, Transfer_T * Transfer) 
{
    ATABlockIO_T * Cmd;

    DBTR(ATA);
    
    /* Unlock the bus */
    BusUnlock(Bus);
    
    Cmd = Transfer->Cmd;
    
    Cmd->ByteCount = Transfer->ByteCount;
    Cmd->Flags |= ATA_BIO_DONE;
    if ((Cmd->Flags & ATA_BIO_POLL) == 0) {
        switch (Cmd->Error) {
            case ATA_BIO_TIMEOUT:
            case ATA_BIO_DF:
            case ATA_BIO_ERROR:
            {
                DEBUG( ATA, DBGLEV_ERROR, "ata error %d\n", Cmd->Error );
                Cmd->Flags |= ATA_BIO_RESET;
                break;
            }
        }
    }
}

int
ATAError(BusSpace_T * Bus, ATABlockIO_T * Cmd) 
{
    Cmd->Error = 0;
    if (Bus->Status & STATUS_BSY) {
        Cmd->Error = ATA_BIO_TIMEOUT;
        return -1;
    }

    if (Bus->Status & STATUS_DWF) {
        Cmd->Error = ATA_BIO_DF;
        return -1;
    }

    if (Bus->Status & STATUS_ERR) {
        Cmd->Error = ATA_BIO_ERROR;
        Cmd->ErrorReg = Bus->Error;
        if (Cmd->ErrorReg & (ERROR_BBK | ERROR_UNC | ERROR_IDNF |
                             ERROR_ABRT | ERROR_TK0NF | ERROR_AMNF)) {
            return -1;
        }
        return 0;
    }

    if (Bus->Status & STATUS_CORR) {
        Cmd->Flags |= ATA_BIO_CORRECTED;
    }
    return 0;
}

//
// ATAPI
//

int
ATAPIAttach(ATADeviceInfo_T * DeviceInfo)
{
    ATACommand_T Cmd;
    unsigned short IdentifyData[256];
    int Status;

	// redundant, probably - test this
    BusMask(DeviceInfo->Bus);
    DeviceInfo->Bus->Reset( DeviceInfo->Bus );
    BusUnmask(DeviceInfo->Bus);

    memset(&Cmd, 0, sizeof(Cmd));    
    Cmd.Command = ATAPI_IDENTIFY;
    Cmd.StatusBefore = 0;
    Cmd.StatusAfter = STATUS_DRQ;
    Cmd.Timeout = 1000; /* 10s */
    Cmd.Flags |= (ATA_CMD_READ);
    Cmd.Data = IdentifyData;
    Cmd.ByteCount = sizeof(IdentifyData);
    
    Status = ATAExecuteCommand(DeviceInfo, &Cmd);
    if (Status == COMPLETE) {
        DeviceInfo->Config = IdentifyData[0];
    }
    else {
        /* TODO Queuing not supported yet, so no worries */
        DEBUG(ATA,DBGLEV_ERROR,"queuing not supported yet\n" );
    }



    return Status;
}

int
ATAPIExecuteCommand(ATADeviceInfo_T * DeviceInfo, ATAPICommand_T * Cmd)
{
    BusSpace_T * Bus;
    Transfer_T Transfer;

    Bus = DeviceInfo->Bus;

	
    memset(&Transfer, 0, sizeof(Transfer));

    if (Cmd->Flags & ATAPI_POLL) {
	    Transfer.Flags |= XFER_POLL;
    }

    Transfer.Drive = DeviceInfo->Drive;
    Transfer.Flags |= XFER_ATAPI;
    Transfer.Cmd = Cmd;
    Transfer.Data = Cmd->Data;
    Transfer.ByteCount = Cmd->DataLength;
    Transfer.Start = ATAPICommandStart;
    Transfer.Interrupt = ATAPICommandInterrupt;
    Transfer.KillTransfer = ATAPICommandDone;
    
    ATAPICommandStart(Bus, &Transfer);
	
  
    return (Cmd->Flags & ATAPI_DONE) ? COMPLETE : QUEUED;
}

void
ATAPICommandStart(BusSpace_T * Bus, Transfer_T * Transfer) 
{
    ATAPICommand_T * Cmd;
    ATADeviceInfo_T * DeviceInfo;

	
    /* Lock the bus */
    BusLock(Bus);
    
 


    Cmd = (ATAPICommand_T *)Transfer->Cmd;
    DeviceInfo = &Bus->Device[Transfer->Drive];
    

    if (Cmd->Flags & ATAPI_POLL) {
		
		BusMask(DeviceInfo->Bus);
    }


    BusWrite8(Bus, Bus->DeviceHeadReg, (Transfer->Drive << 4));
    if (WaitForUnbusy(Bus, ATAPI_DELAY) < 0) {
		
        DEBUG(ATA,DBGLEV_ERROR, "ATAPI command timeout\n");
        Cmd->Error = ATAPI_TIMEOUT;
        ATAPIReset(Bus, Transfer);
        return;
    }

    ATACommand(Bus, Transfer->Drive, ATAPI_PACKET,
               Transfer->ByteCount <= 0xffff ? Transfer->ByteCount : 0xffff,
               0, 0, 0, 0);

    if ((DeviceInfo->Config & ATAPI_CFG_DRQ_MASK) == ATAPI_CFG_IRQ_DRQ) {
		if ((Transfer->Flags & XFER_POLL) == 0) {
			if (!BusWait(Bus,Cmd->Timeout)) {
				/* TODO Timeout IRQ handling */
				DEBUG(ATA,DBGLEV_ERROR, "timeout waiting for irq\n" );
				
			}
		}
    }
    
    while ((Cmd->Flags & ATAPI_DONE) == 0) {
		
        ATAPICommandInterrupt(Bus, Transfer);
    }
}

void
ATAPICommandInterrupt(BusSpace_T * Bus, Transfer_T * Transfer)
{
    ATAPICommand_T * Cmd;
    int Length;
    int IRQReason;
    int Phase;
    SCSICommand_T _ReqSense;
    SCSIRequestSense_T * ReqSense;
    void * SCSICmd;
    int Retries;
    int WaitStatus;
    int i;
    
    Cmd = (ATAPICommand_T *)Transfer->Cmd;
    ReqSense = (SCSIRequestSense_T *)&_ReqSense;
    Retries = 0;
    
	

    if (Transfer->Flags & XFER_TIMEOUT) {
        Cmd->Error = ATAPI_TIMEOUT;
		
        ATAPIReset(Bus, Transfer);
        return;
    } 

    /* Ack interrupt done in wait_for_unbusy */
    BusWrite8(Bus, Bus->DeviceHeadReg, (Transfer->Drive << 4));
    WaitStatus = WaitForUnbusy(Bus, Cmd->Timeout);
	
	if ((Transfer->Flags & XFER_POLL) == 0) {
		// don't unmask bus while polling
		
		BusUnmask(Bus);
	}

    if (WaitStatus != 0) {
        Cmd->Error = ATAPI_TIMEOUT;
		
        ATAPIReset(Bus, Transfer);
        return;
    }

    if ((Transfer->Flags & XFER_SENSE) != 0 &&
        (Bus->Status & STATUS_ERR) != 0 &&
        (Bus->Error & ERROR_ABRT) != 0) {
        ATAPICommandDone(Bus, Transfer);
        return;
    }

  Again:
    Length = BusRead8(Bus, Bus->CylinderLowReg) + 256 * BusRead8(Bus, Bus->CylinderHighReg);
    IRQReason = BusRead8(Bus, Bus->IRQReasonReg);
    Phase = (IRQReason & (IRSN_CMD | IRSN_IN)) | (Bus->Status & STATUS_DRQ);
    
    switch (Phase) {
        case PHASE_CMDOUT:
            if (Transfer->Flags & XFER_SENSE) {
                memset(ReqSense, 0, sizeof(SCSICommand_T));
                ReqSense->Opcode = SCSI_REQUEST_SENSE;
                ReqSense->Length = Transfer->ByteCount;
                SCSICmd = ReqSense;
            }
            else {
                SCSICmd = Cmd->SCSICmd;
            }
            Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)SCSICmd, (Cmd->SCSICmdLength >> 1));
	    
            if ((Transfer->Flags & XFER_POLL) == 0) {
                if (!BusWait(Bus,Cmd->Timeout)) {
                    /* TODO Timeout IRQ handling */
                    DEBUG(ATA, DBGLEV_ERROR, "timeout waiting for irq\n");
                }
            }
            return;

        case PHASE_DATAOUT:
            /* write data */
            if ((Cmd->Flags & ATAPI_DATA_OUT) == 0) {
                Cmd->Error = ATAPI_TIMEOUT;
				
                ATAPIReset(Bus, Transfer);
                return;
            }
            if (Transfer->ByteCount < Length) {
                Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
                                     Transfer->ByteCount >> 1);
                for (i = Transfer->ByteCount; i < Length; i += 2) {
                    Bus->Write16(Bus, Bus->DataReg, 0);
                }
                Transfer->Skip += Transfer->ByteCount;
                Transfer->ByteCount = 0;
            }
            else {
                if (Length > 0) {
                    Bus->Write16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
                                         Length >> 1);
                    Transfer->Skip += Length;
                    Transfer->ByteCount -= Length;
                }
            }
            if ((Transfer->Flags & XFER_POLL) == 0) {
                if (!BusWait(Bus, Cmd->Timeout)) {
                    /* TODO Timeout IRQ handling */
                    DEBUG(ATA,DBGLEV_ERROR, "timeout waiting for irq\n" );
                }
            }
            return;
	    
        case PHASE_DATAIN:
            /* Read data */
            if (((Cmd->Flags & ATAPI_DATA_IN) == 0 &&
                 (Transfer->Flags & XFER_SENSE) == 0)) {
                Cmd->Error = ATAPI_TIMEOUT;
				
                ATAPIReset(Bus, Transfer);
                return;
            }
            if (Transfer->ByteCount < Length) {
                Bus->Read16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
                                    Transfer->ByteCount >> 1);
                /* TODO wdcbit_bucket(chp, len - xfer->c_bcount); */
                // dc- junk data off the drive so it doesn't wedge
                while( Transfer->ByteCount < Length-1 ) {
                    unsigned short val;
                    Bus->Read16Multiple( Bus, Bus->DataReg, &val, 1 );
                    Transfer->ByteCount += 2;
                }


                Transfer->Skip += Transfer->ByteCount;
                Transfer->ByteCount = 0;
            }
            else {
                if (Length > 0) {
                    Bus->Read16Multiple(Bus, Bus->DataReg, (unsigned short *)((char *)Transfer->Data + Transfer->Skip),
                                        Length >> 1);
                    Transfer->Skip += Length;
                    Transfer->ByteCount -= Length;
                }
            }
            if ((Transfer->Flags & XFER_POLL) == 0) {
                if (!BusWait(Bus,Cmd->Timeout)) {
                    /* TODO Timeout IRQ handling */
                    DEBUG(ATA,DBGLEV_ERROR, "timeout waiting for irq\n" );
                }
            }
            return;
	    
        case PHASE_ABORTED:
        case PHASE_COMPLETED:
            if (Transfer->Flags & XFER_SENSE) {
                if (Bus->Status & STATUS_ERR) {
                    Cmd->Error = ATAPI_TIMEOUT;
					
                    ATAPIReset(Bus, Transfer);
                    return;
                }
                else if (Transfer->ByteCount < sizeof(Cmd->SCSISenseData)) {
                    Cmd->Error = ATAPI_SENSE;
                }
                else {
                    Cmd->Error = ATAPI_SHORT_SENSE;
                }
            }
            else {
                Cmd->Residue = Transfer->ByteCount;
                if (Bus->Status & STATUS_ERR) {
                    Cmd->Error = ATAPI_SHORT_SENSE;
                    Cmd->ATAPISense = Bus->Error;
                    if (Cmd->Flags & ATAPI_AUTO_SENSE) {
                        //ATAPICommandDone(Bus, Transfer);
                        Transfer->Data = &Cmd->SCSISenseData;
                        Transfer->ByteCount = sizeof(Cmd->SCSISenseData);
                        Transfer->Skip = 0;
                        Transfer->Flags |= XFER_SENSE;
                        /* TODO This recursion messes up the mutex */

						// tie unmask to bus unlock to fix race during polling

						if (Cmd->Flags & ATAPI_POLL) {
							
							BusUnmask(Bus);
						}


                        BusUnlock(Bus);/* Temp fix */
                        ATAPICommandStart(Bus, Transfer);
                        return;
                    }
                }
            }
            break;
	    
        default:
            if (++Retries < 500) {
                /* TODO Delay maybe */
                Bus->Status = BusRead8(Bus, Bus->StatusReg);
                Bus->Error = BusRead8(Bus, Bus->ErrorReg);
                goto Again;
            }
            if (Bus->Status & STATUS_ERR) {
                Cmd->Error = ATAPI_SHORT_SENSE;
                Cmd->ATAPISense = Bus->Error;
            }
            else {
                Cmd->Error = ATAPI_TIMEOUT;
				
                ATAPIReset(Bus, Transfer);
                return;
            }
    }

	

    ATAPICommandDone(Bus, Transfer);
    return;
}

void
ATAPICommandDone(BusSpace_T * Bus, Transfer_T * Transfer)
{
    ATAPICommand_T * Cmd;

    Cmd = (ATAPICommand_T *)Transfer->Cmd;

    Bus->Error = BusRead8(Bus, Bus->ErrorReg);
    
    /* Unlock the bus */

	if (Cmd->Flags & ATAPI_POLL) {
		

		BusUnmask(Bus);
	}



    BusUnlock(Bus);
    Cmd->Flags |= ATAPI_DONE;
    
    /* TODO Start next transfer */
}

void
ATAPIReset(BusSpace_T * Bus, Transfer_T * Transfer)
{
	
    int Drive;
    Drive = Transfer->Drive;
    DEBUG( ATA, DBGLEV_WARNING, "atapi reset on drive %d\n", Drive);
    
    BusWrite8(Bus, Bus->DeviceHeadReg, (Drive << 4));
    BusWrite8(Bus, Bus->CommandReg, ATAPI_SOFT_RESET);

    if (WaitForUnbusy(Bus, 3000 /* 30s */) != 0) {
        DEBUG(ATA,DBGLEV_ERROR, "reset failed\n" );
    }
    ATAPICommandDone(Bus, Transfer);

    // dc- Some atapi return from DEVICE RESET prematurely. in an effort
    //     to accomodate this, wait a little bit right here (50ms).
#ifdef NOKERNEL
    {
        unsigned int tm;
        tm = BusCurrentTime();
        tm += 50;
        while( BusCurrentTime() < tm ) ;
    }
#else
    cyg_thread_delay(10);
#endif
}
