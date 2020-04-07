// mmgpio.h: Header for memory mapped GPIO routines
// toddm@iobjects.com 08/29/01
// (c) Interactive Objects

#ifndef __MMGPIO_H__
#define __MMGPIO_H__

#include <pkgconf/system.h>
#include <cyg/infra/cyg_type.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define MMGPO_MMOD_VCC (1<<0)
#define MMGPO_PCMCIA_REG (1<<1) /* Low is attribute memory */
#define MMGPO_PCMCIA_8_16_SELECT (1<<2) /* Low is 16 bit access */
#define MMGPO_PCMCIA_IO_SELECT (1<<3) /* Active low */
#define MMGPO_LCD_POWER_ON (1<<4) /* Active low */
#define MMGPO_LCD_ENABLE (1<<5) /* Active high */

#define MMGPI_PCMCIA_VS1 (1<<0)
#define MMGPI_PCMCIA_VS2 (1<<1)
#define MMGPI_PCMCIA_CD (1<<4) /* Active low */
#define MMGPI_PCMCIA_RDY (1<<5) /* Active high */
#define MMGPI_MMOD_CD0 (1<<12)
#define MMGPI_MMOD_CD1 (1<<11)
    
cyg_uint32 GetMMGPI(void);
void SetMMGPO(cyg_uint32 SetBits, cyg_uint32 ClearBits);
    
#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /* __MMGPIO_H__ */
