#ifndef __LCD_H__
#define __LCD_H__

#include <redboot.h>

#if CYGPKG_REDBOOT_DEMOJUMPER_LARGE_LCD
#include "g325e01r300.h"
#else
#include "g241d01r00.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
  
int LCDEnable(void);
int LCDDisable(void);

#define LCD_BACKLIGHT_OFF 0
#define LCD_BACKLIGHT_ON  1
int LCDSetBacklight(int Value);
    
int LCDClear(void);
int LCDPutPixel(unsigned char Color, int X, int Y);
int LCDWriteRaw(const unsigned char* buf, int len );
  
#ifdef __cplusplus
};
#endif

#endif // __LCD_H__
