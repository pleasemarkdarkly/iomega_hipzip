//====================================================================
//
//      dharma_flash.c
//
//      FLASH memory - Hardware support on IObjects Dharma
//
//====================================================================

#include <pkgconf/hal.h>

void
dharma_flash_enable(void *start, void *end)
{
    //    ipaq_EGPIO(SA1110_EIO_VPP, SA1110_EIO_VPP_ON);
}

void
dharma_flash_disable(void *start, void *end)
{
    //    ipaq_EGPIO(SA1110_EIO_VPP, SA1110_EIO_VPP_OFF);
}
