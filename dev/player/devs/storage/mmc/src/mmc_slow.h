// mmc_slow.h: prototypes for the slower mmc routines
// Dan Conti 02/21/01 danc@iobjects.com
// (c) Interactive Objects.com

#ifndef __MMC_SLOW_H__
#define __MMC_SLOW_H__

#include <cyg/kernel/ktypes.h>

#ifdef __cplusplus
extern "C" {
#endif
#if 0
} // brace align
#endif

// routines for controller 0
cyg_uint32  MMCExchangeDataSlow_c0( cyg_uint8 odata, cyg_uint8* idata );
cyg_uint32  MMCGetResponseSlow_c0( cyg_uint8* dBuf, cyg_uint16 dataLength );
void        MMCSendCommandSlow_c0(  cyg_uint32 Arg,  cyg_uint16 Cmd, cyg_uint16 crcData );

// routines for controller 1
cyg_uint32  MMCExchangeDataSlow_c1( cyg_uint8 odata, cyg_uint8* idata );
cyg_uint32  MMCGetResponseSlow_c1( cyg_uint8* dBuf, cyg_uint16 dataLength );
void        MMCSendCommandSlow_c1(  cyg_uint32 Arg,  cyg_uint16 Cmd, cyg_uint16 crcData );

#ifdef __cplusplus
};
#endif

#endif // __MMC_SLOW_H__
