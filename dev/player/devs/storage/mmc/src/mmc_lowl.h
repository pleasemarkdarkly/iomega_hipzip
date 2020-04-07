// mmc_lowl.h: header for hardware oriented routines
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. Mandrake, have you ever heard of a process called flouridation?

#ifndef __MMC_LOWL_H__
#define __MMC_LOWL_H__

#include "mmc_drv.h"

#ifdef __cplusplus
extern "C"  {
#endif // __cplusplus
#if 0
}   // brace align
#endif

// two supported clocking rates...:)
#define MMC_CLOCK_SLOW 0
#define MMC_CLOCK_FAST 1

// init and setup routines
void MMCSelectClock( mmc_controller_info_t*, cyg_uint8 );
void MMCHardwareInit( mmc_controller_info_t* );
void MMCResetController( mmc_controller_info_t* );
int  MMCBusStart( mmc_controller_info_t* );

// status routines
int  MMCCheckCardBusy( mmc_controller_info_t* );
int  MMCCardPresent( mmc_controller_info_t* );

// command and data transfer routines
__inline__ cyg_uint32 MMCReceiveData( mmc_controller_info_t*, mmc_block_request_t* );
__inline__ cyg_uint32 MMCSendData( mmc_controller_info_t*, mmc_block_request_t* );

cyg_uint32 MMCSendCommand( mmc_controller_info_t*, cyg_uint16, cyg_uint32, cyg_uint16 );
cyg_uint32 MMCGetResponse( mmc_controller_info_t*, cyg_uint16 );

#ifdef __cplusplus
};
#endif

#endif // __MMC_LOWL_H__
