// dfs.h: header for dataplay file system
// danc@iobjects.com 12/13/2000

#ifndef __DFS_H__
#define __DFS_H__

#include "dptypes.h"

#ifdef __cplusplus
extern "C" {
#endif

int dfs_commit( void );
DFSHANDLE dfs_mkdir( DFSHANDLE parentdir, const char* name );
DFSHANDLE dfs_createfile( DFSHANDLE dir, cyg_uint16 datatype, const char* name );
int dfs_delete( DFSHANDLE handle );
int dfs_get_attributes( DFSHANDLE handle, dfs_attributes_t* ret );
int dfs_get_file_info( DFSHANDLE handle, dfs_file_info_t* ret );
DFSHANDLE dfs_get_handle( DFSHANDLE searchdir, cyg_uint16 handle_opts, const char* name );
int dfs_get_media_info( dfs_media_info_t* ret );
int dfs_move( DFSHANDLE handle, DFSHANDLE newdir );
int dfs_read( DFSHANDLE file, char* buf, cyg_uint32 offset, cyg_uint32 count, cyg_uint16 options );
int dfs_rename( DFSHANDLE handle, const char* newname );
int dfs_set_attributes( DFSHANDLE handle, cyg_int16 attrib, int set_mask );
int dfs_write( DFSHANDLE file, const char* data, cyg_uint32 count );
int dfs_gfirst( DFSHANDLE dir, dfs_dirstate_t* pdirstate, dp_dir_entry_t* pdirent, cyg_uint16 options );
int dfs_gnext(dfs_dirstate_t* pdirstate, dp_dir_entry_t* pdirent);
int dfs_gdone(dfs_dirstate_t* pdirstate);


#ifdef __cplusplus
};
#endif

#endif // __DFS_H__
