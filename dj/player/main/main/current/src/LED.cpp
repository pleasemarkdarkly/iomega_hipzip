// LED.cpp: Control the DJ LED
// edwardm@iobjects.com 12/02/01
// (c) Interactive Objects

#include <main/main/LED.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <util/debug/debug.h>
#include <util/utils/utils.h>
#include <cyg/kernel/kapi.h>

DEBUG_MODULE_S( LED, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( LED );

USE_HW_LOCK(PBDR);

//! Turn off the LED.
void
LEDOff()
{
	HW_LOCK(PBDR);
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR &= 0xF3;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED off\n");
#endif  // __DJ
	HW_UNLOCK(PBDR);
}

//! Turn the LED red.
void
LEDRed()
{
	HW_LOCK(PBDR);
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR &= 0xF7;
    *(volatile cyg_uint8 *)PBDR |= 0x04;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED red\n");
#endif  // __DJ
	HW_UNLOCK(PBDR);
}

//! Turn the LED green.
void
LEDGreen()
{
	HW_LOCK(PBDR);
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR &= 0xFB;
    *(volatile cyg_uint8 *)PBDR |= 0x08;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED green\n");
#endif  // __DJ
	HW_UNLOCK(PBDR);
}

//! Turn the LED orange.
void
LEDOrange()
{
	HW_LOCK(PBDR);
#ifdef __DJ
    *(volatile cyg_uint8 *)PBDR |= 0x0C;
#else   // __DJ
    DEBUG(LED, DBGLEV_INFO, "LED orange\n");
#endif  // __DJ
	HW_UNLOCK(PBDR);
}

