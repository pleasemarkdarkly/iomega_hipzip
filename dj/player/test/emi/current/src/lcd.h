// LED/LCD EMI Stress Thread
//

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <devs/lcd/lcd.h>

void lcd_thread(cyg_uint32);
void ToggleLED();
void ToggleLCD();