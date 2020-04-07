//==========================================================================
//
//      devs/pcmcia/dharma/dharma_pcmcia.c
//
//      PCMCIA support (Card Services)
//
//==========================================================================
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm
// Contributors: toddm
// Date:         2001-09-14
// Purpose:      PCMCIA support
// Description: 
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <pkgconf/system.h>

#ifdef CYGPKG_KERNEL
#include <pkgconf/kernel.h>   // Configuration header
#include <cyg/kernel/kapi.h>
#endif

#include <pkgconf/io_pcmcia.h>

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_intr.h>           // HAL interrupt macros
#include <cyg/hal/drv_api.h>

#include <cyg/hal/hal_if.h>

#include <cyg/io/pcmcia.h>
#include <cyg/infra/diag.h>

#include <cyg/hal/hal_edb7xxx.h>  // Board definitions
#include <cyg/hal/mmgpio.h>

/* GPIO for power supply */
#define SDA (1<<4)
#define SCL (1<<5)
#define SLA0 (1<<2)

typedef enum 
{
    VL_0,
    VL_33,
    VL_5,
    VL_12
} VoltageLevel_T;

static void ClockPowerSupplyBit(int Bit);
static bool ConfigurePowerSupply(VoltageLevel_T Voltage);

#ifdef CYGPKG_KERNEL
#if defined(DHARMA_INTERRUPTS_ARE_DUMB)
static cyg_interrupt cf_detect_interrupt;
static cyg_handle_t  cf_detect_interrupt_handle;
#endif
static cyg_interrupt cf_irq_interrupt;
#if !defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
static
#endif
cyg_handle_t  cf_irq_interrupt_handle;

// fdecl
void cf_hwr_poll( struct cf_slot* slot );
#if defined(DHARMA_INTERRUPTS_ARE_DUMB)
// This ISR is called when a PC board is inserted
static int
cf_detect_isr(cyg_vector_t vector, cyg_addrword_t data, HAL_SavedRegisters *regs)
{
    struct cf_slot* slot = (struct cf_slot*) data;
    cyg_uint32 last_state = slot->state;
    
    cf_hwr_poll( slot );
    if( last_state != slot->state ) {
        cyg_drv_interrupt_acknowledge(vector);
        return (CYG_ISR_HANDLED);
    }
    else {
        return 0;
    }
}
#endif

// This ISR is called when the card interrupt occurs
static int
cf_irq_isr(cyg_vector_t vector, cyg_addrword_t data, HAL_SavedRegisters *regs)
{
#if defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
    struct cf_slot *slot = (struct cf_slot *)data;
    return (slot->irq_isr_handler.handler)(vector, slot->irq_isr_handler.param);
#else
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_EINT1);
    return (CYG_ISR_HANDLED|CYG_ISR_CALL_DSR);  // Run the DSR
#endif
}

// This DSR handles the card interrupt
static void
cf_irq_dsr(cyg_vector_t vector, cyg_ucount32 count, cyg_addrword_t data)
{
    struct cf_slot *slot = (struct cf_slot *)data;

    // Clear interrupt [edge indication]
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
    // Process interrupt
    (slot->irq_handler.handler)(vector, count, slot->irq_handler.param);
    // Allow interrupts to happen again
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
}
#endif

// This function now busy waits only, as it is only called before the scheduler is started.
static void
do_delay(int ticks)
{
    int i;
    // This is broken up to avoid overflow issues
    for (i = 0; i < 10000; ++i) {
	CYGACC_CALL_IF_DELAY_US(ticks);
    }
}

//
// Fill in the details of a PCMCIA slot and initialize the device
//
void
cf_hwr_init(struct cf_slot *slot)
{
    static int int_init = 0;
    //    cyg_uint32 new_state = GetMMGPI();

    if (!int_init) {
        int_init = 1;

	/* Configure bus to 16bit databus, 8 wait states, EXPRDY.
	   Card is found at nCS3 | A25 */
	*(volatile cyg_uint32 *)MEMCFG1 &= ~0xff000000;
	*(volatile cyg_uint32 *)MEMCFG1 |= 0x81000000 | ((0<<2)<<24);
	*(volatile cyg_uint32 *)SYSCON1 |= SYSCON1_EXCKEN; /* This doesn't seem to do anything, but I will play it safe */
	
#if defined(CYGPKG_KERNEL) 
#if defined(DHARMA_INTERRUPTS_ARE_DUMB)
        // Set up interrupts
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_EINT2,
                                 10,                     // Priority - what goes here?
                                 (cyg_addrword_t) slot,  //  Data item passed to interrupt handler
                                 (cyg_ISR_t *)cf_detect_isr,
                                 (cyg_DSR_t *)0,
                                 &cf_detect_interrupt_handle,
                                 &cf_detect_interrupt);
        cyg_drv_interrupt_attach(cf_detect_interrupt_handle);
	cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT2);
        cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT2);
#endif  // DHARMA_INTERRUPTS_ARE_DUMB
        cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_EINT1,
                                 20,                     // Priority - what goes here?
                                 (cyg_addrword_t) slot,  //  Data item passed to interrupt handler
                                 (cyg_ISR_t *)cf_irq_isr,
                                 (cyg_DSR_t *)cf_irq_dsr,
                                 &cf_irq_interrupt_handle,
                                 &cf_irq_interrupt);
