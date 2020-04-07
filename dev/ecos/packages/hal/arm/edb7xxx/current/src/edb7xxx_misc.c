//==========================================================================
//
//      edb7xxx_misc.c
//
//      HAL misc board support code for ARM EDB7XXX-1
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// The contents of this file are subject to the Red Hat eCos Public License 
// Version 1.1 (the "License"); you may not use this file except in         
// compliance with the License.  You may obtain a copy of the License at    
// http://www.redhat.com/                                                   
//                                                                          
// Software distributed under the License is distributed on an "AS IS"      
// basis, WITHOUT WARRANTY OF ANY KIND, either express or implied.  See the 
// License for the specific language governing rights and limitations under 
// the License.                                                             
//                                                                          
// The Original Code is eCos - Embedded Configurable Operating System,      
// released September 30, 1998.                                             
//                                                                          
// The Initial Developer of the Original Code is Red Hat.                   
// Portions created by Red Hat are                                          
// Copyright (C) 1998, 1999, 2000, 2001 Red Hat, Inc.                             
// All Rights Reserved.                                                     
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    gthomas
// Contributors: gthomas
// Date:         1999-02-20
// Purpose:      HAL board support
// Description:  Implementations of HAL board interfaces
//
//####DESCRIPTIONEND####
//
//========================================================================*/

#include <pkgconf/hal.h>
#include <pkgconf/system.h>
#include CYGBLD_HAL_PLATFORM_H

#include <cyg/infra/cyg_type.h>         // base types
#include <cyg/infra/cyg_trac.h>         // tracing macros
#include <cyg/infra/cyg_ass.h>          // assertion macros

#include <cyg/hal/hal_io.h>             // IO macros
#include <cyg/hal/hal_arch.h>           // Register state info
#include <cyg/hal/hal_diag.h>
#include <cyg/hal/hal_intr.h>           // Interrupt names
#include <cyg/hal/hal_cache.h>
#include <cyg/hal/hal_edb7xxx.h>         // Hardware definitions
#include <cyg/hal/hal_if.h>             // calling interface API

// #define CYGHWR_HAL_ARM_EDB7XXX_BATLOW
#ifdef CYGHWR_HAL_ARM_EDB7XXX_BATLOW
#include <cyg/hal/hal_intr.h>           // HAL interrupt macros
#include <cyg/hal/drv_api.h>            // HAL ISR support
static cyg_interrupt batlow_interrupt;
static cyg_handle_t  batlow_interrupt_handle;
#endif

#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 18432)
#define CPU_CLOCK 0
#elif (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 36864)
#define CPU_CLOCK 1
#elif (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 49152)
#define CPU_CLOCK 2
#elif (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 73728)
#define CPU_CLOCK 3
#elif (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#define CPU_CLOCK 3
#else
#err Invalid CPU clock frequency
#endif

static cyg_uint32 _period;
#if !defined(CYGHWR_HAL_ARM_EDB7XXX_VARIANT_EP7312)
void dram_delay_loop(void);
#endif // CYGHWR_HAL_ARM_EDB7XXX_VARIANT_EP7312

// Use Timer/Counter #2 for system clock

void hal_clock_initialize(cyg_uint32 period)
{
    volatile cyg_uint32 *syscon1 = (volatile cyg_uint32 *)SYSCON1;
    volatile cyg_uint32 *tc2d = (volatile cyg_uint32 *)TC2D;
    // Set timer to 512KHz, prescale mode
    *syscon1 = (*syscon1 & ~(SYSCON1_TC2M|SYSCON1_TC2S)) | SYSCON1_TC2S | SYSCON1_TC2M;
    // Initialize counter
    *tc2d = period;
    _period = period;
}

// This routine is called during a clock interrupt.

static void __inline__
enable_FIQ(void)
{
    asm volatile ("mrs r0,cpsr;"
                  "bic r0,r0,#0x40;"
                  "msr cpsr,r0");
}

#ifdef CYGHWR_HAL_ARM_EDB7XXX_SOFTWARE_DRAM_REFRESH
#error nooo!
#define DRAM_START    0x00000000
#define DRAM_END      0x01000000
#define DRAM_ROW_SIZE 0x00000400
#define DRAM_REFRESH  (((DRAM_END-DRAM_START)/DRAM_ROW_SIZE)+99)/100

