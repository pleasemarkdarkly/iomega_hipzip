// dfs.c: file system implementation for dataplay
// danc@iobjects.com 12/15/2000

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <util/debug/debug.h>
#include <stdlib.h>

#include <devs/storage/dataplay/dP.h>      // our own functions
#include <devs/storage/dataplay/dfs.h>      // our own functions
#include "dp_hw.h"    // for dp_send_cmd

#define DEBUG(s...) diag_printf( ##s )
#define WORKING_STATUS_BITS

#ifndef WORKING_STATUS_BITS
#define dp_verify_phase( x ) 1
#endif

// file i/o commands
int dfs_commit( void ) {
	dp_status_t status;
	char cmd_packet[2];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_COMMIT;

	dp_send_cmd( cmd_packet, 2 );

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n",
			__FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

DFSHANDLE dfs_mkdir( DFSHANDLE parentdir, const char* name ) {
	dp_status_t status;
	DFSHANDLE handle;
	cyg_uint16 len = strlen( name );
	char* cmd_packet = (char*) malloc( len + 10 );

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_CREATEDIR;
	memcpy( (void*) &cmd_packet[4], &parentdir, 4 );
	memcpy( (void*) &cmd_packet[8], &len, 2 );
	memcpy( (void*) &cmd_packet[10], name, len );

	dp_send_cmd( cmd_packet, len+10 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*)&handle, 4 );
	}

	dp_get_status ( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n",
			__FUNCTION__, __LINE__, status.error_code );
		handle = -1;
	}

	return handle;
}

DFSHANDLE dfs_createfile( DFSHANDLE dir, cyg_uint16 datatype, const char* name ) {
	dp_status_t status;
	DFSHANDLE handle;
	cyg_uint16 len = strlen( name );
	char* cmd_packet = (char*) malloc( len + 10 );

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_CREATEFILE;
	memcpy( (void*) &cmd_packet[2], &datatype, 2 );
	memcpy( (void*) &cmd_packet[4], &dir, 4 );
	memcpy( (void*) &cmd_packet[8], &len, 2 );
	memcpy( (void*) &cmd_packet[10], name, len );

	dp_send_cmd( cmd_packet, len+10 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*)&handle, 4 );
	}

	dp_get_status ( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n",
			__FUNCTION__, __LINE__, status.error_code );
		handle = -1;
	}

	return handle;
}

int dfs_delete( DFSHANDLE handle ) {
	dp_status_t status;
	char cmd_packet[8];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_DELETE;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );

	dp_send_cmd( cmd_packet, 8 );

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_get_attributes( DFSHANDLE handle, GETATTRIBUTES_DATA* ret ) {
	dp_status_t status;
	char cmd_packet[8];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETATTR;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );

	dp_send_cmd( cmd_packet, 8 );

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_get_file_info( DFSHANDLE handle, GETFILEINFO_DATA* ret ) {
	dp_status_t status;
	char cmd_packet[8];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETFILEINFO;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );

	dp_send_cmd( cmd_packet, 8 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*)ret, GETFILEINFO_DATA_SIZE);
	}

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

DFSHANDLE dfs_get_handle( DFSHANDLE searchdir, cyg_uint16 handle_opts, const char* name ) {
	dp_status_t status;
	GETHANDLE_DATA h;
	cyg_uint16 len = strlen(name);
	char* cmd_packet = (char*) malloc( len + 10 );

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETHANDLE;
	memcpy( (void*) &cmd_packet[2], &handle_opts, 2 );
	memcpy( (void*) &cmd_packet[4], &searchdir, 4 );
	memcpy( (void*) &cmd_packet[8], &len, 2 );
	memcpy( (void*) &cmd_packet[10], name, len );

	dp_send_cmd( cmd_packet, len+10 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*) &h, GETHANDLE_DATA_SIZE );
	}

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	free( cmd_packet );

	return h.DfsHandle;
}

int dfs_get_media_info( GETMEDIAINFO_DATA* ret ) {
	dp_status_t status;
	char cmd_packet[2];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETMEDIAINFO;

	dp_send_cmd( cmd_packet, 2 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*) ret, GETMEDIAINFO_DATA_SIZE_NO_NAME );
	}

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_move( DFSHANDLE handle, DFSHANDLE newdir ) {
	dp_status_t status;
	char cmd_packet[12];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_MOVE;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );
	memcpy( (void*) &cmd_packet[8], &newdir, 4 );

	dp_send_cmd( cmd_packet, 12 );
	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_read( DFSHANDLE file, char* buf, cyg_uint32 offset, cyg_uint32 count, cyg_uint16 options ) {
	dp_status_t status;
	char cmd_packet[24];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_READFILE;
	memcpy( (void*) &cmd_packet[2], &options, 2 );
	memcpy( (void*) &cmd_packet[4], &file, 4 );
	memcpy( (void*) &cmd_packet[8], &offset, 4 );
	memset( (void*) &cmd_packet[12], 0, 4 );
	memcpy( (void*) &cmd_packet[16], &count, 4 );
	memset( (void*) &cmd_packet[20], 0, 4 );

	dp_send_cmd( cmd_packet, 24 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_data( buf, count );
	}

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_rename( DFSHANDLE handle, const char* newname ) {
	dp_status_t status;
	int len = strlen( newname );
	char* cmd_packet = (char*) malloc( len + 10 );

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_RENAME;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );
	memcpy( (void*) &cmd_packet[8], &len, 2 );
	memcpy( (void*) &cmd_packet[10], newname, len );

	dp_send_cmd( cmd_packet, len+10 );

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	free( cmd_packet );

	return status.error_code;
}

int dfs_set_attributes( DFSHANDLE handle, cyg_int16 attrib, int set_mask ) {
	dp_status_t status;
	char cmd_packet[10];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_SETATTR;
	memcpy( (void*) &cmd_packet[2], &set_mask, 2 );
	memcpy( (void*) &cmd_packet[4], &handle, 4 );
	memcpy( (void*) &cmd_packet[8], &attrib, 2 );

	dp_send_cmd( cmd_packet, 10 );

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_write( DFSHANDLE file, const char* data, cyg_uint32 count ) {
	dp_status_t status;
	char cmd_packet[16];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_WRITEAPPEND;
	cmd_packet[2] = cmd_packet[3] = 0;

	memcpy( (void*) &cmd_packet[4], &file, 4 );
	memcpy( (void*) &cmd_packet[8], &count, 4 );
	memset( (void*) &cmd_packet[12], 0, 4 );

	dp_send_cmd( cmd_packet, 16 );

	if( dp_verify_phase( DP_WRITE_PHASE ) ) {
		dp_write_data( data, count );
	}

	dp_get_status( &status );

	if( status.error_code  ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}


