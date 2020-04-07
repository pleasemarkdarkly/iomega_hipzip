#ifndef CS4340_H
#define CS4340_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" 
{
#endif /* __cplusplus */

#define DAC_MAX_VOLUME 0
#define DAC_MIN_VOLUME 0

/* These are both dummy functions, the cs4340 provides no software control */
void DACEnable(void);
void DACDisable(void);
void DACSetVolume(cyg_int8 Volume);

#ifdef __cplusplus
};
#endif /* __cplusplus */
    
#endif /* CS4340_H */
