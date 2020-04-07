// mmc_proto.h: protocol layer for the mmc driver
// (c) Interactive Objects. This dadio OS is bigger than all of them.

#ifndef __MMC_PROTO_H__
#define __MMC_PROTO_H__

#include <cyg/kernel/kapi.h>
#include "mmc_drv.h"

#ifdef __cplusplus
extern "C" {
#endif
#if 0
}  // brace align
#endif

cyg_uint32 mmcReset( mmc_controller_info_t*, cyg_uint32 );
cyg_uint32 mmcIdentify( mmc_controller_info_t*, cyg_uint16 );
cyg_uint32 mmcSetStandbyState( mmc_controller_info_t*, cyg_uint16 );
cyg_uint32 mmcSetXferState( mmc_controller_info_t*, cyg_uint16 );
cyg_uint32 mmcGetCardIdentification( mmc_controller_info_t*,
				     cyg_uint16, cyg_uint8* );
cyg_uint32 mmcGetConfiguration( mmc_controller_info_t*,
				cyg_uint16, cyg_uint8* );
cyg_uint32 mmcConfigureBlockLength( mmc_controller_info_t*,
				     cyg_uint16, cyg_uint16 );
cyg_uint32 mmcRead( mmc_controller_info_t*, cyg_uint16,
		    mmc_block_request_t* );
cyg_uint32 mmcWrite( mmc_controller_info_t*, cyg_uint16,
		     mmc_block_request_t* );
cyg_uint32 mmcStopTransmission( mmc_controller_info_t* );
cyg_uint32 mmcReadMultiple( mmc_controller_info_t*, cyg_uint16,
			    mmc_block_request_t* );
cyg_uint32 mmcWriteMultiple( mmc_controller_info_t*, cyg_uint16,
			     mmc_block_request_t* );
cyg_uint32 mmcGetStatus( mmc_controller_info_t*, cyg_uint16 );
cyg_uint32 mmcInit( mmc_controller_info_t* );
cyg_uint32 mmcConfigDevice( mmc_controller_info_t* );
cyg_uint32 mmcCheckCardPresent( mmc_controller_info_t* );

#ifdef __cplusplus
}
#endif

#endif // __MMC_PROTO_H__