static void
do_DRAM_refresh(void)
{
    static cyg_uint32 *row_ptr;
    volatile cyg_uint32 val;
    int i;
    for (i = 0;  i < DRAM_REFRESH;  i++) {
        val = *row_ptr;
        row_ptr += DRAM_ROW_SIZE / sizeof(*row_ptr);
        if (row_ptr >= (cyg_uint32 *)DRAM_END) row_ptr = (cyg_uint32 *)DRAM_START;
    }
}
#endif // CYGHWR_HAL_ARM_EDB7XXX_SOFTWARE_DRAM_REFRESH

void hal_clock_reset(cyg_uint32 vector, cyg_uint32 period)
{
    volatile cyg_uint32 *tc2d = (volatile cyg_uint32 *)TC2D;
    if (period != _period) {
        *tc2d = period;
        _period = period;
    }
#if 0
#ifndef CYGPKG_HAL_ARM_EDB7209
// EP7209 has no DRAM/controller, thus no problem
    enable_FIQ();  // Should be safe here
#error aaaah!
#ifdef CYGHWR_HAL_ARM_EDB7XXX_SOFTWARE_DRAM_REFRESH
    do_DRAM_refresh();
#error aaah!
#elif !defined(CYGHWR_HAL_ARM_EDB7XXX_VARIANT_EP7312)
#error aaah!2
    dram_delay_loop();
#endif  // _DRAM_REFRESH
#endif  // _EDB7209
#endif // jesus, don't enable any of this code. 
}

// Read the current value of the clock, returning the number of hardware "ticks"
// that have occurred (i.e. how far away the current value is from the start)

void hal_clock_read(cyg_uint32 *pvalue)
{
    volatile cyg_int32 *tc2d = (volatile cyg_int32 *)TC2D;
    static cyg_int32 clock_val;
    clock_val = *tc2d & 0x0000FFFF;                 // Register has only 16 bits
    if (clock_val & 0x00008000) clock_val |= 0xFFFF8000;  // Extend sign bit
    *pvalue = (cyg_uint32)(_period - clock_val);    // 'clock_val' counts down and wraps
}

// Delay for some number of useconds.  Assume that the system clock
// has been set up to run at 512KHz (default).
void hal_delay_us(int us)
{
    volatile cyg_int32 *tc2d = (volatile cyg_int32 *)TC2D;
    cyg_int32 val, prev;
    while (us >= 2) {
        prev = *tc2d & 0x0000FFFF;                 // Register has only 16 bits
        if (prev & 0x00008000) prev |= 0xFFFF8000;  // Extend sign bit
        while (true) {
            val = *tc2d & 0x0000FFFF;                 // Register has only 16 bits
            if (val & 0x00008000) {
                val |= 0xFFFF8000;  // Extend sign bit
                *tc2d = _period;  // Need to reset counter
            }
            if (val != prev) {
                break;  // At least 2us have passed
            }            
        }
        us -= 2;
    }
}

void
dram_delay_loop(void)
{
    // Temporary fix for DRAM starvation problem
    if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK > 37000) {
        int i;
        for (i = 0;  i < (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK*2)/24;  i++) ;  // approx 300 us
    }
}

#if defined(__DHARMA_V2)
// Fake an interrupt mask register for the expansion interrupts on dharma
static unsigned int XINTMR = 0xffffffff;
#endif

// These tables map the various [soft] interrupt numbers onto the hardware

