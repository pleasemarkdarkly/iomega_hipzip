// dfs.h: new header for dataplay file system
// temancl@fullplaymedia.com 01/07/02
// (c) Fullplay Media Systems

#ifndef __DFS_H__
#define __DFS_H__

#include <devs/storage/dataplay/dpi_host.h>

#ifdef __cplusplus
extern "C" {
#endif

int dfs_commit( void );
DFSHANDLE dfs_mkdir( DFSHANDLE parentdir, const char* name );
DFSHANDLE dfs_createfile( DFSHANDLE dir, cyg_uint16 datatype, const char* name );
int dfs_delete( DFSHANDLE handle );
int dfs_get_attributes( DFSHANDLE handle, GETATTRIBUTES_DATA* ret );
int dfs_get_file_info( DFSHANDLE handle, GETFILEINFO_DATA* ret );
DFSHANDLE dfs_get_handle( DFSHANDLE searchdir, cyg_uint16 handle_opts, const char* name );
int dfs_get_media_info( GETMEDIAINFO_DATA* ret );
int dfs_move( DFSHANDLE handle, DFSHANDLE newdir );
int dfs_read( DFSHANDLE file, char* buf, cyg_uint32 offset, cyg_uint32 count, cyg_uint16 options );
int dfs_rename( DFSHANDLE handle, const char* newname );
int dfs_set_attributes( DFSHANDLE handle, cyg_int16 attrib, int set_mask );
int dfs_write( DFSHANDLE file, const char* data, cyg_uint32 count );


#ifdef __cplusplus
};
#endif

#endif // __DFS_H__
