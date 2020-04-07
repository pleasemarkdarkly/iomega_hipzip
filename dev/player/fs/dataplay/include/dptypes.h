// dptypes.h: types for the dataplay drive
// danc@iobjects.com 12/13/2000

// dataplay drives use big endian

#ifndef __DPTYPES_H__
#define __DPTYPES_H__

#include <cyg/kernel/kapi.h>

#define DP_MAXNAME  512     //  maximum length for a name
#define DFS_DIR_BUFSIZE 256

// ****************  General    ***************

typedef int DFSHANDLE;

// directory entry attributes
typedef struct dp_file_attrib_s {
	cyg_uint16 att_type:3;		// 001 for files, 000 for directories
	cyg_uint16 hidden:1;		// 1 = hidden.
	cyg_uint16 reserved:4;		
	cyg_uint16 att_read:1;		// 0 = error on read
	cyg_uint16 att_write:1;		// 0 = error on write
	cyg_uint16 att_rename:1;	// 0 = error on rename
	cyg_uint16 att_delete:1;	// 0 = error on delete
	cyg_uint16 reserved2:4;
} dp_file_attrib_t;

// internal, for parsing the device response
typedef struct dp_dir_entry_resp_s {
	cyg_uint16 entry_length;	// byte length of this entry
	dp_file_attrib_t attrib;	// file attributes
	cyg_int32 handle;			// handle to the directory
	cyg_uint16 name_length;		// number of bytes in length to follow
	unsigned char name0;		// first byte of name, useful for addr
} dp_dir_entry_resp_t;	

// the information passed to the client layer
typedef struct dp_dir_entry_s {
	dp_file_attrib_t attrib;	// file attributes
	cyg_int32 handle;			// handle to the entry
	cyg_uint16 name_length;		// number of bytes in length to follow
	char* name;					// filename
} dp_dir_entry_t;

// the internal view of the state of a directory traversal
typedef struct dfs_dirstate_int_s {
	short session;
	char buf[DFS_DIR_BUFSIZE];
	short num_entries;
	dp_dir_entry_resp_t* next_entry;
	cyg_uint16 options;
	DFSHANDLE dir;
} dfs_dirstate_int_t;

#define DIRSTATE_SIZE sizeof(dfs_dirstate_int_t)

// the external view of a directory traversal state blob
typedef struct dfs_dirstate_s {
	char state[DIRSTATE_SIZE];
} dfs_dirstate_t;

// readdir options
typedef struct dp_readdir_options_s {
	cyg_uint16 entry_count:4;
	cyg_uint16 filter_by_attr:1;
	cyg_uint16 filter_by_mimetype:1;
	cyg_uint16 restart_session:1;
	cyg_uint16 close_session:1;
	cyg_uint16 filter_by_partialmime:1;
	cyg_uint16 use_obj_metadata:1;
	cyg_uint16 reserved:6;
} dp_readdir_options_t;

// ****************  Responses  ***************

// Status
typedef struct dp_status_s {
	cyg_uint16  cached_write_data:1,    // engine has cached write data in buffer
end_of_file:1,          // read past end of file
media_status:1,         // 1 if media present, 0 if absent
_reserved:13;           // reserved bits
	cyg_uint8   error_code;             // error code
	cyg_uint8	extended_bytes;         // number of bytes available to get_extended_status
} dp_status_t;

// response for Device Information
typedef struct dp_device_info_s {
	cyg_uint8   device_type;        // device type value, always 0x01		
	cyg_uint8   _reserved[3];       // reserved data
	char        device_id[20];      // device id string
	char        firmware_rev[8];    // firmware revision string
	cyg_uint16  packet_size;        // current packet size
	cyg_uint16  max_packet_size;    // max packet size
	char        cmd_set_rev[2];     // command set revision
	cyg_uint16  spinup_current;     // spin up current limit
	cyg_uint32  host_read_rate;     // host read transfer rate (sustained)
	cyg_uint32  host_write_rate;    // host write transfer rate (sustained)
	cyg_uint32  timestamp;          // current time in seconds from 1/1/1970
} dp_device_info_t;

// response for Get Attention Information
typedef struct dp_attention_info_s {
	cyg_uint8   media_inserted:1,   // attention irq caused by media insertion.. ?
_unspecified:7;     // TODO documentation does not explain this
	cyg_uint8   _reserved;          // TODO this space presumed reserved, not clear
} dp_attention_info_t;

// response for Get Attributes
typedef struct dfs_attributes_s {
	cyg_int16   attrib;             // attributes for the given descriptor
	cyg_int16   attrib_mask;        // attributes modification mask for object
} dfs_attributes_t;

// response for Get File Info
typedef struct dfs_file_info_s {
	cyg_int16  attrib;             // attributes for the file
	cyg_int16  file_format_type;   // file format type
	cyg_int32  time_last_modified; // last modified date/time
	cyg_uint64 file_size;          // size of the file
} dfs_file_info_t;

// response for Get Handle
typedef struct dp_handle_s {
	cyg_int32  handle;             // handle to the object found
	cyg_int16  attrib;             // attributes of that object
} dp_handle_t;

// response for Get Media Info
typedef struct dfs_media_info_s {
	cyg_int32   handle;                  // handle to root directory
	cyg_uint32  _reserved;               // reserved
	cyg_uint64  total_size;              // total size of the media ( i think ) TODO
	cyg_uint64  write_size;              // writeable bytes left
	cyg_uint16  name_length;             // number of bytes in the root dir name
	unsigned char name[DP_MAXNAME];      // name of the root directory
} dfs_media_info_t;

// response for Read Dir
typedef struct dp_read_dir_s {
	cyg_uint16       restarted:1,      // current directory restarted due to modification in dir
end_of_list:1,    // current block of data hods final entry
_reserved:14;     // reserved
	cyg_uint16       num_entries;      // number of entries in this block
	cyg_uint32       readdir_session;  // read dir session id
	char				entry00;		 // first byte of first entry, useful for address.
	//  dp_dir_entry_t** dir_entry_list;   // directory entry list
} dp_read_dir_t;


#endif // __DPTYPES_H__
