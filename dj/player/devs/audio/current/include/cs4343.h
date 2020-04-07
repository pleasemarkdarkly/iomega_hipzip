#ifndef CS4343_H
#define CS4343_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define DAC_MAX_VOLUME 18
#define DAC_MIN_VOLUME -96

#define DAC_SUPPORT_HEADPHONE_VOLUME
#define DAC_MAX_HEADPHONE_VOLUME 0
#define DAC_MIN_HEADPHONE_VOLUME -25
    
#define DAC_BYPASS_72 0
#define DAC_BYPASS_93 1

#define DAC_SUPPORT_TONE_CONTROL
#define DAC_MAX_BASS_BOOST 12
#define DAC_MIN_BASS_BOOST 0
typedef enum {BBCF_50Hz = 0, BBCF_100Hz, BBCF_200Hz } BassBoostCornerFrequency_T;
    
#define DAC_MAX_TREBLE_BOOST 12
#define DAC_MIN_TREBLE_BOOST 0
typedef enum {TBCF_2kHz = 0, TBCF_4kHz, TBCF_7kHz } TrebleBoostCornerFrequency_T;
    
void DACEnable(void);
void DACDisable(void);
void DACSetVolume(cyg_int8 Volume);
void DACSetBypass(cyg_int8 Line);
cyg_int8 DACGetVolume(void);
void DACSetHeadphoneVolume(cyg_int8 Volume);
cyg_int8 DACGetHeadphoneVolume(void);
void DACSetTone(cyg_int8 TrebleBoost, cyg_int8 BassBoost);
void DACGetTone(cyg_int8 * TrebleBoost, cyg_int8 * BassBoost);
void DACSetToneCornerFrequency(TrebleBoostCornerFrequency_T TrebleCorner,
			       BassBoostCornerFrequency_T BassCorner);
void DACGetToneCornerFrequency(TrebleBoostCornerFrequency_T * TrebleCorner,
			       BassBoostCornerFrequency_T * BassCorner);
    
#ifdef __cplusplus
};
#endif /* __cplusplus */
    
#endif /* CS4343_H */
