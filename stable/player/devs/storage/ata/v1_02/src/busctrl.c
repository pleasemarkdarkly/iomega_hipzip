// busctrl.c: functions for controlling the ATA/ATAPI bus
// danc@iobjects.com 7/03/01
// (c) Interactive Objects

#include <cyg/hal/hal_edb7xxx.h>   // for syscon1
#include <cyg/hal/hal_diag.h>      // for HAL_DELAY_US
#include <util/debug/debug.h>
#include <devs/storage/ata/_ata_hw.h>

#include "atabus.h"
#include "busctrl.h"

DEBUG_USE_MODULE(ATA);

#if defined(HW_ATA_ENABLE_BUS_0)
extern BusSpace_T Bus0;
#endif
#if defined(HW_ATA_ENABLE_BUS_1)
extern BusSpace_T Bus1;
#endif

void
ata_hard_reset(BusSpace_T* bus)
{
	unsigned int Timeout;
	unsigned char Status;


#ifdef __DJ
	// pull reset line low
	*(volatile cyg_uint8 *)PDDR &= ~0x02;

	// min 25us wait
    HAL_DELAY_US(25);

	*(volatile cyg_uint8 *)PDDR |= 0x02;

	// min 2ms wait
    HAL_DELAY_US(2000);

#if 0
    // Wait on bus master first
    for (Timeout = 0; Timeout < 50; ++Timeout) {
        // TODO: is it safe to assume a master is on the bus?
        BusWrite8(bus, bus->DeviceHeadReg, 0);
        _Wait(100);
        Status = BusRead8(bus, bus->StatusReg);

        if ( (Status & STATUS_BSY) == 0) {
            break;
        }

        _Wait(100);
    }


	DEBUG(ATA, DBGLEV_WARNING, "hard reset waited %d on ATA master\n",Timeout);
#endif
#endif


}

int
BusInit( BusSpace_T* bus ) 
{
    if( !bus->Initialized ) {
        *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
        *(bus->MemcfgRegister) |= bus->MemcfgDefaultSetting;

        // unsure if this is really needed
        *(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN;

#ifndef NOKERNEL
        cyg_semaphore_init( &(bus->IRQSem), 0 );
        cyg_mutex_init( &(bus->Mutex) );

        cyg_drv_interrupt_create( bus->IRQ,
                                  99,
                                  (unsigned int)bus,
                                  (cyg_ISR_t*) bus->ISR,
                                  (cyg_DSR_t*) bus->DSR,
                                  &(bus->IRQHandle),
                                  &(bus->IRQObject));
    
        cyg_drv_interrupt_attach( bus->IRQHandle );
        cyg_drv_interrupt_acknowledge( bus->IRQ );

        bus->MediaChangeCB = NULL;
#endif /* NOKERNEL */

        bus->Initialized = true;

#if defined(HW_ATA_ENABLE_BUS_0)
        if (bus == &Bus0) {
#if defined(HW_ATA_BUS_0_CD_DDR)
            if( HW_ATA_BUS_0_CD_DDR == PDDDR ) {
                *(volatile cyg_uint8*)HW_ATA_BUS_0_CD_DDR |= (HW_ATA_BUS_0_CD_PIN);
            }
            else {
                *(volatile cyg_uint8*)HW_ATA_BUS_0_CD_DDR &= ~(HW_ATA_BUS_0_CD_PIN);
            }
#endif
#if defined(HW_ATA_BUS_0_CD_PORT)
            bus->Device[0].MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_0_CD_PORT) & HW_ATA_BUS_0_CD_PIN;
            bus->Device[0].MediaPresent = bus->Device[0].MediaPresent ? 0 : 1;
#else
            /* Default to media present */
            bus->Device[0].MediaPresent = 1;
#endif
        }
#endif /* HW_ATA_ENABLE_BUS_0 */

#if defined(HW_ATA_ENABLE_BUS_1)
        if (bus == &Bus1) {
#if defined(HW_ATA_BUS_1_CD_DDR)
            if( HW_ATA_BUS_1_CD_DDR == PDDDR ) {
                *(volatile cyg_uint8*)HW_ATA_BUS_1_CD_DDR |= (HW_ATA_BUS_1_CD_PIN);
            }
            else {
                *(volatile cyg_uint8*)HW_ATA_BUS_1_CD_DDR &= ~(HW_ATA_BUS_1_CD_PIN);
            }
	    
#endif
#if defined(HW_ATA_BUS_1_CD_PORT)
            bus->Device[0].MediaPresent = (*(volatile unsigned char *)HW_ATA_BUS_1_CD_PORT) & HW_ATA_BUS_1_CD_PIN;
            bus->Device[0].MediaPresent = bus->Device[0].MediaPresent ? 0 : 1;
#else
            /* Default to media present */
            bus->Device[0].MediaPresent = 1;
#endif
        }
#endif /* HW_ATA_ENABLE_BUS_1 */

        if (bus->Device[0].MediaPresent) {
            bus->Device[0].MediaStatus = ENOERR;
        }
        else {
            bus->Device[0].MediaStatus = -ENOMED;
        }

        /* This is not yet supported in any hardware, so default to present */
        bus->Device[1].MediaPresent = 1;
        bus->Device[1].MediaStatus = ENOERR;

        return true;
    }
    return false;
}