static cyg_uint32 hal_interrupt_bitmap[] = {
    0,              // CYGNUM_HAL_INTERRUPT_unused     0
    INTSR1_EXTFIQ,  // CYGNUM_HAL_INTERRUPT_EXTFIQ     1
    INTSR1_BLINT,   // CYGNUM_HAL_INTERRUPT_BLINT      2
    INTSR1_WEINT,   // CYGNUM_HAL_INTERRUPT_WEINT      3
    INTSR1_MCINT,   // CYGNUM_HAL_INTERRUPT_MCINT      4
    INTSR1_CSINT,   // CYGNUM_HAL_INTERRUPT_CSINT      5
    INTSR1_EINT1,   // CYGNUM_HAL_INTERRUPT_EINT1      6    
    INTSR1_EINT2,   // CYGNUM_HAL_INTERRUPT_EINT2      7
    INTSR1_EINT3,   // CYGNUM_HAL_INTERRUPT_EINT3      8
    INTSR1_TC1OI,   // CYGNUM_HAL_INTERRUPT_TC1OI      9
    INTSR1_TC2OI,   // CYGNUM_HAL_INTERRUPT_TC2OI      10
    INTSR1_RTCMI,   // CYGNUM_HAL_INTERRUPT_RTCMI      11
    INTSR1_TINT,    // CYGNUM_HAL_INTERRUPT_TINT       12
    INTSR1_UTXINT1, // CYGNUM_HAL_INTERRUPT_UTXINT1    13
    INTSR1_URXINT1, // CYGNUM_HAL_INTERRUPT_URXINT1    14
    INTSR1_UMSINT,  // CYGNUM_HAL_INTERRUPT_UMSINT     15
    INTSR1_SSEOTI,  // CYGNUM_HAL_INTERRUPT_SSEOTI     16
    INTSR2_KBDINT,  // CYGNUM_HAL_INTERRUPT_KBDINT     17
    INTSR2_SS2RX,   // CYGNUM_HAL_INTERRUPT_SS2RX      18
    INTSR2_SS2TX,   // CYGNUM_HAL_INTERRUPT_SS2TX      19
    INTSR2_UTXINT2, // CYGNUM_HAL_INTERRUPT_UTXINT2    20
    INTSR2_URXINT2, // CYGNUM_HAL_INTERRUPT_URXINT2    21
#if defined(__EDB7211)
    INTSR3_MCPINT,  // CYGNUM_HAL_INTERRUPT_MCPINT     22
#endif
#if defined(__EDB7209)
    INTSR3_I2SINT,  // CYGNUM_HAL_INTERRUPT_I2SINT     22
#endif
#if defined(__DHARMA_V2)
    XINTSR_XINT0,   // CYGNUM_HAL_INTERRUPT_XINT0      23
#endif
};

static cyg_uint32 hal_interrupt_mask_regmap[] = {
    0,      // CYGNUM_HAL_INTERRUPT_unused     0
    INTMR1, // CYGNUM_HAL_INTERRUPT_EXTFIQ     1
    INTMR1, // CYGNUM_HAL_INTERRUPT_BLINT      2
    INTMR1, // CYGNUM_HAL_INTERRUPT_WEINT      3
    INTMR1, // CYGNUM_HAL_INTERRUPT_MCINT      4
    INTMR1, // CYGNUM_HAL_INTERRUPT_CSINT      5
    INTMR1, // CYGNUM_HAL_INTERRUPT_EINT1      6    
    INTMR1, // CYGNUM_HAL_INTERRUPT_EINT2      7
    INTMR1, // CYGNUM_HAL_INTERRUPT_EINT3      8
    INTMR1, // CYGNUM_HAL_INTERRUPT_TC1OI      9
    INTMR1, // CYGNUM_HAL_INTERRUPT_TC2OI      10
    INTMR1, // CYGNUM_HAL_INTERRUPT_RTCMI      11
    INTMR1, // CYGNUM_HAL_INTERRUPT_TINT       12
    INTMR1, // CYGNUM_HAL_INTERRUPT_UTXINT1    13
    INTMR1, // CYGNUM_HAL_INTERRUPT_URXINT1    14
    INTMR1, // CYGNUM_HAL_INTERRUPT_UMSINT     15
    INTMR1, // CYGNUM_HAL_INTERRUPT_SSEOTI     16
    INTMR2, // CYGNUM_HAL_INTERRUPT_KBDINT     17
    INTMR2, // CYGNUM_HAL_INTERRUPT_SS2RX      18
    INTMR2, // CYGNUM_HAL_INTERRUPT_SS2TX      19
    INTMR2, // CYGNUM_HAL_INTERRUPT_UTXINT2    20
    INTMR2, // CYGNUM_HAL_INTERRUPT_URXINT2    21
#if defined(__EDB7211)
    INTMR3, // CYGNUM_HAL_INTERRUPT_MCPINT     22
#endif
#if defined(__EDB7209)
    INTMR3, // CYGNUM_HAL_INTERRUPT_I2SINT     22
#endif
#if defined(__DHARMA_V2)
    (cyg_uint32)&XINTMR, // CYGNUM_HAL_INTERRUPT_XINT0      23
#endif
};

