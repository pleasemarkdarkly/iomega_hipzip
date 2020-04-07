#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

#include <devs/battery/battery.h>

#define AA_BATTERY_LEVEL_MASK 0xc0
#define AA_BATTERY_LEVEL_20V  0x40
#define AA_BATTERY_LEVEL_25V  0x80

int
BatteryGetLevel(void)
{
    cyg_uint32 Level;

    Level = *(volatile cyg_uint32 *)MMGPI;
    Level &= AA_BATTERY_LEVEL_MASK;
    switch (Level) {
	case 0x00:
	    /* Level < 2.0V */
	    return 0;

	case 0x40:
	    /* 2.0V < Level < 2.5V */
	    return 1;

	case 0xc0:
	    /* 2.5V < Level */
	    return 2;
	    
	default:
	    return -1;
	    break;
    }
}

int
BatteryGetChargeStatus(void)
{
    return 0;
}
