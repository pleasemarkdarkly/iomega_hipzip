// ecos_support.c: board initialization code
// (c) interactive objects

#include <cyg/hal/hal_edb7xxx.h>
#include <util/utils/utils.h>

#ifdef __DJ
#warning DJ CS config
#define CS0_CONFIG 0x54 // FLASH ROM - 16 bit
#define CS1_CONFIG 0x00 // Disabled  
#define CS2_CONFIG 0x00 // Ethernet (now CS2) 16-bit, slow
#define CS3_CONFIG 0x00 // Disabled
#define CS4_CONFIG 0x03 // 0x1b // DJ LCD // fixed for duppy 04/01/02
#define CS5_CONFIG 0x00 // IDE 16 bit, slow
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#elif defined(__DHARMA_V2)
#warning Dharma 2 CS config
#define CS0_CONFIG 0x54 // FLASH ROM
#define CS1_CONFIG 0x00 // Ethernet
#define CS2_CONFIG 0x18 // Memory Module
#define CS3_CONFIG 0x3C // IO Expansion / PCMCIA / LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#else   // __DHARMA=1 presumably
#warning Dharma 1 config
// These wait states may need to be adjusted on v1 for 90mhz support
#define CS0_CONFIG 0x54 // FLASH ROM
#define CS1_CONFIG 0x18 // Memory Module
#define CS2_CONFIG 0x00 // Ethernet
#define CS3_CONFIG 0x3C // LCD
#define CS4_CONFIG 0x02 // 0x1a // USB
#define CS5_CONFIG 0x00 // IDE
#define CS6_CONFIG 0x00 // Local SRAM
#define CS7_CONFIG 0x00 // Boot ROM
#endif  // __DJ

char RedBoot_version[] = "\nFullPlay Media Systems Dharma OS\n\n";

DECLARE_HW_LOCK(PBDR);
DECLARE_HW_LOCK(PDDR);

void hal_edb7xxx_board_init( void ) 
{
    // Init memcfg
    *(volatile unsigned int*)MEMCFG1 =
        (CS0_CONFIG << 0) |
        (CS1_CONFIG << 8) |
        (CS2_CONFIG << 16) |
        (CS3_CONFIG << 24);
    *(volatile unsigned int *)MEMCFG2 = 
        (CS4_CONFIG << 0) |
        (CS5_CONFIG << 8) |
        (CS6_CONFIG << 16) |
        (CS7_CONFIG << 24);

    // Init ports
#ifdef __DJ
	INIT_HW_LOCK(PBDR);
	INIT_HW_LOCK(PDDR);
    *(volatile unsigned char *)PADDR    = 0x00;  // PA0 - PA3 keyboard input
                                                 // PA4-7 unused
    *(volatile unsigned char *)PADR     = 0x00;  // pull lines low

    *(volatile unsigned char *)PBDDR   = 0x1F;   // PB1-PB4 output
	*(volatile unsigned char *)PBDR    = 0x02;   // codec reset high, backlight off, leds off


    *(volatile unsigned char *)PDDDR   = 0x08; // most outputs (note reverse semantics for PDDDR)
                                               // PD0, system reset when high
                                               // PD1, hd reset when low
                                               // PD2, mute when high ('on' for startup)

    *(volatile unsigned char *)PDDR    = 0x06; // PD1 high,  PD2 high (muted)
/*    
	// unused by software , don't initialize unneccesarily
    *(volatile unsigned char *)PEDDR   = 0x00;  // 0 - Audio module, Codec enable
                                                // 1 - unused (pulled high)
                                                // 2 - LCD chip select
    *(volatile unsigned char *)PEDR    = 0x00;  // Everything off
*/
    
#else // __DHARMA=1 presumably
    *(volatile unsigned char *)PADDR   = 0x8c;  // Keyboard data 0-1 input
                                             // Battery detect 2-3 input.  Set these as output
                                             // so that they don't generate a keyboard interrupt.
                                             // 4 - LCD (7312 external)
                                             // 5 - LCD (7312 external)
                                             // 6 - LCD/Video interface
                                             // 7 - LCD (7312 external)
    *(volatile unsigned char *)PADR    = 0x80;  // LCD off
    *(volatile unsigned char *)PBDDR   = 0xFA;  // 0 - (PRDY1) PCMCIA
                                             // 1 - (PRDY2) External mute / Pop control
                                             // 2 - Charge detect for Li-Ion battery circuit
                                             // 3 - LCD (7312 external)
                                             // 4 - Memory module, NAND Command Latch Enable
                                             // 5 - Memory module, NAND Address Latch Enable
                                             // 6 - Memory module, On-board NAND Select (active low)
                                             // 7 - Memory module, SmartMedia Card Enable (active low)
    *(volatile unsigned char *)PBDR    = 0xc2;  // Everything off except backlight
    *(volatile unsigned char *)PDDDR   = 0x08;  // 0 - Memory module, Diagnostic LED control
                                             // 1 - Memory module, Dual MMC
                                             // 2 - Memory module, Dual MMC
                                             // 3 - Memory module, Dual MMC
                                             // 4 - Audio module, I2C Data
                                             // 5 - Audio module, I2C Clock
                                             // 6 - (SDQM0)
                                             // 7 - (SDQM1)
    *(volatile unsigned char *)PDDR    = 0x36;  // Everything off
    *(volatile unsigned char *)PDDDR   = 0x0c;  // make dat an input
    *(volatile unsigned char *)PEDDR   = 0x07;  // 0 - Audio module, Codec enable
                                             // 1 - USB module, USB suspend control
                                             // 2 - LCD touch screen ADC enable
    *(volatile unsigned char *)PEDR    = 0x00;  // Everything off
#endif
}
