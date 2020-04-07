#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

#include <devs/battery/battery.h>

#define LI_ION_BATTERY_LEVEL_MASK 0x300
#define LI_ION_BATTERY_LEVEL_38V  0x100
#define LI_ION_BATTERY_LEVEL_36V  0x200

#define LI_ION_BATTERY_CHARGE_MASK 0x400

int
BatteryGetLevel(void)
{
    cyg_uint32 Level;

    Level = *(volatile cyg_uint32 *)MMGPI;
    Level &= LI_ION_BATTERY_LEVEL_MASK;
    switch (Level) {
	case 0x000:
	    /* Level < 3.6V */
	    return 0;

	case 0x200:
	    /* 3.6V < Level < 3.8V */
	    return 1;

	case 0x300:
	    /* 3.8V < Level */
	    return 2;

	default:
	    return -1;
	    break;
    }
}

int
BatteryGetChargeStatus(void)
{
    cyg_uint32 Status;

    Status = *(volatile cyg_uint32 *)MMGPI;
    Status &= LI_ION_BATTERY_CHARGE_MASK;
    if (Status) {
	return 1;
    }
    else {
	return 0;
    }
}

	
