// mmc_util.h: general mmc utilities
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. We sexi baby. Oh yes, we sexi.

#ifndef __MMC_UTIL_H__
#define __MMC_UTIL_H__

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_diag.h>
#include "mmc_drv.h"

// generic interface to a pause
// the first is intended for thread delays, the second for busy waits during init
#define MMC_SYS_WAIT(x)       cyg_thread_delay(x)
#define MMC_SYS_BUSY_WAIT(x)  hal_delay_us( x*1000 )  //x*10000 )
//#define MMC_SYS_BUSY_WAIT(x) { int zzz; for(zzz=x*1000;zzz;zzz--) ; }

#ifdef __cplusplus
extern "C"  {
#endif
#if 0
}    // brace align
#endif

cyg_uint32 mmcCommandAndResponse( mmc_controller_info_t*, cyg_uint16,
				  cyg_uint32, cyg_uint16, cyg_uint16 );

#ifdef __cplusplus
}
#endif

#endif // __MMC_UTIL_H__
