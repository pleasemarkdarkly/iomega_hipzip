// dp.h: dataplay drive routines
// danc@iobjects.com 12/15/2000
// (c) Interactive Objects

#ifndef __DP_H__
#define __DP_H__

#include <devs/storage/dataplay/dpi_host.h>

#ifdef __cplusplus
extern "C" {
#endif

int dp_init( void );
    
int dp_get_device_info( DEVINFOSTRUCT* ret );
int dp_lock_media( void );
int dp_release_media( void );
int dp_set_parameters( cyg_uint16 packet_size, cyg_uint32 read_xfer_rate,
							  cyg_uint32 write_xfer_rate, cyg_uint16 spinup_current );
int dp_get_attention_info( unsigned char* ret);
int dp_eject_media( void );
int dp_power_control( cyg_uint16 power_state );


// status stuff
#define DP_POWER_DOWN  0x00
#define DP_POWER_IDLE  0x01
#define DP_POWER_READY 0x02
#define DP_POWER_CHECK 0x03
    
// Status
typedef struct dp_status_s {
  cyg_uint16  cached_write_data:1,    // engine has cached write data in buffer
    end_of_file:1,          // read past end of file
    media_status:1,         // 1 if media present, 0 if absent
    _reserved:13;           // reserved bits
  cyg_uint8   error_code;             // error code
  cyg_uint8	extended_bytes;         // number of bytes available to get_extended_status
} dp_status_t;

void dp_get_status( dp_status_t* );

int dp_get_last_error( void );
const char* dp_get_error( int dp_error_code );

#ifdef __cplusplus
};
#endif

#endif // __DP_H__
