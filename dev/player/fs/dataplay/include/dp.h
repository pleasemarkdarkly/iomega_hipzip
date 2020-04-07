// dp.h: dataplay drive routines
// danc@iobjects.com 12/15/2000

#ifndef __DP_H__
#define __DP_H__

#include "dptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int dp_get_device_info( dp_device_info_t* ret );
int dp_lock_media( void );
int dp_release_media( void );
int dp_set_parameters( cyg_uint16 packet_size, cyg_uint32 read_xfer_rate,
							  cyg_uint32 write_xfer_rate, cyg_uint16 spinup_current );
int dp_get_attention_info( dp_attention_info_t* ret);
int dp_load_media( void );
int dp_eject_media( void );
int dp_power_control( cyg_uint16 power_state );

void dp_get_status( dp_status_t* );

int dp_get_last_error( void );
const char* dp_get_error( int dp_error_code );

#ifdef __cplusplus
};
#endif

#endif // __DP_H__