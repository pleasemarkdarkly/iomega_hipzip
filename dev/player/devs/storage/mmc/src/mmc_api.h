// mmc_api.h: header for the block access style api
// Dan Conti 02/20/01 danc@iobjects.com
// (c) Interactive Objects. Insert witty comment here.

#ifndef __MMC_API_H__
#define __MMC_API_H__

#include <cyg/kernel/kapidata.h>
#include "mmc_drv.h"

#ifdef __cplusplus
extern "C" 
{
#endif // __cplusplus
#if 0
}   // brace matching
#endif // 0

cyg_uint32 mmc_DriveOpen( mmc_controller_info_t* );
cyg_uint32 mmc_DriveClose( mmc_controller_info_t* );
cyg_uint32 mmc_DriveWrite( mmc_controller_info_t*, mmc_block_request_t* );
cyg_uint32 mmc_DriveRead( mmc_controller_info_t*, mmc_block_request_t* );
cyg_uint32 mmc_GetGeometry( mmc_controller_info_t*, drive_geometry_t* );
cyg_uint32 mmc_CardPresent( mmc_controller_info_t* );

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // __MMC_API_H__
