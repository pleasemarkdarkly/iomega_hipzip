// dp_proto.c: routines to interace with the dataplay drive
// danc@iobjects.com 12/13/2000
// (c) Interactive Objects

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <assert.h>

#include <devs/storage/dataplay/dp.h>
#include <devs/storage/dataplay/dpi_host.h>
#include "dp_hw.h"


#define DEBUG(s...) diag_printf( ##s )

#define WORKING_STATUS_BITS

#ifndef WORKING_STATUS_BITS
#define dp_verify_phase( x ) (1)
#endif

static int dp_last_error_code = 0;

// utils

int dp_init( void ) 
{
    return dp_hw_init();
}


// ********************** dataplay commands *********************

int dp_get_extended_status(unsigned char *ret,cyg_uint16 size)
{
	dp_status_t status;
    char cmd_packet;

    cmd_packet = DPICMD_DEVICE_INFO;
	dp_send_cmd( &cmd_packet, 1 );
	if( dp_verify_phase( DP_READ_PHASE ) ) {
        dp_read_response( (char*)ret, size );
    }

	  dp_get_status( &status );

	  if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

	  return status.error_code;

}

int dp_get_device_info( DEVINFOSTRUCT* ret ) {
    dp_status_t status;
    char cmd_packet;

    cmd_packet = DPICMD_DEVICE_INFO;

    dp_send_cmd( &cmd_packet, 1 );

    if( dp_verify_phase( DP_READ_PHASE ) ) {
        dp_read_response( (char*)ret, DEVICEINFO_DATA_SIZE );
    }

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}


int dp_lock_media( void ) {
    dp_status_t status;
    char cmd_packet[2];

    cmd_packet[0] = DPICMD_LOCK_RELEASE_MEDIA;
    cmd_packet[1] = 0x01;    // lock the media

    dp_send_cmd( cmd_packet, 2 );

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}

int dp_release_media( void ) {
    dp_status_t status;
    char cmd_packet[2];

    cmd_packet[0] = DPICMD_LOCK_RELEASE_MEDIA;
    cmd_packet[1] = 0x00;    // release the media

    dp_send_cmd( cmd_packet, 2 );

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}


#define DP_PARAM_PSIZE     0x00
#define DP_PARAM_READXFER  0x01
#define DP_PARAM_WRITEXFER 0x02
#define DP_PARAM_SPINUP    0x03

int dp_set_parameters( cyg_uint16 packet_size, cyg_uint32 read_xfer_rate,
                       cyg_uint32 write_xfer_rate, cyg_uint16 spinup_current )
{

    dp_status_t status;
    char cmd_packet[17];
    int packet_len = 0;

    cmd_packet[packet_len] = DPICMD_SET_PARAMETERS;
    packet_len++;

    if( packet_size ) {
        cmd_packet[ packet_len ] = DP_PARAM_PSIZE;
        packet_len++;
        memcpy( (void*) &cmd_packet[ packet_len ], &packet_size, 2 );
        packet_len += 2;
    }
    if( read_xfer_rate ) {
        assert( read_xfer_rate >= 1 && read_xfer_rate <= 20000 );
        cmd_packet[ packet_len ] = DP_PARAM_READXFER;
        packet_len++;
        memcpy( (void*) &cmd_packet[ packet_len ], &read_xfer_rate, 4 );
        packet_len += 4;
    }
    if( write_xfer_rate ) {
        assert( write_xfer_rate >= 1 && write_xfer_rate <= 20000 );
        cmd_packet[ packet_len ] = DP_PARAM_WRITEXFER;
        packet_len++;
        memcpy( (void*) &cmd_packet[ packet_len ], &write_xfer_rate, 4 );
        packet_len += 4;
    }
    if( spinup_current ) {
        assert( spinup_current == 100 || spinup_current == 150 || spinup_current == 200 );
        cmd_packet[ packet_len ] = DP_PARAM_SPINUP;
        packet_len++;
        memcpy( (void*) &cmd_packet[ packet_len ], &spinup_current, 2 );
        packet_len += 2;
    }

    // make sure we actually set something
    assert( packet_len > 1 );

    dp_send_cmd( cmd_packet, packet_len );

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}

