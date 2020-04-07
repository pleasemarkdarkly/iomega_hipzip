#include <devs/audio/cs5332.h>
#include "i2c.h"
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

/* CS5332 */

#ifdef ENABLE_CS42L50
#define CS5332_I2C_ADDRESS 0x20
#else
#define CS5332_I2C_ADDRESS 0x22
#endif

/* I/O and power control */
#define IOPCTL 0x01
#define IOPCTL_CP_EN     (1<<0)
#define IOPCTL_PDN       (1<<1)
#define IOPCTL_AINMUX(n) (n<<4)
#define IOPCTL_BOOST     (1<<6)

/* Interface control */
#define ICTL 0x02
#define ICTL_DIF(n)   (n<<0)
#define ICTL_MASTER   (1<<3)
#define ICTL_RATIO(n) (n<<4)
#define ICTL_MCLKDIV  (1<<6)

/* Analog I/O control */
#define AIOCTL 0x03
#define AIOCTL_HPFREEZE (1<<0)
#define AIOCTL_LR       (1<<1)
#define AIOCTL_INDVC    (1<<2)
#define AIOCTL_ZC       (1<<4)
#define AIOCTL_SOFT     (1<<5)
#define AIOCTL_MUTER    (1<<6)
#define AIOCTL_MUTEL    (1<<7)

/* Left channel digital volume control */
#define DVOLA 0x04

/* Right channel digital volume control */
#define DVOLB 0x05

/* Right/Left Analog Gain control */
#define AGAIN 0x06

/* Clip Detection Status */
#define CLIPDETECT 0x07

/* Line in selection */


extern unsigned int _DAISampleFrequency;

static bool _ADCEnabled = false;
static cyg_int8 _ADCVolume = 0;
static cyg_int8 _ADCLeftVolume = 0;
static cyg_int8 _ADCRightVolume = 0;
static bool _ADCMicBoost = false;
static bool _ADCGainEnabled = false;
static bool _ADCStereoVolume = true;

/* Register shadows */
static cyg_int8 _ADCIOPCTL = IOPCTL_PDN;
static cyg_int8 _ADCICTL = 0;
static cyg_int8 _ADCAIOCTL = (AIOCTL_SOFT | AIOCTL_ZC);
static cyg_int8 _ADCGAIN = 0;


/* select input source - may not be totally relevant for non-Pogo HW */ 
/* make sure App calls mic boost, if you want it for 'MICIN' */
void
ADCSelectSource(int source)
{

  if (_ADCEnabled) {

	 // clear input config bits (there are 2)
  	_ADCIOPCTL &= ~IOPCTL_AINMUX(3);

	if(source == LINEIN)
	{
		_ADCIOPCTL |= IOPCTL_AINMUX(0);
	}
	else if(source == MICIN)
	{
		_ADCIOPCTL |= (IOPCTL_CP_EN | IOPCTL_AINMUX(2));
	}	

	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
  }

}

void
ADCEnable(void)
{
    if (!_ADCEnabled) {

	I2CInit();

	/* Enable the control port, set input to line 2 through the PGA */
#ifdef __POGO
	// pogo defaults to line in AIN_L1 + AIN_R1 (Line In, no mic boost)

	_ADCIOPCTL |= (IOPCTL_CP_EN | IOPCTL_AINMUX(0));
#else
	_ADCIOPCTL |= (IOPCTL_CP_EN | IOPCTL_AINMUX(2));
#endif

	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
	
	/* Set the data format to I2S */
	_ADCICTL |= (ICTL_DIF(0) | ICTL_RATIO(2));

	/* Set the data format to left justified */
    /*	_ADCICTL |= (ICTL_DIF(1) | ICTL_RATIO(2)); */



	I2CWrite(CS5332_I2C_ADDRESS, ICTL, _ADCICTL);
	/* Set the left channel and right channel volumes to be the same */
	/* TODO Right channel seems to be lower volume than left.  In Cirrus code, they ignore one of the
	   channels so you record mono.  To reproduce, use SoundForge to play a sine wave. */

	ADCDisableStereoVolume();

	/* Power on the ADC */
	_ADCIOPCTL &= ~IOPCTL_PDN;
	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
	_ADCEnabled = true;

	/* Explicitly reset clip bits (reading resets bits) */
	ADCGetClip();

	/* seperate gain control by default*/
	ADCEnableGain();
	ADCSetGain(0,0);
	
    }
}

void
ADCDisable(void)
{
    if (_ADCEnabled) {

	/* Power off the ADC */
	_ADCIOPCTL |= IOPCTL_PDN;
	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
	
	_ADCEnabled = false;
    }
}

void
ADCEnableMicBoost(void)
{
    _ADCIOPCTL |= IOPCTL_BOOST;
    _ADCMicBoost = true;
    if (_ADCEnabled) {
	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
    }
}

