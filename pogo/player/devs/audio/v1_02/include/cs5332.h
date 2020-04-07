#ifndef CS5332_H
#define CS5332_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    void ADCEnable(void);
    void ADCDisable(void);    

    /* Adds a +20dB digital gain to the input */
    void ADCEnableMicBoost(void);
    void ADCDisableMicBoost(void);

#define CLIP_L 0x1
#define CLIP_R 0x2
    cyg_uint8 ADCGetClip(void);

#define MUTE_DISABLE 0x0
#define MUTE_RIGHT   0x1
#define MUTE_LEFT    0x2
    void ADCMute(int Mute);

#define ADC_MAX_VOLUME 12
#define ADC_MIN_VOLUME -96

#define LINEIN 0x01
#define MICIN 0x02

  /* mono calls (default is mono control) */
  void ADCSetVolume(cyg_int8 Volume);
  cyg_int8 ADCGetVolume(void);

  /* stereo calls */
  void ADCEnableStereoVolume();
  void ADCDisableStereoVolume();
  void ADCSetStereoVolume(cyg_int8 left, cyg_int8 right);
  cyg_int8 ADCGetLeftVolume(void);
  cyg_int8 ADCGetRightVolume(void);

#define ADC_MAX_GAIN 12
#define ADC_MIN_GAIN 0
    void ADCEnableGain();
    void ADCDisableGain();
    void ADCSetGain(cyg_int8 left,cyg_int8 right);
    cyg_int8 ADCGetLeftGain();
    cyg_int8 ADCGetRightGain();

   void ADCSelectSource(int source);

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* CS5332_H */