#if !defined(CYGIMP_KERNEL_INTERRUPTS_CHAIN)
        cyg_drv_interrupt_attach(cf_irq_interrupt_handle);
#endif
        cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
#endif // CYGPKG_KERNEL
    }
    slot->attr = (unsigned char *)0x32000000;
    slot->attr_length = 0x00400000;
    slot->io = (unsigned char *)0x32000000;
    slot->io_length = 0x00400000;
    slot->mem = (unsigned char *)0x32000000;
    slot->mem_length = 0x00400000;
    slot->int_num = CYGNUM_HAL_INTERRUPT_EINT1;

    /* Set up GPIO before anything else so that we know what state the
       outputs are in before trying to power on the card */
    SetMMGPO(MMGPO_PCMCIA_IO_SELECT, (MMGPO_PCMCIA_8_16_SELECT | MMGPO_PCMCIA_REG));

    // figure out the card status
    cf_hwr_poll( slot );
    
#if 0
    if ((new_state & MMGPI_PCMCIA_CD) == MMGPI_PCMCIA_CD) {
        slot->state = CF_SLOT_STATE_Empty;
    } else {
	slot->state = CF_SLOT_STATE_Inserted;
    }
#endif
}

void
cf_hwr_poll(struct cf_slot *slot)
{
    cyg_uint32 new_state = GetMMGPI();
    if ((new_state & MMGPI_PCMCIA_CD) == MMGPI_PCMCIA_CD) {
        slot->state = CF_SLOT_STATE_Empty;
    } else {
        slot->state = CF_SLOT_STATE_Inserted;
    }
}

//
// Transition the card/slot to a new state
// note: currently only supports transitions to Ready, Empty
//
bool
cf_hwr_change_state(struct cf_slot *slot, int new_state)
{    
    int i;

    if (new_state == CF_SLOT_STATE_Ready) {
        if (slot->state == CF_SLOT_STATE_Inserted) {
	    do_delay(10); // At least 50ms
	    ConfigurePowerSupply(VL_5);
            do_delay(60);  // At least 300 ms
            slot->state = CF_SLOT_STATE_Powered;
	    *(volatile cyg_uint8 *)PBDDR |= (1<<0);
	    *(volatile cyg_uint8 *)PBDR |= (1<<0);
            do_delay(2);  // At least 10 us
            slot->state = CF_SLOT_STATE_Reset;
	    *(volatile cyg_uint8 *)PBDR &= ~(1<<0);
            do_delay(10); // At least 20 ms
            // Wait until the card is ready to talk (up to 10s)
	    for (i = 0; i < 100; ++i) {
		if (GetMMGPI() & MMGPI_PCMCIA_RDY) {
		    diag_printf("Ready!\n");
		    slot->state = CF_SLOT_STATE_Ready;
		    break;
		}
		do_delay(10);
	    }
	    // Unmask the interrupt now that it is safe to do so - OOPS not safe to do so here.
	    //cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_EINT1);
        }
    }
}

//
// Acknowledge interrupt (used in non-kernel environments)
//
void
cf_hwr_clear_interrupt(struct cf_slot *slot)
{
    // Clear interrupt [edge indication]
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_EINT1);
}

static void
ClockPowerSupplyBit(int Bit)
{
    /* Data is clocked in on positive leading edge of clock,
       so set data line... */
    if (Bit) {
	*(volatile cyg_uint8 *)PDDR |= SDA;
    }
    else {
	*(volatile cyg_uint8 *)PDDR &= ~SDA;
    }

    /* ...and clock it in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    do_delay(1);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;
}
    
static bool
ConfigurePowerSupply(VoltageLevel_T Voltage) 
{
    int ConfigurationData;
    int BitMask;

    /* Configure the GPIO first
      PD4 - SDA
      PD5 - SCL
      PB2 - SLA0 */
    *(volatile cyg_uint8 *)PDDDR &= ~(SDA | SCL);
    *(volatile cyg_uint8 *)PBDDR |= SLA0;
    
    /* Configuration will be for 5V, this gives the following
       bits, MSB (D8) first: 1 0110 1010 */
    if (Voltage != VL_5) {
	diag_printf("XXX Not supported XXX\n");
	return false;
    }
    ConfigurationData = 0x16a;

    /* The interface cannot be clocked faster than 2.5MHz, which
       is a 0.4uS pulse width, so we will be fine */

    /* The datasheet shows a clock before the first data bit is
       clocked in */
    *(volatile cyg_uint8 *)PDDR |= SCL;
    do_delay(1);
    *(volatile cyg_uint8 *)PDDR &= ~SCL;

    /* Now clock the data in */
    for (BitMask = 0x100; BitMask; BitMask >>= 1) {
	ClockPowerSupplyBit(ConfigurationData & BitMask);
    }

    /* Now latch the data in */
    *(volatile cyg_uint8 *)PBDR |= SLA0;
    do_delay(1);
    *(volatile cyg_uint8 *)PBDR &= ~SLA0;

    return true;
}
