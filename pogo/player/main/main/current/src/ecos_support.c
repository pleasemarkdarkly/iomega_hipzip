// ecos_support.c: board initialization
// (c) interactive objects

#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/kernel/kapi.h>

// Pogo was using the default settings for dharma
#define CS0_CONFIG 0x54 // FLASH ROM
    //#define CS0_CONFIG 0x14
#define CS1_CONFIG 0x00 // Ethernet
#define CS2_CONFIG 0x18 // Memory Module
#define CS3_CONFIG 0x3C // IO Expansion / PCMCIA / LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM

void hal_edb7xxx_board_init( void ) 
{
    // Init memcfg
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
    
    // Init ports
    *(volatile cyg_uint8 *)PADDR   = 0x8c;  // Keyboard data 0-1 input
                                             // Battery detect 2-3 input.  Set these as output
                                             // so that they don't generate a keyboard interrupt.
                                             // 4 - LCD (7312 external)
                                             // 5 - LCD (7312 external)
                                             // 6 - LCD/Video interface
                                             // 7 - LCD (7312 external)
    *(volatile cyg_uint8 *)PADR    = 0x80;  // LCD off
    *(volatile cyg_uint8 *)PBDDR   = 0xFA;  // 0 - (PRDY1) PCMCIA
                                             // 1 - (PRDY2) External mute / Pop control
                                             // 2 - Charge detect for Li-Ion battery circuit
                                             // 3 - LCD (7312 external)
                                             // 4 - Memory module, NAND Command Latch Enable
                                             // 5 - Memory module, NAND Address Latch Enable
                                             // 6 - Memory module, On-board NAND Select (active low)
                                             // 7 - Memory module, SmartMedia Card Enable (active low)
    *(volatile cyg_uint8 *)PBDR    = 0xc0;  // Everything off
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
    *(volatile cyg_uint8 *)PEDDR   = 0x07;  // 0 - Audio module, Codec enable
                                             // 1 - USB module, USB suspend control
                                             // 2 - LCD touch screen ADC enable
    *(volatile cyg_uint8 *)PEDR    = 0x00;  // Everything off
}