int dp_get_attention_info( unsigned char* ret) {
    dp_status_t status;
    char cmd_packet;

    cmd_packet = DPICMD_GET_ATTENTION_INFO;

    dp_send_cmd( &cmd_packet, 1 );
    if( dp_verify_phase( DP_READ_PHASE ) ) {
        dp_read_response( (char*)ret, 1 );
    }

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}


int dp_eject_media( void ) {
    dp_status_t status;
    char cmd_packet[2];

    cmd_packet[0] = DPICMD_EJECT_MEDIA;
    cmd_packet[1] = 0x00;  // eject media (0x80 forces a commit before eject)

    dp_send_cmd( cmd_packet, 2 );

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}

int dp_power_control( cyg_uint16 power_state ) {
    dp_status_t status;
    char cmd_packet[3];

    cmd_packet[0] = DPICMD_POWER_CONTROL;

    memcpy( (void*) &cmd_packet[1], &power_state, 2 );

    dp_send_cmd( cmd_packet, 3 );

    dp_get_status( &status );

    if( status.error_code  ) {
        DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
    }

    return status.error_code;
}

// after every command, status must be read off the drive
void dp_get_status( dp_status_t* ret) {

    if( dp_verify_phase( DP_STATUS_PHASE ) == 0 ) {
        DEBUG("%s (%d): Warning: phase error (%x, should be status)\n",
              __FUNCTION__, __LINE__, DP_READ_STATUS() );
    }

    dp_read_response( (char*)ret, sizeof( dp_status_t ) );

    dp_last_error_code = ret->error_code;
}

#define DP_ERROR_MESSAGES_BASE    0x00
static const char* dp_error_messages[] =
{
    "No Error",
    "An unknown interface command was attempted",
    "A media access command was attempted while there was no media present",
    "An invalid command packet size was received by the engine",
    "An invalid command packet parameter was received",
    "Media eject attempted while media locked",
    "Media is full, No writeable space available",
    "Illegal command received during write session",
    "Load command received with media already present"
};
#define DFS_ERROR_MESSAGES_BASE   0x10
static const char* dfs_error_messages[] =
{
    "DFS object access denied",
    "A write operation was attempted on an object with no write access",
    "A delete operation was attempted on an object with no delete access",
    "A rename operation was attempted on an object with no rename access",
    "A read operation was attempted on an object with no read access",
    "The DFSHANDLE given was invalid",
    "A DFS object of the same name already exists",
    "The input DFSNAME parameter was too long",
    "The object with the given input DFSNAME parameter was not found",
    "A fatal write error occurred while writing cached data to the media",
    "Illegal buffer size parameter in ReadDir",
    "A bad session ID was passed to ReadDir",
    "An illegal directory move operation was attempted",
    "Illegal attempt to delete a directory that is not empty",
};

// unknown error should always be the last message and should have an index of 0x00
static int other_msg_indices[] = { 0x50, 0xFF, 0x00 };
static const char* other_error_messages[] =
{
    "Media full error occurred while writing data",
    "General engine failure",
    "Unknown error",
};

int dp_get_last_error( void ) {
    return dp_last_error_code;
}

// take a dataplay error code and give back a const ptr to a descriptive message
const char* dp_get_error( int dp_error_code ) {
    int i;
    if( dp_error_code <= DP_ERROR_MESSAGES_BASE + sizeof( dp_error_messages ) ) {
        return dp_error_messages[ dp_error_code ];
    }
    if( dp_error_code <= DFS_ERROR_MESSAGES_BASE + sizeof( dfs_error_messages ) )  {
        return dfs_error_messages[ dp_error_code ];
    }
    for( i = 0; other_msg_indices[i]; i++ ) {
        if( other_msg_indices[i] == dp_error_code ) {
            return other_error_messages[ i ];
        }
    }
    return other_error_messages[ i ];
}