void
ADCDisableMicBoost(void)
{
    _ADCIOPCTL &= ~IOPCTL_BOOST;
    _ADCMicBoost = false;
    if (_ADCEnabled) {
	I2CWrite(CS5332_I2C_ADDRESS, IOPCTL, _ADCIOPCTL);
    }
}

void
ADCMute(int Mute) 
{
    if (Mute & MUTE_RIGHT) {
	_ADCAIOCTL |= AIOCTL_MUTER;
    }
    else {
	_ADCAIOCTL &= ~AIOCTL_MUTER;
    }
    if (Mute & MUTE_LEFT) {
	_ADCAIOCTL |= AIOCTL_MUTEL;
    }
    else {
	_ADCAIOCTL &= ~AIOCTL_MUTEL;
    }
    if (_ADCEnabled) {
	I2CWrite(CS5332_I2C_ADDRESS, AIOCTL, _ADCAIOCTL);
    }
}

void
ADCEnableStereoVolume()
{
  _ADCAIOCTL &= ~AIOCTL_LR;
  _ADCStereoVolume = true;
  if(_ADCEnabled) {
    I2CWrite(CS5332_I2C_ADDRESS, AIOCTL, _ADCAIOCTL);
    I2CWrite(CS5332_I2C_ADDRESS, DVOLA, _ADCLeftVolume);
    I2CWrite(CS5332_I2C_ADDRESS, DVOLB, _ADCRightVolume);

  }


}

void 
ADCDisableStereoVolume()
{
  _ADCAIOCTL |= AIOCTL_LR;
  _ADCStereoVolume = false;
  if(_ADCEnabled) {
    I2CWrite(CS5332_I2C_ADDRESS, AIOCTL, _ADCAIOCTL);
    I2CWrite(CS5332_I2C_ADDRESS, DVOLA, _ADCVolume);    
  }
}

/* stereo volume control */
void
ADCSetStereoVolume(cyg_int8 left, cyg_int8 right)
{
  _ADCLeftVolume = left;
  _ADCRightVolume = right;

  if(_ADCStereoVolume) {
      if (_ADCEnabled) {
	I2CWrite(CS5332_I2C_ADDRESS, DVOLA, _ADCLeftVolume);
	I2CWrite(CS5332_I2C_ADDRESS, DVOLB, _ADCRightVolume);
      }
  }
  
}

cyg_int8
ADCGetLeftVolume(void)
{
  return _ADCLeftVolume;
}

cyg_int8
ADCGetRightVolume(void)
{
  return _ADCLeftVolume;
}

/* mono volume control */
void
ADCSetVolume(cyg_int8 Volume)
{
    _ADCVolume = Volume; 
    if(!_ADCStereoVolume) {
      if (_ADCEnabled) {
	I2CWrite(CS5332_I2C_ADDRESS, DVOLA, _ADCVolume);
      }
    }
}


cyg_int8
ADCGetVolume(void)
{
    return _ADCVolume;
}


/* poll clip detection bits 
 clip bits are reset when read 
Use CLIP_L and CLIP_R to check bits */

cyg_uint8
ADCGetClip(void)
{
    cyg_uint8 bits = 0;

    if (_ADCEnabled) {
	bits = I2CRead(CS5332_I2C_ADDRESS, CLIPDETECT);
    }

    return bits;
}


/* ADC Analog Gain Control */

void ADCEnableGain()
{
  _ADCGainEnabled = true;
  // seperate gain control from digital volume
  _ADCAIOCTL |= AIOCTL_INDVC;
  if (_ADCEnabled) {
    I2CWrite(CS5332_I2C_ADDRESS, AIOCTL, _ADCAIOCTL);
  }

}


void ADCDisableGain()
{

  _ADCGainEnabled = false;
  // merge gain control with digital volume
  _ADCAIOCTL &= ~AIOCTL_INDVC;

  if(_ADCEnabled) {
    I2CWrite(CS5332_I2C_ADDRESS, AIOCTL, _ADCAIOCTL);
  }


}

/* left and right must be <= 12 */
// FIXME - error return format?
void ADCSetGain(cyg_int8 left, cyg_int8 right)
{

  if(((left & 0x0F) > ADC_MAX_GAIN) || ((right & 0x0f) > ADC_MAX_GAIN)) {
    return;
  }
  
  _ADCGAIN = ((left & 0x0F) << 4) | (right & 0x0F);

  if(_ADCEnabled) {
    I2CWrite(CS5332_I2C_ADDRESS,AGAIN,_ADCGAIN);  
  }
 

}


cyg_int8 ADCGetLeftGain()
{
  return ((_ADCGAIN & 0xF0) >> 4);
}


cyg_int8 ADCGetRightGain()
{
  return (_ADCGAIN & 0x0F);
}