void
BusReset( BusSpace_T* bus )
{
    if( bus->IsReset )
        return ;

#ifndef DISABLE_ATA_HARD_RESET
	ata_hard_reset( bus );
#endif
    ATASoftReset( bus );
}

void
BusPowerDown( BusSpace_T* bus )
{
}

void
BusPowerUp( BusSpace_T* bus )
{
}
#ifndef NOKERNEL
cyg_uint32
BusInterruptISR( cyg_vector_t Vector, cyg_addrword_t Data ) 
{
    BusSpace_T* bus = (BusSpace_T*) Data;

    cyg_drv_interrupt_acknowledge( bus->IRQ );
    cyg_drv_interrupt_mask( bus->IRQ );

    return (CYG_ISR_HANDLED | CYG_ISR_CALL_DSR);
}

void
BusInterruptDSR( cyg_vector_t Vector, cyg_ucount32 Count, cyg_addrword_t Data ) 
{
    BusSpace_T* bus = (BusSpace_T*) Data;

    cyg_semaphore_post( &(bus->IRQSem) );
}
#endif /* NOKERNEL */

/* Bus access functions */
#ifndef __OPTIMIZE__
unsigned char
BusRead8( BusSpace_T* Bus, unsigned int Offset)
{
    return *((volatile char *)(Bus->Base + Offset));
}
void
BusWrite8( BusSpace_T* Bus, unsigned int Offset, unsigned char Value) 
{
    *((volatile char *)(Bus->Base + Offset)) = Value;
}
#endif
void
BusWrite16( BusSpace_T* bus, unsigned int Offset, unsigned short Value )
{
    // dc- original Write16 didn't cast this to unsigned...hmm
    *((volatile short*)(bus->Base + Offset)) = Value;
}


#define ALIGN_16( src ) ({ unsigned short val = (unsigned short)*src++;val |= (((unsigned short)*src++)<<8); val; })
// note that length is in shorts
void
BusWrite16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length )
{
    volatile unsigned short* addr = (unsigned short*) (bus->Base + Offset);

    if( (unsigned int)(Buf) & 0x01 ) {
        // attempt to handle unaligned buffers
        // note that this is a huge performance hit, because we have to assemble 16bit values from
        // a pair of 8 bit ones. if this can be avoided, please do so

        unsigned char* start = (unsigned char*)Buf;
        unsigned char* lim = start + (Length<<1);

        DEBUG( ATA, DBGLEV_INFO, "Unaligned data buffer\n");
        
        while( start < (lim-6) ) {
            *addr = ALIGN_16( start );
            *addr = ALIGN_16( start );
            *addr = ALIGN_16( start );
            *addr = ALIGN_16( start );
        }
        while( start < (lim-2) ) {
            *addr = ALIGN_16( start );
            *addr = ALIGN_16( start );
        }
        while( start < lim ) {
            *addr = ALIGN_16( start );
        }
    } else {
        unsigned short* start = Buf;
        unsigned short* lim = start + Length;
        
        while( start < (lim-3) ) {
            *addr = *start++;
            *addr = *start++;
            *addr = *start++;
            *addr = *start++;
        }
        while( start < (lim-1) ) {
            *addr = *start++;
            *addr = *start++;
        }
        while( start < lim ) {
            *addr = *start++;
        }
    }
}

