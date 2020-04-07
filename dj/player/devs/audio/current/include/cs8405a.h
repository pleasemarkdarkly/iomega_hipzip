// cs8405a.h
// CS8405A SPDIF driver
// Teman Clark-Lindh
// 07/29/02
// (c) Fullplay Media

#ifndef CS8405A_H
#define CS8405A_H

#include <cyg/kernel/kapi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


void SPDIFEnable();
void SPDIFDisable();
void SPDIFSetSecurity(bool bCopy, bool bGen);
void SPDIFSetEmphasis(bool bEnabled);


#ifdef __cplusplus
};
#endif /* __cplusplus */
    
#endif /* CS8405A_H */
