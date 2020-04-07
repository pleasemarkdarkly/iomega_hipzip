// __mmc_hw.h: internal header for hardware locations on MMC interface
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects, in da house.

// note that PDDDR is inverted, which screws up my nice clean header. sigh.

#ifndef __MMC_HW__
#define __MMC_HW__

#include <cyg/hal/hal_platform_ints.h>
#include <cyg/hal/hal_edb7xxx.h>

// configuration time hardware definitions
#include <devs/storage/mmc/_mmc_hw.h>

#define HW_MMC_CR_TIMEOUT  0x100    // timeout for card response (Ncr), min 2 max 64 clocks
//#define HW_MMC_CR_TIMEOUT  0x10000  // timeout for card response (Ncr), min 2 max 64 clocks
#define HW_MMC_AC_TIMEOUT  0x20000  // timeout for data ready, min 2 max TAAC + NSAC
// TAAC on sandisk cards is 1.5ms, at 13.5 ns tick that is roughly 116,000 ticks

#if 0
// hard reset lines
#ifdef __WINJAM
#define HW_MMC_HARD_RESET_PORT0  PDDR
#define HW_MMC_HARD_RESET_DDR0   PDDDR
#define HW_MMC_HARD_RESET_PIN0   0x80    // PD7

#define HW_MMC_HARD_RESET_PORT1  PBDR
#define HW_MMC_HARD_RESET_DDR1   PBDDR
#define HW_MMC_HARD_RESET_PIN1   0x80   // PB7

#else

// controller 0 reset line
#define HW_MMC_HARD_RESET_PORT0  PBDR
#define HW_MMC_HARD_RESET_DDR0   PBDDR
#define HW_MMC_HARD_RESET_PIN0   0x04    // PB2

// controller 1 reset line
#define HW_MMC_HARD_RESET_PORT1  PBDR
#define HW_MMC_HARD_RESET_DDR1   PBDDR
#define HW_MMC_HARD_RESET_PIN1   0x04    // PB2
#endif
#endif

#if (HW_MMC_HARD_RESET_PORT0 == HW_MMC_HARD_RESET_PORT1 && HW_MMC_HARD_RESET_PIN0 == HW_MMC_HARD_RESET_PIN1)
#define HW_MMC_SHARED_RESET
#warning "Media change does not always work with shared reset"
#endif

#if 0
// macros for controller 0 pin locations
#ifdef __WINJAM
#define HW_MMC_PORT0     PDDR
#define HW_MMC_DDR0      PDDDR
#define HW_MMC_CLK0      0x01    // PD0
#define HW_MMC_CMD0      0x02    // PD1
#define HW_MMC_DATA0     0x04    // PD2
#define HW_MMC_CARD0     0x08    // PD3

// macros for controller 1 pin locations
#define HW_MMC_PORT1     PBDR
#define HW_MMC_DDR1      PBDDR
#define HW_MMC_CLK1      0x01    // PB0
#define HW_MMC_CMD1      0x02    // PB1
#define HW_MMC_DATA1     0x04    // PB2
#define HW_MMC_CARD1     0x08    // PB3

#else

#define HW_MMC_PORT0     PBDR
#define HW_MMC_DDR0      PBDDR
#define HW_MMC_CLK0      0x10    // PB4
#define HW_MMC_CMD0      0x20    // PB5
#define HW_MMC_DATA0     0x40    // PB6
#define HW_MMC_CARD0     0x80    // PB7

// macros for controller 1 pin locations
#define HW_MMC_PORT1     PDDR
#define HW_MMC_DDR1      PDDDR
#define HW_MMC_CLK1      0x01    // PD0
#define HW_MMC_CMD1      0x02    // PD1
#define HW_MMC_DATA1     0x04    // PD2
#define HW_MMC_CARD1     0x08    // PD3
#endif
#endif

// internal macros
#ifndef ASM_HEADERS

#if __DHARMA == 2
#include <devs/mmgpio/mmgpio.h>
#endif

// generic macros to abstract the whole port interaction thing
#define MMC_TEST_PORT(port)      *(( volatile unsigned char*) port)
#define MMC_SET_BITS(port,val)   *(( volatile unsigned char*) port) |=   (unsigned char)(val)
#define MMC_CLEAR_BITS(port,val) *(( volatile unsigned char*) port) &= ~((unsigned char)(val))
#define MMC_SET_PORT(port,val)   *(( volatile unsigned char*) port) =    (unsigned char)(val)