static cyg_uint32 hal_interrupt_clear_map[] = {
    0,      // CYGNUM_HAL_INTERRUPT_unused     0
    0,      // CYGNUM_HAL_INTERRUPT_EXTFIQ     1
    BLEOI,  // CYGNUM_HAL_INTERRUPT_BLINT      2
    TEOI,   // CYGNUM_HAL_INTERRUPT_WEINT      3
    MCEOI,  // CYGNUM_HAL_INTERRUPT_MCINT      4
    COEOI,  // CYGNUM_HAL_INTERRUPT_CSINT      5
    0,      // CYGNUM_HAL_INTERRUPT_EINT1      6    
    0,      // CYGNUM_HAL_INTERRUPT_EINT2      7
    0,      // CYGNUM_HAL_INTERRUPT_EINT3      8
    TC1EOI, // CYGNUM_HAL_INTERRUPT_TC1OI      9
    TC2EOI, // CYGNUM_HAL_INTERRUPT_TC2OI      10
    RTCEOI, // CYGNUM_HAL_INTERRUPT_RTCMI      11
    TEOI,   // CYGNUM_HAL_INTERRUPT_TINT       12
    0,      // CYGNUM_HAL_INTERRUPT_UTXINT1    13
    0,      // CYGNUM_HAL_INTERRUPT_URXINT1    14
    UMSEOI, // CYGNUM_HAL_INTERRUPT_UMSINT     15
    0,      // CYGNUM_HAL_INTERRUPT_SSEOTI     16
    KBDEOI, // CYGNUM_HAL_INTERRUPT_KBDINT     17
    0,      // CYGNUM_HAL_INTERRUPT_SS2RX      18
    0,      // CYGNUM_HAL_INTERRUPT_SS2TX      19
    0,      // CYGNUM_HAL_INTERRUPT_UTXINT2    20
    0,      // CYGNUM_HAL_INTERRUPT_URXINT2    21
#if defined(__EDB7211)
    0,      // CYGNUM_HAL_INTERRUPT_MCPINT     22
#endif
#if defined(__EDB7209)
    0,      // CYGNUM_HAL_INTERRUPT_I2SINT     22
#endif
#if defined(__DHARMA_V2)
    0,      // CYGNUM_HAL_INTERRUPT_XINT0      23
#endif
};

static struct regmap {
    int        first_int, last_int;
    cyg_uint32 stat_reg, mask_reg;
} hal_interrupt_status_regmap[] = {
    { CYGNUM_HAL_INTERRUPT_EXTFIQ, CYGNUM_HAL_INTERRUPT_SSEOTI,   INTSR1, INTMR1},
    { CYGNUM_HAL_INTERRUPT_KBDINT, CYGNUM_HAL_INTERRUPT_URXINT2, INTSR2, INTMR2},
#if defined(__EDB7211)
    { CYGNUM_HAL_INTERRUPT_MCPINT, CYGNUM_HAL_INTERRUPT_MCPINT,  INTSR3, INTMR3},
#endif
#if defined(__EDB7209)
    { CYGNUM_HAL_INTERRUPT_I2SINT, CYGNUM_HAL_INTERRUPT_I2SINT,  INTSR3, INTMR3},
#endif
    //    { CYGNUM_HAL_INTERRUPT_CSINT,  CYGNUM_HAL_INTERRUPT_SSEOTI,  INTSR1, INTMR1},
#if defined(__DHARMA_V2)
    { CYGNUM_HAL_INTERRUPT_XINT0,  CYGNUM_HAL_INTERRUPT_XINT0, XINTSR_XINT0, (cyg_uint32)&XINTMR },
#endif
    { 0, 0, 0, 0 }
};

#ifdef CYGHWR_HAL_ARM_EDB7XXX_BATLOW
// This ISR is called when the battery low interrupt occurs
int 
cyg_hal_batlow_isr(cyg_vector_t vector, cyg_addrword_t data, HAL_SavedRegisters *regs)
{
    diag_printf("Battery low\n");
    cyg_drv_interrupt_mask(CYGNUM_HAL_INTERRUPT_BLINT);
    // Presumably, one would leave this masked until the battery changed
    cyg_drv_interrupt_acknowledge(CYGNUM_HAL_INTERRUPT_BLINT);
    return 0;  // No need to run DSR
}
#endif

// Weak-linking symbol to perform custom board initialization. The assumption
// is that target devices will be spun, but it's inconvenient to modify this file
// for each minor change to a hardware layout. This call allows us to customize
// prior to static initialization.
externC void
hal_edb7xxx_board_init( void ) CYGBLD_ATTRIB_WEAK;

//
// Early stage hardware initialization
//   Some initialization has already been done before we get here.  For now
// just set up the interrupt environment.

