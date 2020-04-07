#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

#include <devs/audio/cs4343.h>
#include "i2c.h"


/* CS4343 */

#ifdef ENABLE_CS42L50
#define CS4343_I2C_ADDRESS 0x22
#else
#define CS4343_I2C_ADDRESS 0x20
#endif

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
    
#if defined(__IOMEGA_32)
        /* Take the DAC out of reset */
        *(volatile cyg_uint8 *)PEDDR |= 0x02;
        *(volatile cyg_uint8 *)PEDR |= 0x02;
#endif /* __IOMEGA_32 */
	
        I2CInit();	
        /* Configure DAC while PDN is set */
        I2CWrite(CS4343_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_PDN|PMCTL_CP_EN));
#if defined(DISABLE_VOLUME_CONTROL)
#warning volume control disabled
		// dac in left-justified mode is the correct setting
        I2CWrite(CS4343_I2C_ADDRESS, MDCTL2, (MDCTL2_LINE(DAC_BYPASS_72)|MDCTL2_DIF(2)));
#else
        I2CWrite(CS4343_I2C_ADDRESS, VMCTL, (VMCTL_TC_EN|VMCTL_LIM_EN|VMCTL_ATAPI(9)));
        I2CWrite(CS4343_I2C_ADDRESS, MDCTL2, (MDCTL2_DIF(2)));
#endif /* DISABLE_VOLUME_CONTROL */
	
        /* Start DAC power up sequence */
        I2CWrite(CS4343_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_CP_EN));
	
        /* Write volume and tone control settings */
        I2CWrite(CS4343_I2C_ADDRESS, DVOLA, _DACVolume);
        I2CWrite(CS4343_I2C_ADDRESS, HVOLA, _DACHeadphoneVolume);
        I2CWrite(CS4343_I2C_ADDRESS, TNCTL, (TNCTL_TB(_DACTrebleBoost)|TNCTL_BB(_DACBassBoost)));
#if defined(DISABLE_VOLUME_CONTROL)
        I2CWrite(CS4343_I2C_ADDRESS, MDCTL, (MDCTL_VCBYP|MDCTL_AB));
#else
        I2CWrite(CS4343_I2C_ADDRESS, MDCTL, (MDCTL_BBCF(_DACBassCorner)|MDCTL_TBCF(_DACTrebleCorner)|MDCTL_AB));
#endif /* DISABLE_VOLUME_CONTROL */

	
        /* Output won't begin for ~11000 samples, so wait for to minimize lost audio */
        /* TODO Still losing initial audio data */
        cyg_thread_delay( (11000 * 100 ) / _DAISampleFrequency );


#if defined(__DJ)
        // disable mute, let 42l50 handle it from here		
		*(volatile cyg_uint8 *)PDDR &= ~0x04;	
#elif defined(__IOMEGA_32)
        /* Disable headphone mute */
        *(volatile cyg_uint8 *)PBDDR |= 0x40;
        *(volatile cyg_uint8 *)PBDR |= 0x40;
#elif defined(__POGO)
        /* Disable headphone mute */
        *(volatile cyg_uint8 *)PEDDR |= 0x02;
        *(volatile cyg_uint8 *)PEDR |= 0x02;
#elif defined(__DHARMA) || defined(__DHARMA_V2)

        /* Disable headphone mute */
        *(volatile cyg_uint8 *)PBDDR |= 0x02;
        *(volatile cyg_uint8 *)PBDR |= 0x02;
#endif /* __DHARMA */

	
        _DACEnabled = true;
    }
}


void
DACDisable(void)
{
    if (_DACEnabled) {

#if defined(__DJ)
        /* this has to be here, since __DHARMA is defined for all
         * products based on dharma
         */
#elif defined(__POGO)
        /* Enable headphone mute */
        *(volatile cyg_uint8 *)PEDDR |= 0x02;
        *(volatile cyg_uint8 *)PEDR &= ~0x02;
#elif defined(__DHARMA) || defined(__DHARMA_V2)
        /* Enable headphone mute */
        *(volatile cyg_uint8 *)PBDDR |= 0x02;
        *(volatile cyg_uint8 *)PBDR &= ~0x02;
#elif defined(__IOMEGA_32)
        /* Enable headphone mute */
        *(volatile cyg_uint8 *)PBDDR |= 0x40;
        *(volatile cyg_uint8 *)PBDR &= ~0x40;
#endif /* __IOMEGA_32 */
	
        /* Start DAC power down sequence */
        I2CWrite(CS4343_I2C_ADDRESS, PMCTL, (PMCTL_AMUTE|PMCTL_SZC(2)|PMCTL_POR|PMCTL_PDN|PMCTL_CP_EN));

        /* Need to wait a bit to discharge to prevent pop */
        cyg_thread_delay(40);
	
        _DACEnabled = false;
    }
}


// set bypass level
void
DACSetBypass(cyg_int8 Line)
{
	I2CWrite(CS4343_I2C_ADDRESS, MDCTL2, (MDCTL2_LINE(Line)|MDCTL2_DIF(2)));
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
        I2CWrite(CS4343_I2C_ADDRESS, DVOLA, _DACVolume);
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
        I2CWrite(CS4343_I2C_ADDRESS, HVOLA, _DACHeadphoneVolume);
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
        I2CWrite(CS4343_I2C_ADDRESS, TNCTL, (TNCTL_TB(_DACTrebleBoost)|TNCTL_BB(_DACBassBoost)));
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
        I2CWrite(CS4343_I2C_ADDRESS, MDCTL, (MDCTL_TBCF(_DACTrebleCorner)|MDCTL_BBCF(_DACBassCorner)|MDCTL_AB));
    }
}

void
DACGetToneCornerFrequency(TrebleBoostCornerFrequency_T * TrebleCorner,
                          BassBoostCornerFrequency_T * BassCorner)
{
    *TrebleCorner = _DACTrebleCorner;
    *BassCorner = _DACBassCorner;
}