// general macros for DDR fiddling
#define _DDR_PORT_INPUT(port, pin)        *(( volatile unsigned char*) port ) &= ~(pin)
#define _DDR_PORT_OUTPUT(port, pin)       *(( volatile unsigned char*) port ) |=  (pin)


// routines to open and close the reset pin on the DDR
// controller 0
#if HW_MMC_HARD_RESET_DDR0 == PDDDR
#define HW_MMC_OPEN_RESET0()  _DDR_PORT_INPUT(  HW_MMC_HARD_RESET_DDR0, HW_MMC_HARD_RESET_PIN0 )
#define HW_MMC_CLOSE_RESET0() _DDR_PORT_OUTPUT( HW_MMC_HARD_RESET_DDR0, HW_MMC_HARD_RESET_PIN0 )
#else
#define HW_MMC_OPEN_RESET0()  _DDR_PORT_OUTPUT( HW_MMC_HARD_RESET_DDR0, HW_MMC_HARD_RESET_PIN0 )
#define HW_MMC_CLOSE_RESET0() _DDR_PORT_INPUT(  HW_MMC_HARD_RESET_DDR0, HW_MMC_HARD_RESET_PIN0 )
#endif
// controller 1
#if HW_MMC_NUM_DRIVES == 2
#if HW_MMC_HARD_RESET_DDR1 == PDDDR
#define HW_MMC_OPEN_RESET1()   _DDR_PORT_INPUT(  HW_MMC_HARD_RESET_DDR1, HW_MMC_HARD_RESET_PIN1 )
#define HW_MMC_CLOSE_RESET1()  _DDR_PORT_OUTPUT( HW_MMC_HARD_RESET_DDR1, HW_MMC_HARD_RESET_PIN1 )
#else
#define HW_MMC_OPEN_RESET1()   _DDR_PORT_OUTPUT( HW_MMC_HARD_RESET_DDR1, HW_MMC_HARD_RESET_PIN1 )
#define HW_MMC_CLOSE_RESET1()  _DDR_PORT_INPUT(  HW_MMC_HARD_RESET_DDR1, HW_MMC_HARD_RESET_PIN1 )
#endif // HARD_RESET_DDR1 == PDDDR
#endif // NUM_DRIVES == 2

// routines to power off and on the cards
// controller 0
#if __DHARMA == 2
#define HW_MMC_POWER_OFF0() SetMMGPO(MMGPO_MMOD_VCC, 0x00)
#define HW_MMC_POWER_ON0() SetMMGPO(0x00, MMGPO_MMOD_VCC)
#else /* __DHARMA == 1 */
#define HW_MMC_POWER_OFF0()                  \
                   HW_MMC_OPEN_RESET0();     \
                   MMC_SET_BITS(   HW_MMC_HARD_RESET_PORT0, HW_MMC_HARD_RESET_PIN0 ); \
                   HW_MMC_CLOSE_RESET0()
  
#define HW_MMC_POWER_ON0()                   \
                   HW_MMC_OPEN_RESET0();     \
                   MMC_CLEAR_BITS( HW_MMC_HARD_RESET_PORT0, HW_MMC_HARD_RESET_PIN0 ); \
                   HW_MMC_CLOSE_RESET0()
#endif

// controller 1
#if HW_MMC_NUM_DRIVES == 2
#if __DHARMA == 2
#define HW_MMC_POWER_OFF1() HW_MMC_POWER_OFF0()
#define HW_MMC_POWER_ON1() HW_MMC_POWER_ON0()
#else /* __DHARMA == 1 */
#define HW_MMC_POWER_OFF1()                  \
                   HW_MMC_OPEN_RESET1();     \
                   MMC_SET_BITS( HW_MMC_HARD_RESET_PORT1, HW_MMC_HARD_RESET_PIN1 ); \
                   HW_MMC_CLOSE_RESET1()
#define HW_MMC_POWER_ON1()                   \
                   HW_MMC_OPEN_RESET1();     \
                   MMC_CLEAR_BITS( HW_MMC_HARD_RESET_PORT1, HW_MMC_HARD_RESET_PIN1 ); \
                   HW_MMC_CLOSE_RESET1()