void hal_hardware_init(void)
{
    volatile cyg_uint32 *icr;
    int vector;
    
    // Clear and initialize instruction cache
    HAL_ICACHE_INVALIDATE_ALL();
    HAL_ICACHE_ENABLE();

    // Any hardware/platform initialization that needs to be done.
    *(volatile cyg_uint32 *)INTMR1 = 0;
    *(volatile cyg_uint32 *)INTMR2 = 0;
#if !defined(__CL7111)
    *(volatile cyg_uint32 *)INTMR3 = 0;
#if !defined(__EDB7312)
    *(volatile cyg_uint8 *)SYSCON3 = SYSCON3_CLKCTL(CPU_CLOCK);
#endif // __EDB7312
#endif // __CL7111

#if 0
    diag_printf("IMR1: %04x, IMR2: %04x\n",
                *(volatile cyg_uint32 *)INTMR1,
                *(volatile cyg_uint32 *)INTMR2);
    diag_printf("Memcfg1: %08x, Memcfg2: %08x, DRAM refresh: %08x\n", 
                *(volatile cyg_uint32 *)MEMCFG1,
                *(volatile cyg_uint32 *)MEMCFG2,
                *(volatile cyg_uint8 *)DRFPR);
#endif

#if 0
    
#if 0
#define MEMCFG_BUS_WIDTH(n)   (n<<0)
#define MEMCFG_BUS_WIDTH_32   (0<<0)
#define MEMCFG_BUS_WIDTH_16   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (2<<0)
#define MEMCFG_WAIT_STATES(n) (n<<2)     // 0 is max, 15 min
#define MEMCFG_SQAEN          (1<<6)
#define MEMCFG_CLKENB         (1<<7)

// These need to be checked/improved
#define CS0_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(3) | MEMCFG_SQAEN
#define CS1_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(4)
#define CS2_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#define CS3_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#define CS4_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#define CS5_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#define CS6_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#define CS7_CONFIG MEMCFG_BUS_WIDTH_32 | MEMCFG_WAIT_STATES(0)
#endif
    
#if defined(__EDB7209)
#if defined(__EDB7312)

    // Bus configuration

#if defined(__DJ)
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#error Darwin Jukebox not set for 90Mhz yet
#else
#define CS0_CONFIG 0x54 // FLASH ROM - 16 bit
#define CS2_CONFIG 0x00 // Ethernet (now CS2) 16-bit, slow
#define CS1_CONFIG 0x00 // Disabled  
#define CS3_CONFIG 0x00 // Disabled
#define CS4_CONFIG 0x1b // DJ LCD
#define CS5_CONFIG 0x00 // IDE 16 bit, slow
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#endif
#else /* Not Darwin Jukebox */

#ifdef __DHARMA
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#error Dharma 2 not set for 90Mhz yet
#else
#define CS0_CONFIG 0x54 // FLASH ROM
    //#define CS0_CONFIG 0x14
#define CS1_CONFIG 0x00 // Ethernet
#define CS2_CONFIG 0x18 // Memory Module
#define CS3_CONFIG 0x3C // IO Expansion / PCMCIA / LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#endif
#else /* __DHARMA == 1 */
#if (CYGHWR_HAL_ARM_EDB7XXX_PROCESSOR_CLOCK == 90000)
#define CS0_CONFIG 0x10 // FLASH ROM
#define CS1_CONFIG 0x18 // Memory Module
#define CS2_CONFIG 0x80 // Ethernet
#define CS3_CONFIG 0x3C // LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#else
#define CS0_CONFIG 0x54 // FLASH ROM
#define CS1_CONFIG 0x18 // Memory Module
#define CS2_CONFIG 0x00 // Ethernet
#define CS3_CONFIG 0x3C // LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#endif
#endif
#endif
    
    *(volatile cyg_uint32 *)MEMCFG1 = 
        (CS0_CONFIG << 0) |
        (CS1_CONFIG << 8) |
        (CS2_CONFIG << 16) |
        (CS3_CONFIG << 24);
    *(volatile cyg_uint32 *)MEMCFG2 = 
        (CS4_CONFIG << 0) |
        (CS5_CONFIG << 8) |
        (CS6_CONFIG << 16) |
        (CS7_CONFIG << 24);

    // Set up GPIO lines
#if defined(__DJ)
    *(volatile cyg_uint8 *)PADDR    = 0x00;  // PA0 - PA3 keyboard input
					     // PA4-7 unused

    *(volatile cyg_uint8 *)PADR     = 0x00;  // pull lines low


#elif defined(__DAR)
    *(volatile cyg_uint8 *)PADDR    = 0xc0;  // Keyboard data 0-5 input.
                                             // 6 - EL backlight enable
                                             // 7 - LCD glass supply enable
    *(volatile cyg_uint8 *)PADR    = 0x80;  // LCD off
#elif defined(__WINJAM)
    *(volatile cyg_uint8 *)PADDR    = 0xc0;  // PA0 - PA5 keyboard input
                                             // 6 - LCD
                                             // 7 - LCD
    *(volatile cyg_uint8 *)PADR     = 0x00;  // pull lines low
#else /* __DHARMA */
    *(volatile cyg_uint8 *)PADDR   = 0x8c;  // Keyboard data 0-1 input
                                             // Battery detect 2-3 input.  Set these as output
                                             // so that they don't generate a keyboard interrupt.
                                             // 4 - LCD (7312 external)
                                             // 5 - LCD (7312 external)
                                             // 6 - LCD/Video interface
                                             // 7 - LCD (7312 external)
    *(volatile cyg_uint8 *)PADR    = 0x80;  // LCD off
#endif

#if defined(__DJ)
    *(volatile cyg_uint8 *)PBDDR   = 0xFF; // everything out
    *(volatile cyg_uint8 *)PBDR    = 0x01; // codec reset high



#elif defined(__DAR)
    *(volatile cyg_uint8 *)PBDDR   = 0x3A;  // 0 - 
                                             // 1 - 
                                             // 2 - 
                                             // 3 - 
                                             // 4 - 
                                             // 5 - 
                                             // 6 - Rotary dial
                                             // 7 - Rotary dial
    *(volatile cyg_uint8 *)PBDR    = 0x00;  // Everything off
#elif defined(__WINJAM)
    *(volatile cyg_uint8 *)PBDDR   = 0x07;  // 0 - MMC0 CLK
                                             // 1 - MMC0 CMD
                                             // 2 - MMC0 DATA
                                             // 3 - MMC0 CARD
                                             // 4 - 2.5V bat detect
                                             // 5 - 2.0V bat detect
                                             // 6 - Board power control
                                             // 7 - MMC0 RESET
    *(volatile cyg_uint8 *)PBDR     = 0x06;  // lift CMD, dat
    *(volatile cyg_uint8 *)PBDDR    = 0x03;  // make dat an input
#else /* __DHARMA */
    *(volatile cyg_uint8 *)PBDDR   = 0xFA;  // 0 - (PRDY1) PCMCIA
                                             // 1 - (PRDY2) External mute / Pop control
                                             // 2 - Charge detect for Li-Ion battery circuit
                                             // 3 - LCD (7312 external)
                                             // 4 - Memory module, NAND Command Latch Enable
                                             // 5 - Memory module, NAND Address Latch Enable
                                             // 6 - Memory module, On-board NAND Select (active low)
                                             // 7 - Memory module, SmartMedia Card Enable (active low)
    *(volatile cyg_uint8 *)PBDR    = 0xc0;  // Everything off
#endif


#if defined(__DJ)
    *(volatile cyg_uint8 *)PDDDR   = 0x00;                                                                                      // 3 - Memory module, Dual MMC
                                             // 4 - Audio module, I2C Data
                                             // 5 - Audio module, I2C Clock
                                             // 6 - (SDQM0)
                                             // 7 - (SDQM1)
    *(volatile cyg_uint8 *)PDDR    = 0x00;  // Everything off
#elif defined(__WINJAM)
    *(volatile cyg_uint8 *)PDDDR   = 0xc8;  // 0 - Memory module, MMC1 CLK
                                             // 1 - Memory module, MMC1 CMD
                                             // 2 - Memory module, MMC1 DATA
                                             // 3 - Memory module, MMC1 CARD
                                             // 4 - Audio module, I2C Data
                                             // 5 - Audio module, I2C Clock
                                             // 6 - Power switch detect
                                             // 7 - Memory Module, MMC1 RESET
    *(volatile cyg_uint8 *)PDDR    = 0x06;  // lift CMD, dat
    *(volatile cyg_uint8 *)PDDDR   = 0xcc;  // make dat an input
#else /* __DAR || __DHARMA */
    *(volatile cyg_uint8 *)PDDDR   = 0x08;  // 0 - Memory module, Diagnostic LED control
                                             // 1 - Memory module, Dual MMC
                                             // 2 - Memory module, Dual MMC
                                             // 3 - Memory module, Dual MMC
                                             // 4 - Audio module, I2C Data
                                             // 5 - Audio module, I2C Clock
                                             // 6 - (SDQM0)
                                             // 7 - (SDQM1)
    *(volatile cyg_uint8 *)PDDR    = 0x36;  // Everything off
    *(volatile cyg_uint8 *)PDDDR   = 0x0c;  // make dat an input
#endif

#if defined(__DJ)
    *(volatile cyg_uint8 *)PEDDR   = 0x00;  // 0 - Audio module, Codec enable
                                             // 1 - unused (pulled high)
                                             // 2 - LCD chip select
    *(volatile cyg_uint8 *)PEDR    = 0x00;  // Everything off
#elif defined(__WINJAM)
    *(volatile cyg_uint8 *)PEDDR   = 0x07;  // 0 - Audio module, Codec enable
                                             // 1 - unused (pulled high)
                                             // 2 - LCD chip select
    *(volatile cyg_uint8 *)PEDR    = 0x00;  // Everything off
#else /* __DAR || __DHARMA */
    *(volatile cyg_uint8 *)PEDDR   = 0x07;  // 0 - Audio module, Codec enable
                                             // 1 - USB module, USB suspend control
                                             // 2 - LCD touch screen ADC enable
    *(volatile cyg_uint8 *)PEDR    = 0x00;  // Everything off
#endif

    // Initialize system control

#ifdef __DJ 
    *(volatile cyg_uint32 *)SYSCON2 = SYSCON2_KBD6;
#else
    // Keyboard only on PA0-1 on Dharma
    *(volatile cyg_uint32 *)SYSCON2 = SYSCON2_KBD6;
#endif


#if defined(__WINJAM)
    *(volatile cyg_uint32 *)SYSCON3 |= SYSCON3_ENPD67;
#endif

#else /* __EDB7209 */
    *(volatile cyg_uint32 *)MEMCFG1 = 
        (CS0_CONFIG << 0) |       // FLASH rom
        (CS1_CONFIG << 8) |       // NAND flash
        (CS2_CONFIG << 16) |      // Ethernet
        (CS3_CONFIG << 24);       // Parallel printer, keyboard, touch panel
    *(volatile cyg_uint32 *)MEMCFG2 = 
        (CS4_CONFIG << 0) |       // USB
        (CS5_CONFIG << 8) |       // Expansion
        (CS6_CONFIG << 16) |      // Local SRAM
        (CS7_CONFIG << 24);       // Boot ROM
    // This value came from Cirrus, but doesn't match the recommendations above?
    *(volatile cyg_uint32 *)MEMCFG1 = 0x3C001814;
    // Set up GPIO lines
    *(volatile cyg_uint8 *)PADDR   = 0x00;  // Keyboard data 0-7 input
    *(volatile cyg_uint8 *)PBDDR   = 0xFA;  // 0 - I/O on J22
                                            // 1 - RTS on UART1
                                            // 2 - Ring on UART1
                                            // 3 - SSI header, Pin 13
                                            // 4 - NAND Command Latch Enable
                                            // 5 - NAND Address Latch Enable
                                            // 6 - On-board NAND Select (active low)
                                            // 7 - SmartMedia Card Enable (active low)
    *(volatile cyg_uint8 *)PBDR    = 0xC0;  // Everything off
    *(volatile cyg_uint8 *)PDDDR   = 0x40;  // 0 - Diagnostic LED control
                                            // 1 - Enable DC-DC converter for LCD
                                            // 2 - Enable LCD
                                            // 3 - ENable LCD Backlight
                                            // 4 - CS4342 I2C Data
                                            // 5 - CS4342 I2C Clock
                                            // 6 - SmartMedia Presence indicator
                                            // 7 - I/O on J22
    *(volatile cyg_uint8 *)PDDR    = 0x00;  // Everything off
    *(volatile cyg_uint8 *)PEDDR   = 0x05;  // 0 - Codec or ADC/DAC
                                            // 1 - I/O on JP38 (0 when inserted)
                                            // 2 - Enable touch panel
    *(volatile cyg_uint8 *)PEDR    = 0x01;  // Enable audio (not CODEC)
#endif
#endif
#endif // 0
    
    *(volatile cyg_uint32 *)SYSCON2 = SYSCON2_KBD6;
    
    // Initialize system control
    *(volatile cyg_uint32 *)SYSCON2 |= SYSCON2_KBWEN;
    
    // Reset all interrupt masks (disable all interrupt sources)
    for (vector = CYGNUM_HAL_ISR_MIN;  vector < CYGNUM_HAL_ISR_COUNT;  vector++) {
        icr = (volatile cyg_uint32 *)hal_interrupt_clear_map[vector];
        if (icr) *icr = 0;  // Just a write clears the latch
    }

    // Perform custom initialization steps
    hal_edb7xxx_board_init();
    
#ifdef CYGHWR_HAL_ARM_EDB7XXX_BATLOW
    cyg_drv_interrupt_create(CYGNUM_HAL_INTERRUPT_BLINT,
                             99,                     // Priority - what goes here?
                             0,                      //  Data item passed to interrupt handler
                             cyg_hal_batlow_isr,
                             0,
                             &batlow_interrupt_handle,
                             &batlow_interrupt);
    cyg_drv_interrupt_attach(batlow_interrupt_handle);
    cyg_drv_interrupt_unmask(CYGNUM_HAL_INTERRUPT_BLINT);
#endif

    // Initialize real-time clock (for delays, etc, even if kernel doesn't use it)
    hal_clock_initialize(CYGNUM_HAL_RTC_PERIOD);

    // Set up eCos/ROM interfaces
    hal_if_init();

}

