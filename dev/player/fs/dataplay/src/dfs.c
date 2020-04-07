// dfs.c: file system implementation for dataplay
// danc@iobjects.com 12/15/2000

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>

#include <stdlib.h>
#include <fs/dataplay/dfs.h>      // our own functions
#include <fs/dataplay/dptypes.h>  // types in structures returned
#include <fs/dataplay/dpcodes.h>  // command codes
#include <fs/dataplay/dp_hw.h>    // for dp_send_cmd
#include <fs/dataplay/dp.h>       // for dp_get_status

#define DEBUG(s...) diag_printf(##s)

//#define WORKING_STATUS_BITS

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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_get_attributes( DFSHANDLE handle, dfs_attributes_t* ret ) {
	dp_status_t status;
	char cmd_packet[8];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETATTR;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );

	dp_send_cmd( cmd_packet, 8 );

	dp_get_status( &status );

	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_get_file_info( DFSHANDLE handle, dfs_file_info_t* ret ) {
	dp_status_t status;
	char cmd_packet[8];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETFILEINFO;
	memcpy( (void*) &cmd_packet[4], &handle, 4 );

	dp_send_cmd( cmd_packet, 8 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*)ret, sizeof( dfs_file_info_t ) );
	}

	dp_get_status( &status );

	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

DFSHANDLE dfs_get_handle( DFSHANDLE searchdir, cyg_uint16 handle_opts, const char* name ) {
	dp_status_t status;
	dp_handle_t h;
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
		dp_read_response( (char*) &h, sizeof( dp_handle_t ) );
	}

	dp_get_status( &status );

	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	free( cmd_packet );

	return h.handle;
}

int dfs_get_media_info( dfs_media_info_t* ret ) {
	dp_status_t status;
	char cmd_packet[2];

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_GETMEDIAINFO;

	dp_send_cmd( cmd_packet, 2 );

	if( dp_verify_phase( DP_READ_PHASE ) ) {
		dp_read_response( (char*) ret, sizeof( dfs_media_info_t ) );
	}

	dp_get_status( &status );

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
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

	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	return status.error_code;
}

int dfs_readdir( dfs_dirstate_int_t* pstate) {
	dp_status_t status;
	char cmd_packet[24];
	int count = DFS_DIR_BUFSIZE;
	dp_read_dir_t* p_resp;

	cmd_packet[0] = DPICMD_DFS_COMMAND;
	cmd_packet[1] = DFSCMD_READDIR;
	memcpy( (void*) &cmd_packet[2], &pstate->options, 2 );	// Options associated with returning the directory entries.
	memcpy( (void*) &cmd_packet[4], &pstate->dir, 4 );		// Handle of the directory in which to get the list.
	memcpy( (void*) &cmd_packet[8], &pstate->session, 4 );// The ReadDir ID associated with the current session (0 for new session).
	memset( (void*) &cmd_packet[12], 0, 2 );		// Maximum number of bytes to transfer to host (0 for automatic).
	
	dp_send_cmd( cmd_packet, 14 );
	
	if( dp_verify_phase( DP_READ_PHASE ) ) 
		dp_read_data( pstate->buf, count );
	
	dp_get_status( &status );
	if( status.error_code != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: get_status gave back error code of %d\n", __FUNCTION__, __LINE__, status.error_code );
	}

	p_resp = (dp_read_dir_t*) pstate->buf;
	if (p_resp) {
		diag_printf("read dir response looks like: restarted %d, end %d, session %d, #entries %d\n",p_resp->restarted, p_resp->end_of_list, p_resp->readdir_session,p_resp->num_entries);
	}
	
	pstate->next_entry = (dp_dir_entry_resp_t*)&p_resp->entry00;
	pstate->num_entries = p_resp->num_entries;

	return status.error_code;
}

void get_next_entry(dfs_dirstate_int_t* pstate, dp_dir_entry_t* pdirent)
{
	dp_dir_entry_resp_t* p_resp_entry = pstate->next_entry;

	pdirent->attrib = p_resp_entry->attrib;
	pdirent->handle = p_resp_entry->handle;
	pdirent->name = &p_resp_entry->name0;	// pass out the pointer into the dirstate
	pdirent->name_length = p_resp_entry->name_length;
	pdirent->attrib = p_resp_entry->attrib;

	pstate->next_entry = (dp_dir_entry_resp_t*)p_resp_entry + p_resp_entry->entry_length;
	--(pstate->num_entries);
	
	if (pdirent) {
		diag_printf("first entry looks like: attrib %d, handle %d, namelen %d\n",pdirent->attrib,pdirent->handle,pdirent->name_length);
	}
}

// retrieve the first entry in the supplied directory
int dfs_gfirst( DFSHANDLE dir, dfs_dirstate_t* pdirstate, dp_dir_entry_t* pdirent, cyg_uint16 options ) {
	dp_status_t status;
	int ret;
	dfs_dirstate_int_t* pstate = (dfs_dirstate_int_t*)pdirstate;
	
	pstate->session = 0;
	pstate->dir = dir;
	
	ret = dfs_readdir(pstate);
	if( ret != DP_NO_ERROR ) {
		DEBUG("%s (%d): Warning: dfs_readdir gave back error code of %d\n", __FUNCTION__, __LINE__, ret);
	}
	else
	{	
		get_next_entry(pstate, pdirent);
	}
	
	return status.error_code;
}

// retrieve the next directory entry in the session
int dfs_gnext(dfs_dirstate_t* pdirstate, dp_dir_entry_t* pdirent) {
	dfs_dirstate_int_t* pstate = (dfs_dirstate_int_t*)pdirstate;
	
	if (!pstate->num_entries) {
		dp_read_dir_t* p_readdir_resp = (dp_read_dir_t*) pstate->buf;
		if (p_readdir_resp->end_of_list)
			return DFS_GENERAL;
		dfs_readdir(pstate);
	}

	get_next_entry(pstate, pdirent);

	return DP_NO_ERROR;
}

// close the session
int dfs_gdone(dfs_dirstate_t* pdirstate) {
	dfs_dirstate_int_t* pstate = (dfs_dirstate_int_t*)pdirstate;
	dp_readdir_options_t* opts = (dp_readdir_options_t*)&pstate->options;
	int ret;

	opts->close_session = 1;

	ret = dfs_readdir( pstate );
	if (ret != DP_NO_ERROR) {
		DEBUG("%s (%d): Error closing session of %d\n",__FUNCTION__,__LINE__, ret);
	}
	
	return ret;
}