#endif
#endif // NUM_DRIVES == 2

// macros to twiddle bits

// note that using this new method, i lose a bic on the cmd pin when it switches to input
// macros for controller 0
#if HW_MMC_DDR0 == PDDDR
#define MMC_DDR_DATA0_INPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_DATA0 )
#define MMC_DDR_DATA0_OUTPUT() _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_DATA0 )
#define MMC_DDR_CMD0_INPUT()   _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_CMD0  )
#define MMC_DDR_CMD0_OUTPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_CMD0  )
#define MMC_DDR_CLK0_INPUT()   _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_CLK0  )
#define MMC_DDR_CLK0_OUTPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_CLK0  )
#define MMC_DDR_CARD0_INPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_CARD0 )
#else
#define MMC_DDR_DATA0_INPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_DATA0 )
#define MMC_DDR_DATA0_OUTPUT() _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_DATA0 )
#define MMC_DDR_CMD0_INPUT()   _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_CMD0  )
#define MMC_DDR_CMD0_OUTPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_CMD0  )
#define MMC_DDR_CLK0_INPUT()   _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_CLK0  )
#define MMC_DDR_CLK0_OUTPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR0, HW_MMC_CLK0  )
#define MMC_DDR_CARD0_INPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR0, HW_MMC_CARD0 )
#endif // DDR0 == PDDDR

#if HW_MMC_NUM_DRIVES == 2
// macros for controller 1
#if HW_MMC_DDR1 == PDDDR
#define MMC_DDR_DATA1_INPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_DATA1 )
#define MMC_DDR_DATA1_OUTPUT() _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_DATA1 )
#define MMC_DDR_CMD1_INPUT()   _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_CMD1  )
#define MMC_DDR_CMD1_OUTPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_CMD1  )
#define MMC_DDR_CLK1_INPUT()   _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_CLK1  )
#define MMC_DDR_CLK1_OUTPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_CLK1  )
#define MMC_DDR_CARD1_INPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_CARD1 )
#else
#define MMC_DDR_DATA1_INPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_DATA1 )
#define MMC_DDR_DATA1_OUTPUT() _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_DATA1 )
#define MMC_DDR_CMD1_INPUT()   _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_CMD1  )
#define MMC_DDR_CMD1_OUTPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_CMD1  )
#define MMC_DDR_CLK1_INPUT()   _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_CLK1  )
#define MMC_DDR_CLK1_OUTPUT()  _DDR_PORT_OUTPUT( HW_MMC_DDR1, HW_MMC_CLK1  )
#define MMC_DDR_CARD1_INPUT()  _DDR_PORT_INPUT(  HW_MMC_DDR1, HW_MMC_CARD1 )
#endif // DDR1 = PDDDR
#endif // NUM_DRIVES == 2

#define MMC_TEST_PORT0() MMC_TEST_PORT( HW_MMC_PORT0 )
#define MMC_TEST_PORT1() MMC_TEST_PORT( HW_MMC_PORT1 )

// check to see if the given cards are present
#if __DHARMA == 2
#define MMC_TEST_CARD0_PRESENT() ( (GetMMGPI() & MMGPI_MMOD_CD0) == 0 )
#define MMC_TEST_CARD1_PRESENT() ( (GetMMGPI() & MMGPI_MMOD_CD1) == 0 )
#else /* __DHARMA == 1 */
#define MMC_TEST_CARD0_PRESENT() ( (MMC_TEST_PORT0() & HW_MMC_CARD0) == 0 )
#define MMC_TEST_CARD1_PRESENT() ( (MMC_TEST_PORT1() & HW_MMC_CARD1) == 0 )
#endif

#define MMC_READ_PORT(port,clock,tmp)                        \
                               tmp = MMC_TEST_PORT(port);    \
                               MMC_SET_PORT( port, (tmp) | clock );  \
                               tmp = MMC_TEST_PORT(port);    \
                               MMC_SET_PORT( port, (tmp) & ~clock )

// clock the cards and read up the port
#define MMC_READ_PORT0(tmp) MMC_READ_PORT(HW_MMC_PORT0, HW_MMC_CLK0, tmp)
#define MMC_READ_PORT1(tmp) MMC_READ_PORT(HW_MMC_PORT1, HW_MMC_CLK1, tmp)
#endif // ASM_HEADERS

#endif // __MMC_HW__







