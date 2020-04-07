#include <cyg/hal/hal_diag.h>
#include <devs/storage/ata/_ata_hw.h>
#include <util/debug/debug.h>
#include "atabus.h"
#include "busctrl.h"

DEBUG_USE_MODULE(ATA);

void cf_power_on( BusSpace_T* bus ) 
{
    /* Power on interface */
    *(volatile cyg_uint8 *)PBDDR |= 0x20;
    *(volatile cyg_uint8 *)PBDR &= ~0x20;
}
void cf_power_off( BusSpace_T* bus ) 
{
    /* Power off interface */
    *(volatile cyg_uint8 *)PBDDR |= 0x20;
    *(volatile cyg_uint8 *)PBDR |= 0x20;
}

/* Use the PCMCIA reset sequence */
void
cf_hard_reset(BusSpace_T * Bus)
{
    volatile cyg_uint8* PORT = (volatile cyg_uint8*) HW_ATA_BUS_1_HARD_RESET_PORT;
    volatile cyg_uint8* DDR  = PORT + 0x40;
    int Status;
    int NumTries;
    int RetryCount;

    DBTR(ATA);
    
    /* Set up GPIO */
    *(volatile cyg_uint8 *)PBDDR |= 0x20; /* Power */
    *DDR |= HW_ATA_BUS_1_HARD_RESET_PIN;

    /* Try this multiple times in the hopes that it will eventually work */
    for( RetryCount = 0; RetryCount < 10; RetryCount++ ) {
	/* Check whether we've already applied power */
	if (!(*(volatile cyg_uint8 *)PBDR & 0x20)) {
	    DEBUG(ATA, DBGLEV_INFO, "Power is applied, turning OFF\n");
	    cf_power_off(Bus);
	}
	
	/* Check whether media is actually inserted */
	Status = (*(volatile unsigned char *)HW_ATA_BUS_1_CD_PORT) & HW_ATA_BUS_1_CD_PIN;
	if (Status & HW_ATA_BUS_1_CD_PIN) {
            DEBUG(ATA, DBGLEV_FATAL, "FIXME No media FIXME\n");
            // dc- for now, don't try to initialize an absent card. this puts us out of sync
            //     since the rest of the driver thinks a card is actually present, and will
            //     do so until the next media change
            return ;
	}
	
	/* Now start the PCMCIA power on/reset sequence */
	
	/* Wait after card detect a bit */
	cyg_thread_delay(10);
	
	cf_power_on(Bus);
	cyg_thread_delay(60);
	
	/* Assert reset */
	*PORT &= ~(HW_ATA_BUS_1_HARD_RESET_PIN);
	cyg_thread_delay(2);
	
	/* De-assert reset */
	*PORT |= (HW_ATA_BUS_1_HARD_RESET_PIN);
	cyg_thread_delay(10);
	
	/* Wait for ready */
	BusWrite8(Bus, Bus->DeviceHeadReg, 0);
	for (NumTries = 0; NumTries < 10; ++NumTries) {
	    Status = BusRead8(Bus, Bus->StatusReg);
	    if (!(Status & STATUS_BSY)) {
		break;
	    }
	    DEBUG(ATA, DBGLEV_INFO, "Status %x\n", Status);
	    cyg_thread_delay(2);
	}
        // successfull init
        if( NumTries < 10 ) {
            break;
        }
    }
}