#define READ_16_ALIGNED( dst, src )    *dst++ = *src
#define READ_16_UNALIGNED( dst, src )  ({ unsigned short val = *src; *dst++ = (unsigned char)(val&0xff); *dst++ = (unsigned char)(val>>8); })
void
BusRead16Multiple( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length )
{
    volatile unsigned short* addr = (unsigned short*) (bus->Base + Offset);

    if( ((unsigned int)(Buf) & 0x03) == 0 && (Length & 0x03) == 0 ) {
        unsigned int* start = (unsigned int*) Buf;
        unsigned int* lim = (unsigned int*) (Buf + Length);

        // this code does fast as shit reads on word aligned buffers
        // %0 = base address
        // %1 = store address
        // %2 = limit address
        // r6, r7, r8, r9, r10, r2 = data registers
        // note- the compiler is dumb enough to let me use the fp, then try
        //       to restore the stack from it. smooth.
        asm volatile ("0:              ;"
                      "ldrh      r6, [%0];"    /* load the first 6 shorts */
                      "ldrh      r7, [%0];"
                      "ldrh      r8, [%0];"
                      "ldrh      r9, [%0];"
                      "ldrh      r10,[%0];"
                      "ldrh      r2 ,[%0];"
                      "orr       r6, r6, r7,lsl #16;"  /* assemble the first 3 words */
                      "orr       r7, r8, r9,lsl #16;"
                      "orr       r8,r10, r2,lsl #16;"
                      "ldrh      r9, [%0];"            /* load the last 2 shorts */
                      "ldrh      r10,[%0];"
                      "orr       r9, r9,r10,lsl #16;"  /* assemble */
                      "stmia     %1!, {r6,r7,r8,r9};"
                      "cmp       %1, %2;"
                      "blt       0b;"
                      :
                      : "r" (addr), "r" (start), "r" (lim)
                      : "r6", "r7", "r8", "r9", "r10", "r2" );
    } else if( ((unsigned int)(Buf) & 0x01) == 0 ) {
        // this code handles short aligned buffers
        unsigned short* start = Buf;
        unsigned short* lim = start + Length;
        
        while( start < (lim-3) ) {
            READ_16_ALIGNED( start, addr );
            READ_16_ALIGNED( start, addr );
            READ_16_ALIGNED( start, addr );
            READ_16_ALIGNED( start, addr );
        }
        while( start < (lim-1) ) {
            READ_16_ALIGNED( start, addr );
            READ_16_ALIGNED( start, addr );
        }
        while( start < lim ) 
        {
            READ_16_ALIGNED( start, addr );
        }
    } else { //if( (unsigned int)(Buf) & 0x01 ) {
        // attempt to handle unaligned buffers
        unsigned char* start = (unsigned char*) Buf;
        unsigned char* lim   = start + (Length<<1);

        DEBUG( ATA, DBGLEV_INFO, "Unaligned data buffer\n");

        while( start < (lim-6) ) {
            READ_16_UNALIGNED( start, addr );
            READ_16_UNALIGNED( start, addr );
            READ_16_UNALIGNED( start, addr );
            READ_16_UNALIGNED( start, addr );
        }
        while( start < (lim-2) ) {
            READ_16_UNALIGNED( start, addr );
            READ_16_UNALIGNED( start, addr );
        }
        while( start < lim ) 
        {
            READ_16_UNALIGNED( start, addr );
        }
    }

}

void
BusWrite16M( BusSpace_T* bus, unsigned int Offset, unsigned short Value )
{
    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->Memcfg16bitSetting;

    *((volatile unsigned short*)(bus->Base + Offset)) = Value;

    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->MemcfgDefaultSetting;
}

void
BusWrite16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length )
{
    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->Memcfg16bitSetting;

#if 0  // set to 1 if unrolled write loops dont do it for you
    {
        volatile unsigned short * Address;
        int i;
	
        Address = (volatile unsigned short *)(bus->Base + Offset);
        for (i = 0; i < Length; ++i) {
            *Address = Buf[i];
        }
    }
#else
    // probably not the most efficient way to do this, but the loop unroll adds about 5% to the
    // throughput on this driver, so in the end it should balance out.
    BusWrite16Multiple( bus, Offset, Buf, Length );
#endif
    
    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->MemcfgDefaultSetting;
}

void
BusRead16MultipleM( BusSpace_T* bus, unsigned int Offset, unsigned short* Buf, int Length )
{
    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->Memcfg16bitSetting;

    BusRead16Multiple( bus, Offset, Buf, Length );

    *(bus->MemcfgRegister) &= ~(bus->MemcfgMask);
    *(bus->MemcfgRegister) |= bus->MemcfgDefaultSetting;
}
