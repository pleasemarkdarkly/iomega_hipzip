#include "cs4342.h"
#include "i2c.h"
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

/* CS4342 */

#define CS4342_I2C_ADDRESS 0x20

/* Power and muting control */
#define PMCTL    0x01
#define PMCTL_CP_EN  (1<<0)
#define PMCTL_PDN    (1<<1)
#define PMCTL_PDNLN  (1<<2)
#define PMCTL_PDNHP  (1<<3)
#define PMCTL_POR    (1<<4)
#define PMCTL_SZC(n) (n<<5)
#define PMCTL_AMUTE  (1<<7)

/* Channel A analog headphone attentuation control */
#define HVOLA    0x02

/* Channel B analog headphone attentuation control */
#define HVOLB    0x03

/* Channel A digital volume control */
#define DVOLA    0x04

/* Channel B digital volume control */
#define DVOLB    0x05

/* Tone control */
#define TNCTL    0x06
#define TNCTL_TB(n) (n<<0)
#define TNCTL_BB(n) (n<<4)

/* Mode control */
#define MDCTL    0x07
#define MDCTL_VCBYP   (1<<0)
#define MDCTL_DEM(n)  (n<<1)
#define MDCTL_AB      (1<<3)
#define MDCTL_TBCF(n) (n<<4)
#define MDCTL_BBCF(n) (n<<6)

/* Limiter attack rate */
#define LMTRTTCK 0x08

/* Limiter release rate */
#define LMTRRLS  0x09

/* Volume and mixing control */
#define VMCTL    0x0a
#define VMCTL_ATAPI(n) (n<<0)
#define VMCTL_LIM_EN   (1<<4)
#define VMCTL_TC_EN    (1<<5)
#define VMCTL_TC(n)    (n<<6)

/* Mode control 2 */
#define MDCTL2   0x0b
#define MDCTL2_DIF(n)  (n<<0)
#define MDCTL2_LINE(n) (n<<5)
#define MDCTL2_MCLKDIV (1<<7)

extern unsigned int _DAISampleFrequency;

static bool _DACEnabled = false;
static cyg_int8 _DACVolume = 0;
static cyg_int8 _DACHeadphoneVolume = 0;
static cyg_int8 _DACBassBoost = 0;
static cyg_int8 _DACTrebleBoost = 0;
static TrebleBoostCornerFrequency_T _DACTrebleCorner = TBCF_7kHz;
static BassBoostCornerFrequency_T _DACBassCorner = BBCF_200Hz;

void
DACEnable(void)
{
    if (!_DACEnabled) {
	I2CInit();

	/* Configure DAC while PDN is set */
	I2CWrite(CS4342_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_PDN|PMCTL_CP_EN));
	I2CWrite(CS4342_I2C_ADDRESS, VMCTL, (VMCTL_TC_EN|VMCTL_LIM_EN|VMCTL_ATAPI(9)));
	I2CWrite(CS4342_I2C_ADDRESS, MDCTL2, (MDCTL2_DIF(2)));
	
	/* Start DAC power up sequence */
	I2CWrite(CS4342_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_CP_EN));
	
	/* Write volume and tone control settings */
	I2CWrite(CS4342_I2C_ADDRESS, DVOLA, _DACVolume);
	I2CWrite(CS4342_I2C_ADDRESS, HVOLA, _DACHeadphoneVolume);
	I2CWrite(CS4342_I2C_ADDRESS, TNCTL, (TNCTL_TB(_DACTrebleBoost)|TNCTL_BB(_DACBassBoost)));
	I2CWrite(CS4342_I2C_ADDRESS, MDCTL, (MDCTL_TBCF(_DACTrebleCorner)|MDCTL_BBCF(_DACBassCorner)|MDCTL_AB));
	
	/* Output won't begin for ~11000 samples, so wait for to minimize lost audio */
	/* TODO Still not happy with this, still losing initial audio data */
	cyg_thread_delay((11000 / _DAISampleFrequency) * 100);
	
	_DACEnabled = true;
    }
}

void
DACDisable(void)
{
    if (_DACEnabled) {
	I2CWrite(CS4342_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_PDN|PMCTL_CP_EN));
	_DACEnabled = false;
    }
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
	
    if (_DACEnabled) {
	I2CWrite(CS4342_I2C_ADDRESS, DVOLA, _DACVolume);
    }
}

cyg_int8
DACGetVolume(void)
{
    return _DACVolume;
}

void
DACSetHeadphoneVolume(cyg_int8 Volume)
{
    _DACHeadphoneVolume = Volume;
    if (_DACHeadphoneVolume > DAC_MAX_HEADPHONE_VOLUME) {
	_DACHeadphoneVolume = DAC_MAX_HEADPHONE_VOLUME;
    }
    if (_DACHeadphoneVolume < DAC_MIN_HEADPHONE_VOLUME) {
	_DACHeadphoneVolume = DAC_MIN_HEADPHONE_VOLUME;
    }
	
    if (_DACEnabled) {
	I2CWrite(CS4342_I2C_ADDRESS, HVOLA, _DACHeadphoneVolume);
    }    
}

cyg_int8
DACGetHeadphoneVolume(void)
{
    return _DACHeadphoneVolume;
}

void
DACSetTone(cyg_int8 TrebleBoost, cyg_int8 BassBoost)
{
    /* Clamp treble boost to supported ranges */
    _DACTrebleBoost = TrebleBoost;
    if (_DACTrebleBoost > DAC_MAX_TREBLE_BOOST) {
	_DACTrebleBoost = DAC_MAX_TREBLE_BOOST;
    }
    if (_DACTrebleBoost < DAC_MIN_TREBLE_BOOST) {
	_DACTrebleBoost = DAC_MIN_TREBLE_BOOST;
    }

    /* Clamp bass boost to supported ranges */
    _DACBassBoost = BassBoost;
    if (_DACBassBoost > DAC_MAX_BASS_BOOST) {
	_DACBassBoost = DAC_MAX_BASS_BOOST;
    }
    if (_DACBassBoost < DAC_MIN_BASS_BOOST) {
	_DACBassBoost = DAC_MIN_BASS_BOOST;
    }

    if (_DACEnabled) {
	I2CWrite(CS4342_I2C_ADDRESS, TNCTL, (TNCTL_TB(_DACTrebleBoost)|TNCTL_BB(_DACBassBoost)));
    }
}

void
DACGetTone(cyg_int8 * TrebleBoost, cyg_int8 * BassBoost)
{
    *TrebleBoost = _DACTrebleBoost;
    *BassBoost = _DACBassBoost;
}

void
DACSetToneCornerFrequency(TrebleBoostCornerFrequency_T TrebleCorner,
			  BassBoostCornerFrequency_T BassCorner)
{
    _DACTrebleCorner = TrebleCorner;
    _DACBassCorner = BassCorner;
    
    if (_DACEnabled) {
	I2CWrite(CS4342_I2C_ADDRESS, MDCTL, (MDCTL_TBCF(_DACTrebleCorner)|MDCTL_BBCF(_DACBassCorner)|MDCTL_AB));
    }
}

void
DACGetToneCornerFrequency(TrebleBoostCornerFrequency_T * TrebleCorner,
			  BassBoostCornerFrequency_T * BassCorner)
{
    *TrebleCorner = _DACTrebleCorner;
    *BassCorner = _DACBassCorner;
}
