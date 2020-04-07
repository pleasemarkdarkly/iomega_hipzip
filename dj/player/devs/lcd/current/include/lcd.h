#ifndef __LCD_H__
#define __LCD_H__

#include <devs/lcd/_lcd_hw.h>

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
int LCDWriteInverted1to4(const unsigned char* buf, int len );
int LCDWriteInverted(const unsigned char* buf, int len );
  
#ifdef __cplusplus
};
#endif

#endif // __LCD_H__
