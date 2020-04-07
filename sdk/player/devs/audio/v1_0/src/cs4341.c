#include "cs4341.h"
#include "i2c.h"
#include <cyg/kernel/kapi.h>

/* CS4341 */

#define CS4341_I2C_ADDRESS 0x22
#define MODE_CONTROL_REG   0x01
#define VOL_MIX_REG        0x02
#define A_VOL_REG          0x03
#define B_VOL_REG          0x04

#define AMUTE              0x80
#define DIF_1              0x01
#define DIF_2              0x02
#define DEM_44KHZ          0x04
#define POR                0x02
#define PDN                0x01

#define AB                 0x80
#define SOFT               0x40
#define ZERO               0x20
#define ATAPI_LR           0x09

#define MUTE_VOL           0x80

static cyg_int8 _DACVolume = 0;
static bool _DACInitialized = false;

void
DACInit(void)
{
    I2CInit();
    
    I2CWrite(CS4341_I2C_ADDRESS, MODE_CONTROL_REG, (AMUTE | DIF_2 | DEM_44KHZ | POR));
    I2CWrite(CS4341_I2C_ADDRESS, VOL_MIX_REG, (AB | SOFT | ATAPI_LR));
    I2CWrite(CS4341_I2C_ADDRESS, A_VOL_REG, -_DACVolume);
    I2CWrite(CS4341_I2C_ADDRESS, B_VOL_REG, 0x00);

    /* Output won't begin for 10000 LRCK cycles, so wait
     * approximately .25s to minimize lost audio */
    /* TODO Compute the actual delay */
    cyg_thread_delay(25);

    _DACInitialized = true;
}

void
DACSetVolume(cyg_int8 Volume)
{
    _DACVolume = Volume;
    if (_DACVolume > DAC_MAX_VOLUME) {
	_DACVolume = DAC_MAX_VOLUME;
    }
    if (_DACVolume < DAC_MIN_VOLUME) {
	_DACVolume = DAC_MIN_VOLUME;
    }
	
    if (_DACInitialized) {
	I2CWrite(CS4341_I2C_ADDRESS, A_VOL_REG, -_DACVolume);
    }
}
