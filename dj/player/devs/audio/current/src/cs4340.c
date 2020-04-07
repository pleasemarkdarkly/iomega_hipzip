#include "cs4340.h"
#include <cyg/kernel/kapi.h>

/* CS4340 */

void
DACEnable(void)
{
    /* Audio output begins approximately 10,000 LRCLK cycles after
     * it is activated.  So delay for that amount of time to minimize
     * audio loss. */
#if 1
    cyg_thread_delay(30);
#endif
    
    return;
}

void
DACDisable(void)
{
}

void
DACSetVolume(cyg_int8 Volume)
{
    return;
}
