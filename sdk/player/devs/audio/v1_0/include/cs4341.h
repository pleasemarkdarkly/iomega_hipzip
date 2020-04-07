#ifndef CS4341_H
#define CS4341_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" 
{
#endif /* __cplusplus */

#define DAC_MAX_VOLUME 0
#define DAC_MIN_VOLUME -90

void DACInit(void);
void DACSetVolume(cyg_int8 Volume);

#ifdef __cplusplus
};
#endif /* __cplusplus */
    
#endif /* CS4341_H */