void
hal_edb7xxx_board_init( void )
{
#if !defined(__DHARMA) && !defined(__DHARMA_V2)
    // Turn on the DIAG LED to let the world know the board is alive
    // This uses PD0, which is used for other GPIO on Dharma
    *(volatile unsigned char *)LEDFLSH = LEDFLSH_ENABLE|LEDFLSH_DUTY(16)|LEDFLSH_PERIOD(1);
#endif
}

//
// This routine is called to respond to a hardware interrupt (IRQ).  It
// should interrogate the hardware and return the IRQ vector number.

// This code is a little convoluted to keep it general while still avoiding
// reading the hardware a lot, since the interrupt status is split across
// three separate registers.

int hal_spurious_ints;


#define HAL_INTERRUPT_PROFILE
#ifdef HAL_INTERRUPT_PROFILE
cyg_uint32 hal_int_counter[32];

void hal_reset_int_counter()
{
    int i;
    for( i = 0; i < 32; i++ ) {
        hal_int_counter[i] = 0;
    }
}
cyg_uint32* hal_get_int_counter()
{
    return hal_int_counter;
}

#endif

int hal_IRQ_handler(void)
{
    struct regmap *map = hal_interrupt_status_regmap;
    cyg_uint32 stat;
    int vector;
    while (map->first_int) {
        stat = *(volatile cyg_uint32 *)map->stat_reg & *(volatile cyg_uint32 *)map->mask_reg;
        for (vector = map->first_int;  vector <= map->last_int;  vector++) {
            if (stat & hal_interrupt_bitmap[vector])
            {
#ifdef HAL_INTERRUPT_PROFILE
                hal_int_counter[vector]++;
#endif
                return vector;
            }
        }
        map++;  // Next interrupt status register
    }
    hal_spurious_ints++;
    return CYGNUM_HAL_INTERRUPT_NONE; // This shouldn't happen!
}

