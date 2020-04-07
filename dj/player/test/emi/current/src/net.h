// LED/LCD EMI Stress Thread
//

#include <cyg/infra/diag.h>
#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <devs/lcd/lcd.h>

extern "C"
{
void net_thread(cyg_uint32 ignored);
};