//
// Interrupt control
//

void hal_interrupt_mask(int vector)
{
    volatile cyg_uint32 *imr;
#ifdef __DHARMA_V2
    if (vector == CYGNUM_HAL_INTERRUPT_XINT0) {
	CYG_ASSERT( 0, "Cannot mask expansion interrupts!" );
    }
#endif
    
    imr = (volatile cyg_uint32 *)hal_interrupt_mask_regmap[vector];
    *imr &= ~hal_interrupt_bitmap[vector];
}

void hal_interrupt_unmask(int vector)
{
    volatile cyg_uint32 *imr;
#ifdef __DHARMA_V2
    if (vector == CYGNUM_HAL_INTERRUPT_XINT0) {
	CYG_ASSERT( 0, "Cannot mask expansion interrupts!" );
    }
#endif
    
    imr = (volatile cyg_uint32 *)hal_interrupt_mask_regmap[vector];
    *imr |= hal_interrupt_bitmap[vector];
}

void hal_interrupt_acknowledge(int vector)
{
    // Some interrupt sources have a register for this.
    volatile cyg_uint8 *icr;
    icr = (volatile cyg_uint8 *)hal_interrupt_clear_map[vector];
    if (icr) {
        *icr = 0;  // Any data clears interrupt
    }
}

void hal_interrupt_configure(int vector, int level, int up)
{
    // No interrupts are configurable on this hardware
}

void hal_interrupt_set_level(int vector, int level)
{
    // No interrupts are configurable on this hardware
}

/*------------------------------------------------------------------------*/
// EOF hal_misc.c
