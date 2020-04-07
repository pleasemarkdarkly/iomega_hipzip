// dpi_host.h
// dharma dataplay support header
// temancl@fullplaymedia.com 01/07/02

// changelog:
// 01/07/02 Compiler definitions

/*-----------------------------------------------------------------------------
File:		dpi_host.h
Part of:	DataPlay Host Interface Module
Summary:	Include file for interface commands usable by host systems.
Notes:	All structures in this file assume a 2-byte alignment for structures.
Copyright, DataPlay, Inc. 2000-2001. All rights reserved.
$Revision: 95 $
$Log: /Engine/Header/dpi_host.h $
 * 
 * 95    11/19/01 9:20a Llee
 * Fixed GET_OBJECT_METADATA_DATA_ALL_FIXED to include new extended
 * metadata values.
 * 
 * 94    11/16/01 2:46p Llee
 * Rolled revision to 17.
 * 
 * 93    11/15/01 4:41p Llee
 * Added extended options to GetObjectMetadata.
 * 
 * 92    11/14/01 2:32p Tfeldman
 * Added FILE_UNLIMITED_GENERIC_CHECK_OUT_COPIES to ContentKey state bit
 * definitions.
 * 
 * 91    11/09/01 8:58a Llee
 * Added options to Commit command packet.
 * Added CollectionContentId and DataBlockSize to GetMediaMetadata command
 * packet.
 * 
 * 90    10/31/01 4:08p Mgurkowski
 * Changed Engine Serial to Engine number.  This field is not the
 * serial number, but actually an ASCII numbering
 * 
 * 89    10/31/01 11:55a Tfeldman
 * Added engine serial number size symbol.
 * 
 * 88    10/05/01 4:47p Llee
 * Added 3 bytes to MAX_CMD_PACKET_LENGTH to account for worst case
 * CKCMD_CREATE_FILE packet.
 * 
 * 87    10/03/01 6:17p Llee
 * Removed obsolete items and rolled to revision 16.
 * 
 * 86    10/02/01 12:57p Llee
 * Changed SET_USER_METADATA_SIZE_NO_NAME to
 * SET_USER_METADATA_SIZE_NO_METADATA
 * 
 * 85    10/02/01 9:19a Mgurkowski
 * Removed Setting Current Time in Set Device Parms
 * 
 * 84    10/02/01 8:27a Llee
 * Changed DFSMIME to 2 bytes of length rather than 1. Fixed
 * SET_USER_METADATA definitions.
 * 
 * 83    10/01/01 1:45p Llee
 * Added SET_USER_METADATA and GET_USER_METADATA structures.
 * 
 * 82    10/01/01 12:36p Llee
 * Temp: Added back DFS_ATT_READ and DFS_ATT_READ_CHILD for build.
 * 
 * 81    10/01/01 10:31a Llee
 * Removed DFS_ATT_READ and DFS_ATT_READ_CHILD.
 * 
 * 80    9/27/01 12:32p Tfeldman
 * Added Lockable to the CkState bits.
 * 
 * 79    9/13/01 11:44a Llee
 * Added read-child-list permission attribute and create-denied and
 * read-child-list-denied errors.
 * 
 * 78    9/06/01 11:03a Mgurkowski
 * Added DFS_INCOMPLETE to the status phase bits.
 * 
 * 77    9/06/01 10:46a Llee
 * Added DFS_ATT_READ_CHILDREN permission.
 * 
 * 76    9/05/01 5:43p Mgurkowski
 * Changed command packet size to use MAX_CMD_PACKET_LENGTH
 * 
 * 75    9/05/01 5:35p Llee
 * Added MAX_PACKET_LENGTH and DFS_ATT_CREATE
 * 
 * 74    8/17/01 1:10p Llee
 * Rolled COMMAND_REV to 15.
 * 
 * 73    8/17/01 12:52p Mgurkowski
 * Changed I_SYSTEM_FAILURE to I_UNRECOVERABLE
 * 
 * 72    8/17/01 12:17p Llee
 * Moved ContentKey error codes to dpi_ckhost.h
 * 
 * 71    8/16/01 9:01a Llee
 * Added DFSCMD_WRITEMODIFY.
 * 
 * 70    8/13/01 11:09a Mgurkowski
 * Moved Extended Status structure typedef to dpi_host
 * 
 * 67    7/03/01 9:27a Llee
 * Changed COMMAND_REV to 14
 * 
 * 66    7/02/01 2:44p Llee
 * Changed names of the character set identifiers to match the spec.
 * 
 * 65    6/12/01 3:28p Llee
 * Added getting the encryption pad count value from GetObjectMetadata
 * function.
 * 
 * 64    6/12/01 8:32a Tstephens
 * Shorten structure names for GET_OBJECT_METADATA_DATA_EXAMPLE1
 * GET_OBJECT_METADATA_DATA_EXAMPLE2 (and ..EXAMPLE3) to be in the form of
 * GET_OBJECT_METADATA_DATA_EX1 for C51 Complier's limit of 32 sig
 * chars in a name. 
 * 
 * 63    6/04/01 11:23a Tfeldman
 * Changed the type of U128, U160 and U304 back to be structures. Note
 * that the standard, Hungarian notation pointer types are still not
 * defined.
 * 
 * 62    5/14/01 5:30p Tfeldman
 * Changed the type of U128, U160 and U304 to be arrays instead of
 * structures. Note that now the standard, Hungarian notation pointer
 * types are not defined for these base defined types.
 * 
 * 61    5/11/01 11:27a Llee
 * Changed ContentKey string in DevInfoStruct to 6 bytes.
 * Added examples of structures for GET_OBJECT_METADATA and
 * GET_MEDIA_METADATA.
 * 
 * 60    5/03/01 5:23p Mgurkowski
 * Change to Set Device Parms defines
 * 
 * 59    5/02/01 6:35p Tfeldman
 * Added the U304 type for wrapped keys.
 * 
 * 58    4/22/01 4:34p Tfeldman
 * Added error codes for ContentKey secure metadata.
 * 
 * 57    4/22/01 2:09p Tfeldman
 * Added error codes for ContentKey Unlock.
 * 
 * 56    4/21/01 8:56p Tfeldman
 * Added invalid KeyBox error code.
 * 
 * 55    4/17/01 1:58p Llee
 * Put ReadDir back to code 0x0B.
 * Added ContentKey version string to GetDeviceInfo.
 * 
 * 54    4/16/01 1:42p Llee
 * Changed to provide backwards compatibility with old ReadDir command,
 * with option to return generalized OBJECT_METADATA.
 * 
 * 53    4/11/01 7:00p Tfeldman
 * Moved FILE_DRM_PLAY from dpi_ckhost.h to dpi_host.h. Updated CK error
 * codes.
 * 
 * 52    4/10/01 3:05p Llee
 * Removed DFS_ATT_DRM bit from DFS attributes.
 * 
 * 51    4/09/01 8:00p Llee
 * Changed READDIR command packet. Removed RDDIRENTRY structure.
 * Added GET_OBJECT_METADATA_DATA and GET_MEDIA_METADATA_DATA structures.
 * Added CONTENTKEY_STATE definitions.
 * 
 * 50    4/04/01 4:30p Tfeldman
 * Yet another ContentKey error code.
 * 
 * 49    4/04/01 3:38p Tfeldman
 * Added a couple ContentKey error codes.
 * 
 * 48    4/04/01 10:14a Tfeldman
 * Moved CONTENTKEY_STATE from dpi_host.h to dpi_ckhost.h
 * 
 * 47    3/30/01 11:35a Mgurkowski
 * Added Power State defines for Power Control Command
 * 
 * 46    3/30/01 11:29a Mgurkowski
 * Added Unique Engine Number
 * 
 * 45    3/29/01 7:01p Llee
 * Added DFSCMD_GET_OBJECT_METADATA and DFSCMD_GET_MEDIA_METADATA
 * commands.
 * Added ContentKey values.
 * Changed Len member in DFSNAME to NameInfo to force rework of length in
 * bottom bits and char set in upper bits.
 * 
 * 44    3/21/01 4:48p Mgurkowski
 * Added Skip (shock) Seconds to Device Info and Set Parms commands
 * 
 * 43    3/21/01 8:51a Tfeldman
 * Added invalid play session error code symbol.
 * 
 * 42    3/18/01 3:37p Tfeldman
 * Added CK_COPY_COUNT_IS_ZERO error symbol.
 * 
 * 41    3/17/01 6:28p Tfeldman
 * Added some ContentKey error symbols.
 * 
 * 40    3/16/01 8:37p Tfeldman
 * Fixed ContentKey state names (they were "permissions").
 * 
 * 39    3/16/01 10:24a Tfeldman
 * Added some ContentKey error code symbols.
 * 
 * 38    3/14/01 8:35a Tfeldman
 * Added U128, U160 and U384 types.
 * 
 * 37    3/13/01 2:30p Tfeldman
 * Added DPICMD_CK_COMMAND definition. Currently the same value as
 * DPICMD_DFS_COMMAND.
 * 
 * 36    2/08/01 1:33p Mgurkowski
 * Change to PARM_STATE value
 * 
 * 35    2/08/01 1:30p Mgurkowski
 * Added Power Mode Command parm
 * 
 * 34    2/01/01 4:43p Rhines
 * warn of undefined default compiler
 * 
 * 33    1/24/01 4:53p Mgurkowski
 * Added Status Phase Length
 * 
 * 32    1/16/01 10:14a Llee
 * Added side A/B information to GetMediaInfo.
 * 
 * 31    12/20/00 2:30p Mgurkowski
 * Added Download Firmware defines
 * 
 * 30    12/19/00 10:22a Llee
 * Changed DELETE packet to DELETEPKT.
 * 
 * 29    12/15/00 3:15p Llee
 * Added DELETE structure. Cleaned up comments.
 * 
 * 28    12/05/00 5:39p Mgurkowski
 * Changes to Device info command
 * 
 * 27    12/05/00 3:49p Llee
 * Added skipping defines if __refdef_h__ defined.
 * 
 * 26    11/08/00 9:33a Llee
 * Updated how firmware rev is created.
 * 
 * 25    11/03/00 1:12p Llee
 * Removed old DFS Data-typing support. Several fixes for ReadDir.
 * 
 * 23    10/31/00 2:04p Llee
 * Added new DFS metadata system replacing old memory management system.
 * 
 * 22    10/05/00 4:10p Llee
 * Added GetDirInfo command.
 * 
 * 21    8/31/00 4:14p Mgurkowski
 * Added Media State command
 * 
 * 20    8/30/00 4:15p Llee
 * Added DFS_ATT_MOVE permissions attribute.
 * 
 * 19    8/07/00 1:03p Mgurkowski
 * Added I_LOAD_WITH_MEDIA
 * 
 * 18    8/04/00 2:02p Mgurkowski
 * Added Power Control Command
 * 
 * 17    7/24/00 3:23p Mgurkowski
 * Chenged code rev so it's not confused with releases
 * 
 * 16    7/18/00 4:24p Rhines
 * Added Keil C51 section
 * Removed ';' from TYPEDEF for Keil C51
 * 
 * 15    7/14/00 11:17a Mgurkowski
 * Bump Rev number
 * 
 * 14    7/11/00 4:18p Llee
 * Cleaned up error codes.
 * 
 * 13    7/10/00 5:12p Mgurkowski
 * Added Error code 0x08 (wrt session)
 * 
 * 12    7/10/00 1:36p Llee
 * Updated for rev 8 cmd spec. Added revision strings. Added
 * DEVINFOSTRUCT.
 * 
 * 11    6/15/00 3:06p Llee
 * Added option to GetHandle for parent directory.
 * 
 * 10    6/15/00 1:13p Llee
 * Removed file and directory count from GetMediaInfo
 * 
 * 9     6/15/00 9:44a Llee
 * Fixes for Delete and ReadDir
 * 
 * 8     6/13/00 3:49p Llee
 * Implemented new and improved ReadDir command.
 * 
 * 7     6/07/00 7:08p Llee
 * Changes for new attributes and attributes permissions mask.
 * 
 * 6     6/05/00 7:39p Llee
 * Cleanup + new error code.
 * 
 * 5     5/31/00 3:58p Mgurkowski
 * Added I_MEDIA_LOCKED
 * 
 * 4     5/31/00 1:19p Llee
 * Changes to dpi_host.h.
 * 
 * 3     5/26/00 1:06p Mpropps
 * Added no WSA error code.
 * 
 * 2     5/26/00 10:32a Llee
 * Added DPICMD equates.
 * 
 * 1     5/26/00 10:18a Llee
 * DPI Include File shippable to customers.
-----------------------------------------------------------------------------*/

#ifndef __dpi_host_h__
#define __dpi_host_h__

/* Revisions Values */
#define	COMMAND_REV			"17"			// Command specification revision (2 chars)

/* ----------------------- Simple Type Definitions --------------------------*/
#ifndef __k_defs_h__
#ifndef __refdefs_h__						// Reference kit defines ing refdefs.h

	
// General Compiler Requirements
// 	- Structure alignment must be set to 2 bytes.
// 

// Build Options - may be overridden before inclusion

// Type definition macro.
#define TYPEDEF(typeid, typename)							\
	typedef typeid 				typename;


// ARM-ELF-GCC datatypes - temancl@fullplaymedia.com

TYPEDEF (unsigned char, 	U8);			// Unsigned 8-bits
TYPEDEF (unsigned short, 	U16);			// Unsigned 16-bits
TYPEDEF (unsigned long, 	U32);			// Unsigned 32-bits
TYPEDEF (char,				S8); 			// Signed 8-bits
TYPEDEF (short, 			S16);			// Signed 16-bits
TYPEDEF (long, 				S32);			// Signed 32-bits
struct U64_tag	{U32 lo, hi;};	
TYPEDEF (struct U64_tag, 	U64);			// Unsigned 64-bits


struct U128_tag {U8 Byte[128/8];};
typedef struct U128_tag U128;				// Unsigned 128 bits (16 bytes)
struct U160_tag {U8 Byte[160/8];};
typedef struct U160_tag U160;				// Unsigned 160 bits (20 bytes)
struct U304_tag {U8 Byte[304/8];};
typedef struct U304_tag U304;				// Unsigned 304 bits (38 bytes)

// Define DFS standard types
TYPEDEF (U32,		DFSHANDLE);			// File system handle
TYPEDEF (U16,		DFSATTRIB);			// File attributes
TYPEDEF (U16,		DFSATTRIBMASK);	// File attributes modification mask
TYPEDEF (U32,		DFSTIME);			// Timestamp for a file
TYPEDEF (U16,		DFSREADOPTS);		// ReadFile options
TYPEDEF (U16,		DFSWRITEOPTS);		// WriteFile options
TYPEDEF (U16,		DFSSETATTROPTS);	// SetAttributes options
TYPEDEF (U16,		DFSGETHANDLEOPTS);// GetHandle options
TYPEDEF (U16,		RDDIROPTIONS);		// ReadDir options
TYPEDEF (U16,		RDDIRSESSION);		// ReadDir session ID
TYPEDEF (U16,		RDDIRFLAGS);		// ReadDir flags


#endif	// __refdefs_h__
#endif	// _k_defs_h



/* ------------------------------- Constants --------------------------------*/

// Command Code Values
// DPI command definitions.
#define	DPICMD_DEVICE_INFO			0x00	//	Get device information

#define	DPICMD_LOCK_RELEASE_MEDIA	0x01	//	Locks/Release the media for ejection

#define	DPICMD_SET_PARAMETERS		0x02	//	Set device paramters
	// Set Device Parameters Ids
	#define	PARM_INTF_PACKET_SIZE	0	 	// Set interface packet size
	#define	PARM_HOST_RD_XFER_RATE	1	 	// Host Sustained Read Transfer Rate in Kbytes/second
	#define	PARM_HOST_WR_XFER_RATE	2	 	// Host Sustained Write Transfer Rate in Kbytes/second
	#define	PARM_MAX_SPINUP_CURRENT	3	 	// Set max Spin-up current (ma)
	#define	PARM_SKIP_SECONDS			5	 	// Set Skip Seconds
	#define	PARM_AUTO_HOST_RDRATE	6	 	// Enable/Disable Auto Host Sustained Read Transfer Rate
		#define	PARM_AUTO_RATE_ON		1		// Option Parm for PARM_AUTO_HOST_RDRATE
		#define	PARM_AUTO_RATE_OFF	0		// Option Parm for PARM_AUTO_HOST_RDRATE

#define	DPICMD_GET_ATTENTION_INFO	0x03	//	Get reason information for the Attention IRQ
	// Attention Information bits
	#define	MEDIA_INSERTED				0x8000// Media inserted attention

#define	DPICMD_GET_EXTENDED_STATUS	0x04	//	Get Extended Status bytes

#define	DPICMD_EJECT_MEDIA			0x05	// Eject Media command

#define	DPICMD_POWER_CONTROL			0x06	// Power Control command
	// Power Control Parameters
	#define	PARM_PREPARE_PWR_DN		0	 	// Enter lowest power state and prepare for power-off
	#define	PARM_IDLE					1	 	// Enter Idle state (spundown, micro off)
	#define	PARM_READY					2	 	// Enter Ready state (spunup, micro on)
	#define	PARM_STATE					3		// Return Power Mode State
		// State Definitions
		#define	STATE_IDLE				1
		#define	STATE_READY				2
		#define	STATE_ACTIVE			3

#define	DPICMD_MEDIA_STATE			0x07	//	Get Media State Command

#define	DPICMD_DOWNLOAD_FIRMWARE	0x0F	//	Download Firmware Command
	#define	PARM_DWN_LOAD_1			0x05 	// Download Parmameter 1
	#define	PARM_DWN_LOAD_2			0xF0 	// Download Parmameter 2

#define	DPICMD_DFS_COMMAND			0x10	//	DataPlay File System Command
#define	DPICMD_CK_COMMAND				0x10	//	ContentKey Command

// DFS sub-command definitions
#define	DFSCMD_COMMIT 		 	0x01			// Commit cached data to the media.	
#define	DFSCMD_CREATEDIR		0x02			// Create a new DFS directory within a directory.	
#define	DFSCMD_CREATEFILE		0x03			// Create a new DFS file within a directory.	
#define	DFSCMD_DELETE			0x04			// Delete DFS object.	
#define	DFSCMD_GETATTR			0x05			// Returns attributes for a DFS object.	
#define	DFSCMD_GETDIRINFO		0x0F			// Return extended directory information.	
#define	DFSCMD_GETFILEINFO	0x06			// Return extended file information.	
#define	DFSCMD_GETHANDLE		0x07			// Return handle for DFS object with given name.	
#define	DFSCMD_GET_MEDIA_METADATA	 0x12	// Return media metadata
#define	DFSCMD_GETMEDIAINFO	0x08			// Return media information	
#define	DFSCMD_GET_OBJECT_METADATA  0x13	// Return file/directory metadata
#define	DFSCMD_GET_USER_METADATA	 0x15	// Return user metadata for an object
#define	DFSCMD_MOVE				0x09			// Move an object within directory structure.
#define	DFSCMD_READFILE		0x0A			// Read data from a file.	
#define	DFSCMD_READDIR			0x0B			// Read directory entries for a given directory
#define	DFSCMD_RENAME			0x0C			// Rename/move DFS object.	
#define	DFSCMD_SETATTR			0x0D			// Set attributes for a DFS object.	
#define	DFSCMD_SET_USER_METADATA	0x016	// Set user metadata for an object
#define	DFSCMD_WRITEAPPEND	0x0E			// Appends data to a file.
#define	DFSCMD_WRITEMODIFY	0x14			// Write modifies data in a file.

// DPI Status Phase Length (bytes)
#define	STATUS_PHASE_LENGTH	0x0004		// Status Phase Length in bytes

// DPI Maximum Command packet size (bytes)
#define	MAX_CMD_PACKET_LENGTH	(132 + 2 + MAX_NAME_LEN + 1)	// Maximum packet size for any command
																					// 132 = static part of CKCMD_CREATE_FILE
																					// +2 = NAMEINFO length
																					// +MAX_NAME_LEN = maximum size of name
																					// +1 = pad to make it even

// Status bits returned to the host during ending status phase
#define	CACHED_DATA				0x0001		// Write Data is Cached
#define	END_OF_FILE				0x0002		// End of file encountered
#define	MEDIA_STATUS			0x0004		// Media Present if set
#define	DFS_INCOMPLETE			0x0008		// New Media failed resulting in the File System info being incomplete

// DFS State Attribute bits
#define	DFS_ATT_STATE_MASK	0x00FF		// All state related bits
#define	DFS_ATT_TYPE_MASK		0x0001		// Bit mask for type of object
#define	DFS_ATT_HIDDEN			0x0008		// Object is hidden during ReadDir
#define	DFS_ATT_CONTENTKEY	0x0010		// File is controlled by ContentKey

// DFS Permission Attribute bits
#define	DFS_ATT_PERMISSIONS					0xFF00		// Permissions mask - all the permissions bits
#define	DFS_ATT_WRITE							0x0200		// Content of object may be written
#define	DFS_ATT_SET_USER_METADATA			0x0400		// Object may have its user metadata modified
#define	DFS_ATT_DELETE							0x0800		// Object may be deleted
#define	DFS_ATT_MOVE							0x1000		// Object may be moved
#define	DFS_ATT_CREATE							0x2000		// Object may have other objects created

// DFS object types in attributes
#define	DFS_TYPE_DIR			0				// Directory type
#define	DFS_TYPE_FILE			1				// File type

// DFS Limitations
#define	MAX_NAME_LEN			511		  	// Maximum number of bytes for a name
#define	MAX_MIME_LEN			511			// Maximum number of bytes in a Mime type
#define	MIN_READDIR_BUFFER 	32				// Minimum number of bytes for a ReadDir output buffer
#define	MAX_READDIR_BUFFER  	16384			// Maximum read directory buffer which may be requested

// CONTENTKEY_STATE definition
TYPEDEF (U16, CONTENTKEY_STATE);				// ContentKey permission and attribute flags
#define	FILE_CONTENTKEY_COPY_FREELY						0x0001	// if set, ContentKey copies may be made without restriction.
#define	FILE_CONTENTKEY_LOCKED								0x0002	// 0 = file is unlocked, 1 = file is locked
#define	FILE_CONTENTKEY_PLAY									0x0004	// if set, the ContentKey Play method is allowed.
#define	FILE_DRM_COPY_FREELY									0x0008	// if set, the DRM copies may be made without restriction.
#define	FILE_UNLIMITED_FIRST_GENERATION_CK_COPIES		0x0010	// if set, first generation ContentKey copies may be made without limit.
#define	FILE_UNLIMITED_FIRST_GENERATION_DRM_COPIES	0x0020	// if set, first generation DRM copies may be made without limit.
#define	FILE_DRM_PLAY											0x0040	// if set, the DRM Play method is allowed
#define	FILE_CONTENTKEY_LOCKABLE							0x0080	// 0 = file may not be locked, 1 = may be locked

// OBJECT_METADATA definition
TYPEDEF (U16, OBJECT_METADATA);
#define	METADATA_ATTRIBUTES 					0x0001	// if set, the Attributes of the object is added to the output structure.
#define	METADATA_HANDLE						0x0002	//	if set, the handle of the object is added to the output structure
#define	METADATA_PARENT 						0x0004	// if set, the parent(s) of the object are added to the output structure.
#define	METADATA_LASTMODTIME					0x0008	// if set, the Last Modified Time of the object is added to the output structure.
#define	METADATA_DATASIZE						0x0010	// if set, the size of the data associated with the object is added to the output structure.
#define	METADATA_ATTRIBMASK 					0x0020	// if set, the Attributes Modification Mask of the object is added to the output structure.
#define	METADATA_NAMEINFO						0x0040	// if set, the length of the name of the object is added to the output structure.
#define	METADATA_MIMELENGTH 					0x0080	// if set, the length of the MIME type associated with the object is added to the output structure.
#define	METADATA_CONTENTKEY_STATE 			0x0100	// if set, the ContentKey permissions for the file is added to the output structure.
#define	METADATA_CONTENTKEY_COPY_COUNT	0x0200 	// if set, the number of ContentKey copies remaining on the file is added to the output structure.
#define	METADATA_CONTENTKEY_DRM_COUNT 	0x0400	// if set, the number of DRM copies remaining on the file is added to the output structure.
#define	METADATA_CONTENTKEY_PAD_COUNT		0x0800	// if set, the number of bytes of encryption pad for the file is added to the output structure.
#define	METADATA_DFSNAME						0x4000	// if set, the name of the object is added to the output structure
#define	METADATA_DFSMIME						0x8000	// if set, the MIME string of the object is added to the output structure.

// EXT_OBJECT_METADATA definition
TYPEDEF (U16, EXT_OBJECT_METADATA);
#define	EXT_METADATA_ORIGINAL_CONTENTKEY_COPY_COUNT	0x0001	// if set, the original number of ContentKey copies for the file is added to the output structure.
#define	EXT_METADATA_ORIGINAL_CONTENTKEY_DRM_COUNT	0x0002	// if set, the original number of DRM copies for the file is added to the output structure.

// MEDIA_METADATA definition
TYPEDEF (U16, MEDIA_METADATA);
#define	METADATA_ROOT_HANDLE					0x0001	// if set, the handle of the root directory is added to the output structure.
#define	METADATA_MEDIA_FLAGS					0x0002	// if set, the state information of the media is added to the output structure.
#define	METADATA_ROOT_NAMEINFO				0x0004	// if set, the length and character set of the root directory name is added to the output structure.
#define	METADATA_TOTAL_BYTES					0x0008	// if set, the total bytes on the medium is added to the output structure.
#define	METADATA_REMAINING_BYTES			0x0010	// if set, the remaining writeable bytes is added to the output structure.
#define	METADATA_CONTENT_SIDE_ID			0x0020	// if set, the mastered side identifier is added to the output structure.
#define	METADATA_UNIQUE_MEDIA_ID			0x0040	// if set, the unique media identifier is added to the output structure.
#define	METADATA_COLLECTION_CONTENT_ID	0x0080	// if set, the unique identifier for a multi-sided collection is added to the output structure.
#define	METADATA_DATA_BLOCK_SIZE			0x0100	// if set, the size, in bytes, of the data block on the media is added to output structure.
#define	METADATA_ROOT_DFSNAME				0x8000	// if set, the name of the root directory  is added to the output structure

// User metadata definitions
#define	USER_METADATA_MAX_LENGTH			0x01FF		// Maximum number of bytes in a user metadata string
#define	USER_METADATA_LENGTH_MASK			0x01FF		// Mask for the length of the metadata
#define	USER_METADATA_OTHER_MASK			0xFE00		// Mask for the other bits of the user metadata length
#define	USER_METADATA_DFSNAME				0x0000		// Tag for the DFSNAME of the object
#define	USER_METADATA_MIME					0x0001		// Tag for the MIME type of the object
#define	USER_METADATA_LAST_MOD_TIME		0x0002		// Tag for the last modification time of the object
#define	USER_METADATA_FIRST_DRM				0x8000		// First DRM protected metadata tag
#define	USER_METADATA_LAST_DRM				0xFFFF		// Last DRM protected metadata tag

// NAMEINFO definition
TYPEDEF (U16, NAMEINFO);
#define	NAMEINFO_LENGTH_MASK					0x01FF		// Mask for length of name
#define	NAMEINFO_CHARSET_MASK				0x8000		// Mask for character set of name
#define	NAMEINFO_CHARSET_OFFSET				15				// First bit position for charset
#define	CS_UNICODE_8_BIT						(0x0)			// 1-byte Unicode charset
#define	CS_UNICODE_16_BIT_LE					(0x1)			// 2-byte Unicode, little-endian

// MEDIAFLAGS definition
TYPEDEF (U16, MEDIAFLAGS);
#define	MEDIA_SIDE_B  							0x0001		//	bit 0: 0=Media is on side A, 1=Media is on side B

// Interface Error Codes
#define	I_UNKNOWN_COMMAND						0x01	// Unknown interface command
#define	I_NO_MEDIA								0x02	// Media Not Present
#define	I_BAD_CMD_SIZE							0x03	// Invalid Command Packet Size
#define	I_BAD_CMD_PARM							0x04	// Invalid Command Packet Parameter
#define	I_MEDIA_LOCKED							0x05	// Media Is Locked
#define	I_WRT_SESSION_ACTIVE					0x07	// Illegal command during write session
#define	I_LOAD_WITH_MEDIA						0x08	// Load command when Media ia already present
#define	DFS_ACCESS_DENIED						0x10	// DFS object access denied
#define	DFS_WRITE_DENIED						0x11	// Write attempted on object with DFS_ATT_WRITE clear
#define	DFS_DELETE_DENIED						0x12	// Delete attempted on object with DFS_ATT_DELETE clear
#define	DFS_BAD_DFSHANDLE						0x15	// The input DFSHANDLE parameter was invalid
#define	DFS_NAME_EXISTS						0x16	// A DFS object of the same name already exists
#define	DFS_NAME_TOO_LONG						0x17	// The input DFSNAME parameter was too long
#define	DFS_NAME_NOT_FOUND					0x18 	// The object with the given input DFSNAME parameter was not found
#define	DFS_WRTCACHE_FAIL						0x19	// Error occurred writing write cached data to the media
#define	DFS_BAD_BUFFER_SIZE					0x1A	// Illegal buffer size passed to DFS command
#define	DFS_BAD_SESSION_ID					0x1B	// Bad session ID on ReadDir command
#define	DFS_ILLEGAL_MOVE						0x1C	// Illegal directory move operation attempted
#define	DFS_DIR_NOT_EMPTY						0x1D	// Attempt to delete directory that is not empty
#define	DFS_MOVE_DENIED						0x1E	// Attempt to move an object with DFS_ATT_MOVE clear
#define	DFS_BAD_WRITE_OFFSET					0x1F	// Attempt to write insert at an illegal offset
#define	DFS_FILE_LOCKED						0x20	// Illegal operation attempted on a locked file
#define	DFS_CREATE_DENIED						0x21	// Attempt to create an object with DFS_ATT_CREATE clear
#define	DFS_SET_USER_METADATA_DENIED		0x22	// Attempt to set the user metadata with DFS_ATT_SET_USER_METADATA clear
#define	DFS_USER_METADATA_NOT_FOUND 		0x23	// Requested metadata was not found for the given object
#define	DFS_COMMIT_VERIFY_ERROR				0x24	// Verify of data failed on commit
#define	R_MEDIA_FULL							0x50	// Media is full
#define	I_RESERVED_1							0xFE	// Used by DataPlay Custom USB adapter board
#define	I_UNRECOVERABLE						0xFF	// Unrecoverable Error


/* ------------------------------- DFS Type Definitions ----------------------------*/



/*---- STRUCTURE ----
Name : DFSNAME
Purp : Holds the DFS name.
---------------------*/
// Define equate for the number of bytes reserved for Name in DFSNAME structure.
// This is used to calculate the size of the other structures with a zero length name.
typedef struct
{													// Bytes: Description
	NAMEINFO	NameInfo;						// 00-01: Length and charset of name
	U8			Name[MAX_NAME_LEN];			// 02-xx: Reserve characters
} DFSNAME;									
#define	DFSNAME_SIZE_NO_NAME	(sizeof (NAMEINFO)) 							// Number of bytes without the Name
#define	DFSNAME_NAME_SIZE	(sizeof (DFSNAME) - DFSNAME_SIZE_NO_NAME)	// Number of bytes used by name
#define	GetNameLength(pDfsName)			((pDfsName)->NameInfo & NAMEINFO_LENGTH_MASK)
#define	SetNameLength(pDfsName, Len)	{(pDfsName)->NameInfo = ((pDfsName)->NameInfo & ~NAMEINFO_LENGTH_MASK) | ((NAMEINFO) (Len));}
#define	GetNameCharSet(pDfsName)		(((pDfsName)->NameInfo & NAMEINFO_CHARSET_MASK) >> NAMEINFO_CHARSET_OFFSET)
#define	SetNameCharSet(pDfsName, cs)	{(pDfsName)->NameInfo = ((pDfsName)->NameInfo & ~NAMEINFO_CHARSET_MASK) | (((NAMEINFO) (cs)) << NAMEINFO_CHARSET_OFFSET);}


/*---- STRUCTURE ----
Name : DFSMIME
Purp : Holds the DFS mime string.
---------------------*/
// Define equate for the number of bytes reserved for Name in DFSMIME structure.
// This is used to calculate the size of the other structures with a zero length name.
typedef struct
{												// Bytes: Description
	U16	Len;	  							// 00-00: Length of name in bytes
	U8		Name[MAX_MIME_LEN];			// 01-xx: Reserve characters
} DFSMIME;									
#define	DFSMIME_SIZE_NO_NAME	(sizeof (U8))									// Number of bytes without the Name
#define	DFSMIME_NAME_SIZE	(sizeof (DFSMIME) - DFSMIME_SIZE_NO_NAME)	// Number of bytes used by name





/* ---------------------------- DFS Command Definitions ----------------------------*/



/*---- STRUCTURE ----
Name : COMMIT
Purp : Commit packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				CommitOptions;		// 02-03: Options for commit
		#define	FORCE_TO_MEDIA	0x0001//			 0 = Allow caching updates in flash
												//			 1 = Force updates to the media
		#define	VERIFY_WRITE	0x0002//			 0 = no verify of write data
												// 		 1 = data written during write session is verified
} COMMIT;
#define	COMMIT_SIZE	(sizeof (COMMIT))



/*---- STRUCTURE ----
Name : CREATEFILE
Purp : Defines a DFS create packet from the host
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;  			// 02-03: Reserved
	DFSHANDLE		DfsHandle; 			// 04-07: Directory to create file
	DFSNAME			Name;					// 08-xx: DFS name structure
} CREATEFILE;

// Define equate for the size of CREATFILE with a zero length name
#define	CREATEFILE_SIZE_NO_NAME	(sizeof (CREATEFILE) - DFSNAME_NAME_SIZE)	


/*---- STRUCTURE ----
Name : CREATEDIR
Purp : Defines a DFS packet for the CREATEDIR command from the host.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	 			// 00-00: Command as sent from the host
	U8					DfsSubCmd;			// 01-01: DFS subcommand from host
	U16				Reserved;			// 02-03: Reserved
	DFSHANDLE		DfsHandle;			// 04-07: Handle to the directory to create directory
	DFSNAME			Name;					// 08-xx: DFS name structure
} CREATEDIR;

// Define equate for the size of CREATFILE with a zero length name
#define	CREATEDIR_SIZE_NO_NAME	(sizeof (CREATEDIR) - DFSNAME_NAME_SIZE)	


/*---- STRUCTURE ----
Name : DELETEPKT
Purp : Delete packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;			// 02-03: Reserved
	DFSHANDLE		DfsHandle;			// 04-07: Dfs handle to remove
} DELETEPKT;
#define	DELETEPKT_SIZE	(sizeof (DELETEPKT))


/*---- STRUCTURE ----
Name : GETATTRIBUTES
Purp : GetAttributes packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;			// 02-03: Reserved
	DFSHANDLE		DfsHandle;			// 04-07: Dfs handle to get attributes
} GETATTRIBUTES;
#define	GETATTRIBUTES_SIZE	(sizeof (GETATTRIBUTES))



/*---- STRUCTURE ----
Name : GETDIRINFO
Purp : GetDirInfo packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;  			// 02-03: Reserved
	DFSHANDLE		DfsHandle; 			// 04-07: Dfs handle to get info on
} GETDIRINFO;
#define	GETDIRINFO_SIZE	(sizeof (GETDIRINFO))



/*---- STRUCTURE ----
Name : GETFILEINFO
Purp : GetFileInfo packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;  			// 02-03: Reserved
	DFSHANDLE		DfsHandle; 			// 04-07: Dfs handle to get info on
} GETFILEINFO;
#define	GETFILEINFO_SIZE	(sizeof (GETFILEINFO))


/*---- STRUCTURE ----
Name : GETHANDLE
Purp : Defines a DFS packet for the GETHANDLE command from the host.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	 			// 00-00: Command as sent from the host
	U8					DfsSubCmd;			// 01-01: DFS subcommand from host
	DFSGETHANDLEOPTS GetHandleOpts;	// 02-03: Options for get handle
		#define	GET_PARENT		0x0001		// bit 0: Retrieve the parent for the specified handle (ignore name)
		
	DFSHANDLE		DfsHandle;			// 04-07: Handle to the directory to create directory
	DFSNAME			Name;					// 08-xx: DFS name structure
} GETHANDLE;

// Define equate for the size of CREATFILE with a zero length name
#define	GETHANDLE_SIZE_NO_NAME	(sizeof (GETHANDLE) - DFSNAME_NAME_SIZE)	


/*---- STRUCTURE ----
Name : GET_MEDIA_METADATA
Purp : GetMediaMetadata packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8						HostCmd;	  		// 00-00: Command as sent from the host
	U8						DfsSubCmd; 		// 01-01: DFS subcommand from host
	MEDIA_METADATA		MediaMetadata;	// 02-03: Metadata options
} GET_MEDIA_METADATA;
#define	GET_MEDIA_METADATA_SIZE	(sizeof (GET_MEDIA_METADATA))


/*---- STRUCTURE ----
Name : GETMEDIAINFO
Purp : GetMediaInfo packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
} GETMEDIAINFO;
#define	GETMEDIAINFO_SIZE	(sizeof (GETMEDIAINFO))

/*---- STRUCTURE ----
Name : GET_OBJECT_METADATA
Purp : GetObjectMetadata packet definition.
---------------------*/
typedef struct 
{															// Bytes: Description	
	U8							HostCmd;	  				// 00-00: Command as sent from the host
	U8							DfsSubCmd; 				// 01-01: DFS subcommand from host
	OBJECT_METADATA		ObjectMetadata;		// 02-03: Metadata options
	DFSHANDLE				DfsHandle;				// 04-07: Handle to get metadata for
	EXT_OBJECT_METADATA	ExtObjectMetadata;	// 08-09: Extended object metadata
} GET_OBJECT_METADATA;
#define	GET_OBJECT_METADATA_SIZE	(sizeof (GET_OBJECT_METADATA))

/*---- STRUCTURE ----
Name : GET_USER_METADATA
Purp : GetUserMetadata packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description	
	U8						HostCmd;	  		// 00-00: Command as sent from the host
	U8						DfsSubCmd; 		// 01-01: DFS subcommand from host
	U16					UserMetadataTag;// 02-03: Metadata options
	DFSHANDLE			DfsHandle;		// 04-07: Handle to get metadata for
} GET_USER_METADATA;
#define	GET_USER_METADATA_SIZE	(sizeof (GET_USER_METADATA))


/*---- STRUCTURE ----
Name : MOVE
Purp : Move command packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;			// 02-03: Reserved
	DFSHANDLE		DfsHandle;			// 04-07: Dfs handle of object to move
	DFSHANDLE		DirHandle;			// 08-0C: Handle of new directory for object
} MOVE;
#define	MOVE_SIZE	(sizeof (MOVE))


/*---- STRUCTURE ----
Name : READDIR
Purp : Packet for the READDIR command.
---------------------*/
typedef struct
{													// Bytes: Description
	U8						HostCmd;	 			// 00-00: Command as sent from the host
	U8						DfsSubCmd; 			// 01-01: DFS subcommand from host
	RDDIROPTIONS		RddirOptions;		// 02-03: Options for READDIR command
		#define			RDDIR_ENTRY_COUNT			0x000F	// bit 0-3: Number of entries to return (0 = fill buffer)
		#define			FILTER_BY_ATTRIBUTES		0x0010	// bit 4  : Apply the attributes filter
		#define			FILTER_BY_MIMETYPE		0x0020	// bit 5  : Apply the MIME filetype filter
		#define			RDDIR_RESTART_SESSION	0x0040	// bit 6  : Restart the directory
		#define			RDDIR_CLOSE_SESSION		0x0080	// bit 7  : Close the given read directory session
		#define			FILTER_BY_PARTIALMIME	0x0100	// bit 8  : Apply partial MIME filetype filter
		#define			USE_OBJECT_METADATA		0x0200	// bit 9  : Use the ObjectMetadata parameter to create output
	DFSHANDLE			DfsHandle;			// 04-07: Handle to the directory to create directory
	RDDIRSESSION		SessionID;			// 08-09: Directory session identifier
	OBJECT_METADATA	ObjectMetadata;	// 0A-0B: Object metadata options for data returned
	U16					MaxByteCount;		// 0C-0D: Maximum number of bytes to transfer
	DFSATTRIB			AttribMask;			// 0E-0F: (optional) Mask value for attributes
	DFSATTRIB			AttribMatch;		// 10-11: (optional) Match value for attributes			
	DFSMIME				MimeFilter;			// 12-xx: (optional) Mime-type filter
} READDIR;
#define	READDIR_SIZE	(sizeof (READDIR))

// Define equate for the size of CREATFILE with a zero length name
#define	READDIR_SIZE_NO_NAME	(sizeof (READDIR) - DFSMIME_NAME_SIZE)	



/*---- STRUCTURE ----
Name : READFILE
Purp : Defines a DFS read packet from the host.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	 			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	DFSREADOPTS		XferOpts; 			// 02-03: Transfer options.
		#define		READ_STREAMING		0x0001	// bit 0: Read streaming mode for ReadFile command
		#define		READ_TO_EOF	  		0x0002	// bit 1: Read to EOF for ReadFile command
	
	DFSHANDLE		DfsHandle; 			// 04-07: DFS object to read from
	U64				ByteOffset;			// 08-0F: Byte offset to start read from
	U64				ByteCount;			// 10-17: Number of bytes to transfer
} READFILE;
#define	READFILE_SIZE	(sizeof (READFILE))



/*---- STRUCTURE ----
Name : REMOVE
Purp : Remove packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;	 		// 02-03: Reserved
	DFSHANDLE		DfsHandle;	 		// 04-07: Dfs handle to remove
} REMOVE;
#define	REMOVE_SIZE	(sizeof (REMOVE))



/*---- STRUCTURE ----
Name : RENAME
Purp : Rename packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	U16				Reserved;			// 02-03: Reserved
	DFSHANDLE		DfsHandle;			// 04-07: Dfs handle of object to rename
	DFSNAME			Name;					// 08-xx: Name of new object
} RENAME;
#define	RENAME_SIZE_NO_NAME	(sizeof (RENAME) - DFSNAME_NAME_SIZE)



/*---- STRUCTURE ----
Name : SETATTRIBUTES
Purp : SetAttributes packet definition.
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	  			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	DFSSETATTROPTS	SetAttrOpts;		// 02-03: Set attributes command options
		#define		SET_ATTRIB_MASK	0x0001		// bit 0 : 0 = Set the object's attributes
																//			  1 = Set the object's Attributes Modification Mask	
	
	DFSHANDLE		DfsHandle;	 		// 04-07: Dfs handle to remove
	DFSATTRIB		Attributes;	 		// 08-09: Attributes to set for the item
	
} SETATTRIBUTES;
#define	SETATTRIBUTES_SIZE	(sizeof (SETATTRIBUTES))


/*---- STRUCTURE ----
Name : SET_USER_METADATA
Purp : Defines a DFS packet for the SET_USER_METADATA command from the host.
---------------------*/
typedef struct 
{														// Bytes: Description
	U8						HostCmd;	 				// 00-00: Command as sent from the host
	U8						DfsSubCmd;				// 01-01: DFS subcommand from host
	U16					UserMetadataTag;		// 02-03: Tag number for the user metadata
	DFSHANDLE			DfsHandle; 				// 04-07: Dfs handle to set mime
	U16					UserMetadataLength;	// 08-09: Number of bytes in the user metadata
	U8						UserMetadata[USER_METADATA_MAX_LENGTH];	// 0A-xx: User metadata
} SET_USER_METADATA;

// Define equate for the size of SET_USER_METADATA with zero length metadata
#define	SET_USER_METADATA_SIZE_NO_METADATA	(sizeof (SET_USER_METADATA) - USER_METADATA_MAX_LENGTH)	


/*---- STRUCTURE ----
Name : WRITEAPPEND
Purp : Defines a DFS append-file packet from the host
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	 			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	DFSWRITEOPTS 	XferOpts; 			// 02-03: Write transfer options
		#define		WRITE_FOREVER 	0x0001 	// bit 0: Write forever	
		
	DFSHANDLE		DfsHandle;			// 04-07: DFS object to write to
	U64				ByteCount;			// 08-0F: Number of bytes to transfer
} WRITEAPPEND;
#define	WRITEAPPEND_SIZE	(sizeof (WRITEAPPEND))

/*---- STRUCTURE ----
Name : WRITEMODIFY
Purp : Defines a DFS modify-file packet from the host
---------------------*/
typedef struct 
{												// Bytes: Description
	U8					HostCmd;	 			// 00-00: Command as sent from the host
	U8					DfsSubCmd; 			// 01-01: DFS subcommand from host
	DFSWRITEOPTS 	XferOpts; 			// 02-03: Write transfer options (see WRITEAPPEND, above)
	DFSHANDLE		DfsHandle;			// 04-07: DFS object to write to
	U64				InsertByteCount;	// 08-0F: Number of bytes to transfer and insert into file
	U64				ByteOffset;			// 10-17: Offset into file to modify
	U64				DeleteByteCount;	// 18-1F; Number of bytes in the original file to delete
} WRITEMODIFY;
#define	WRITEMODIFY_SIZE	(sizeof (WRITEMODIFY))


/*---- UNION  ----
Name : PACKET
Purp : Contains all possible packets padded by maximum name length.
---------------------*/
union PACKET
{
	U8								Generic[MAX_NAME_LEN+2];		// Generic packet array
	COMMIT						Commit;
	CREATEDIR					CreateDir;
	CREATEFILE					CreateFile;
	GETATTRIBUTES				GetAttributes;
	GETDIRINFO 					GetDirInfo;
	GETFILEINFO 				GetFileInfo;
	GETHANDLE					GetHandle;
	GET_OBJECT_METADATA		GetObjectMetadata;
	GETMEDIAINFO				GetMediaInfo;
	GET_MEDIA_METADATA		GetMediaMetadata;
	GET_USER_METADATA			GetUserMetadata;
	MOVE							Move;
	READDIR						ReadDir;	 
	READFILE						ReadFile;
	REMOVE						Remove;
	RENAME						Rename;
	SETATTRIBUTES				SetAttributes;
	SET_USER_METADATA			SetUserMetadata;
	WRITEAPPEND					WriteAppend;
	WRITEMODIFY					WriteModify;
};






/* ------------------------- Command Response Definitions -----------------------*/

/*---- STRUCTURE ----
Name : DEVINFOSTRUCT
Purp : Device Information Table
---------------------*/
#define	SIZEOF_ENGINE_NUMBER		(20)
typedef struct
{															// Bytes (offsets in hex) 
	U8			DeviceType;								// 00-00: Device Type
	U8			Reserved[3];							// 01-03: PAD
	U8			DeviceID[20];							// 04-17: Device ASCII String ID
	U8			FirmWare[8];							// 18-1F: Frimware revision ASCII String
	U16		PacketSize;								// 20-21: Current Packet Size
	U16		MaxPacketSize;							// 22-23: Max Packet Size
	U8			CommandRev[2];							// 24-25: Command Revision Number
	U16		SpinupCurrent;							// 26-27: Current Spin-up Current limit
	U32		HostReadRate;							// 28-2B: Host Read Transfer Rate
	U32		HostWriteRate;							// 2C-2F: Host Write Transfer Rate
	U32		TimeStamp;								// 30-33: DFS Time Stamp
	U16		SkipSec;									// 34-35: Current Skip Buffer Size in Sec.
	U8			EngineNum[SIZEOF_ENGINE_NUMBER];	// 36-49: Unique Engine Number, also known as the engine's serial number
	U8			ContentKeyVersion[6];				// 4A-4F: ContentKey version number
} DEVINFOSTRUCT;
#define	DEVICEINFO_DATA_SIZE	(sizeof (DEVINFOSTRUCT))
#define	MAX_DEVICEINFO_DATA_SIZE 	(512)				// Maximum bytes ever needed


/*---- STRUCTURE ----
Name : GETATTRIBUTES_DATA
Purp : Return structure for GetAttributes
---------------------*/
typedef struct 
{												// Bytes: Description
	DFSATTRIB		Attributes;			// 00-01: Attributes of object
	DFSATTRIBMASK	AttribMask;			// 02-03: Attribute Modification Mask value of object
} GETATTRIBUTES_DATA;
#define	GETATTRIBUTES_DATA_SIZE	(sizeof (GETATTRIBUTES_DATA))


/*---- STRUCTURE ----
Name : GETDIRINFO_DATA
Purp : A DFS directory information structure for GetDirInfo 
---------------------*/
typedef struct 
{												// Bytes: Description
	DFSATTRIB		Attributes;			// 00-01: Attributes of object
	U16				Reserved;  			// 02-03: Reserved
	DFSTIME			CreationTime; 		// 04-07: Creation time of directory
} GETDIRINFO_DATA;
#define	GETDIRINFO_DATA_SIZE	(sizeof (GETDIRINFO_DATA))


/*---- STRUCTURE ----
Name : GETFILEINFO_DATA
Purp : A DFS file information structure for GetFileInfo 
---------------------*/
typedef struct 
{												// Bytes: Description
	DFSATTRIB		Attributes;			// 00-01: Attributes of object
	U16				Reserved;			// 02-03: Reserved
	DFSTIME			LastModTime; 		// 04-07: Last modification time
	U64				FileSize;  			// 08-0F: Size of file in bytes
} GETFILEINFO_DATA;
#define	GETFILEINFO_DATA_SIZE	(sizeof (GETFILEINFO_DATA))


/*---- STRUCTURE ----
Name : GETHANDLE_DATA
Purp : Return structure for GetHandle
---------------------*/
typedef struct 
{												// Bytes: Description
	DFSHANDLE		DfsHandle;			// 00-03: Handle of object
	DFSATTRIB		Attributes;			// 04-05: Attributes of object
} GETHANDLE_DATA;
#define	GETHANDLE_DATA_SIZE	(sizeof (GETHANDLE_DATA))


/*---- STRUCTURE ----
Name : GETMEDIAINFO_DATA
Purp : Return structure for GetMediaInfo
---------------------*/
typedef struct 
{												// Bytes: Description
	DFSHANDLE	RootHandle;				// 00-03: Handle of root directory
	MEDIAFLAGS	MediaFlags;				// 04-05: Media flags
	U16			Reserved;				// 04-07: Reserved
	U64			TotalBytes;				// 08-0F: Total bytes on medium
	U64			RemainingBytes;		// 10-17: Remaining writable bytes
	DFSNAME		RootName;				// 18-xx: Name of root directory
} GETMEDIAINFO_DATA;
#define	GETMEDIAINFO_DATA_SIZE_NO_NAME	(sizeof (GETMEDIAINFO_DATA) - DFSNAME_NAME_SIZE)



/*---- STRUCTURE ----
Name : RDDIRHEADER
Purp : Header structure that goes on the front of every ReadDir return block.
---------------------*/
typedef struct 
{												// Bytes: Description
	RDDIRFLAGS			RddirFlags;			// 00-01: Flags describing state of read directory session
		#define			RDDIR_RESTARTED		0x0001	// bit 0 : Directory was restarted due to modification
		#define			RDDIR_END_OF_LIST		0x0002	// bit 1 : The current block contains the end of the directory list
	U16					EntryCount;			// 02-03: Number of entries in the block
	RDDIRSESSION		SessionID;			// 04-05: Read directory session identifier
	OBJECT_METADATA 	ObjectMetadata;	// 06-07: Object metadata settings for output structures
} RDDIRHEADER;
#define	RDDIRHEADER_SIZE	(sizeof (RDDIRHEADER))


/*---- STRUCTURE ----
Name : RDDIRENTRY
Purp : Structure used by READDIR to when USE_OBJECT_METADATA flag is cleared.
---------------------*/
typedef struct 
{												// Bytes: Description
	U16				EntrySize;	  		// 00-01: Size of this entry in bytes.
	DFSATTRIB		Attributes;	  		// 02-03: Entry attributes (including type of object)
	DFSHANDLE		DfsHandle;	  		// 04-07: Handle of object
	DFSNAME			Name;					// 08-xx: DFS name structure
} RDDIRENTRY;
// Define size of RDDIRENTRY for a zero length name
#define	RDDIRENTRY_SIZE_NO_NAME	(sizeof (RDDIRENTRY) - DFSNAME_NAME_SIZE)


/*---- STRUCTURE ----
Name : GET_USER_METADATA_DATA
Purp : Return structure for GET_USER_METADATA
---------------------*/
typedef struct 
{												// Bytes: Description
	U16			UserMetadataLength;	// 00-01: Number of bytes in the metadata to follow
	U8				UserMetadata[USER_METADATA_MAX_LENGTH];	// 02-xx: User metadata
} GET_USER_METADATA_DATA;
#define	GET_USER_METADATA_DATA_SIZE					(sizeof (GET_USER_METADATA_DATA))
#define	GET_USER_METADATA_DATA_SIZE_NO_METADATA	(sizeof (GET_USER_METADATA_DATA) - USER_METADATA_MAX_LENGTH)


/*---- STRUCTURE ----
Name : GET_OBJECT_METADATA_DATA_EXAMPLE1
Purp : Example of get object metadata return structure.
Notes: This structure represents the return structure for the case where ObjectMetadata has the
		 following bits set (this, or similar, might be useful as a replacement for GetFileInfo without the name itself):
			METADATA_ATTRIBUTES + METADATA_PARENT + METADATA_DATASIZE + METADATA_ATTRIBMASK 
			+ METADATA_NAMEINFO + METADATA_CONTENTKEY_STATE		 	
---------------------*/
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	U16					ByteCount;				// Word			Number of bytes in the output structure
	DFSATTRIB			Attributes;				// Word			Attributes of object
	DFSHANDLE			ParentHandle;			// DoubleWord	Handle of parent
	U64					DataSize;				// QuadWord		Size of data for the object
	DFSATTRIBMASK		AttribMask;				// Word			Attribute Modification Mask value of object
	NAMEINFO				NameInfo;				// Word			Name information for object (length & character set)
	CONTENTKEY_STATE	ContentKeyState;		// Word			State of ContentKey for the object
} GET_OBJECT_METADATA_DATA_EX1;

/*---- STRUCTURE ----
Name : GET_OBJECT_METADATA_DATA_EXAMPLE2
Purp : Example of get object metadata return structure.
Notes: This structure represents the return structure for the case where ObjectMetadata has the
		 following bits set (this, or similar, might be useful for a DFSCMD_READDIR command, if the name itself is not desired):
			METADATA_ATTRIBUTES + METADATA_HANDLE + METADATA_DATASIZE + METADATA_ATTRIBMASK 
			+ METADATA_NAMEINFO + METADATA_MIMELENGTH + METADATA_CONTENTKEY_STATE		 	
---------------------*/
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	U16					ByteCount;				// Word			Number of bytes in the output structure
	DFSATTRIB			Attributes;				// Word			Attributes of object
	DFSHANDLE			ObjectHandle;			// DoubleWord	Handle of object
	U64					DataSize;				// QuadWord		Size of data for the object
	DFSATTRIBMASK		AttribMask;				// Word			Attribute Modification Mask value of object
	NAMEINFO				NameInfo;				// Word			Name information for object (length & character set)
	U8						MimeLength;				// Byte			Mime string length
	U8						AlignmentPad;			//
	CONTENTKEY_STATE	ContentKeyState;		// Word			State of ContentKey for the object
} GET_OBJECT_METADATA_DATA_EX2;

/*---- STRUCTURE ----
Name : GET_OBJECT_METADATA_DATA_EXAMPLE3
Purp : Example of get object metadata return structure.
Notes: This structure represents the return structure for the case where ObjectMetadata has the
		 following bits set: (This might be useful to retrieve the name only)
		 		METADATA_DFSNAME
---------------------*/
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	U16					ByteCount;				// Word			Number of bytes in the output structure
	DFSNAME				DfsName;					// Word			The DFSNAME structure (including NameInfo and Name elements)
} GET_OBJECT_METADATA_DATA_EX3;


/*---- STRUCTURE ----
Name : GET_OBJECT_METADATA_DATA_ALL_FIXED
Purp : Example of get object metadata return structure.
Notes: This structure represents the return structure for the case where ObjectMetadata has the
		 following bits set (this represents all the fixed length data):
			ObjectMetadata = METADATA_ATTRIBUTES + METADATA_HANDLE + METADATA_PARENT + METADATA_LASTMODTIME + METADATA_DATASIZE 
			+ METADATA_ATTRIBMASK + METADATA_NAMEINFO + METADATA_MIMELENGTH 
			+ METADATA_CONTENTKEY_STATE + METADATA_CONTENTKEY_COPY_COUNT + METADATA_CONTENTKEY_DRM_COUNT	 	
			+ METADATA_CONTENTKEY_PAD_COUNT
			
			ExtendedObjectMetadata = EXT_METADATA_ORIGINAL_CONTENTKEY_COPY_COUNT + EXT_METADATA_ORIGINAL_CONTENTKEY_DRM_COUNT	
---------------------*/
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	U16					ByteCount;				// Word			Number of bytes in the output structure
	DFSATTRIB			Attributes;				// Word			Attributes of object
	DFSHANDLE			ObjectHandle;			// DoubleWord	Handle of object
	DFSHANDLE			ParentHandle;			// DoubleWord	Handle of parent
	DFSTIME				LastModTime;			// DoubleWord	Last modification time for object
	U64					DataSize;				// QuadWord		Size of data for the object
	DFSATTRIBMASK		AttribMask;				// Word			Attribute Modification Mask value of object
	NAMEINFO				NameInfo;				// Word			Name information for object
	U8						MimeLength;				// Byte			Mime string length
	U8						AlignmentPad1;			//
	CONTENTKEY_STATE	ContentKeyState;		// Word			State of ContentKey for the object
	U8						ContentKeyCopyCount;	// Byte			Number of remaining copies available for object
	U8						ContentKeyDrmCount;	// Byte			Number of remaining DRM copies available for object
	U8						ContentKeyPadCount;	// Byte			Number of pad bytes in the file for encrytpion
	U8						ContentKeyOriginalCopyCount;	//Byte Number of original ContentKey copies
	U8						ContentKeyOriginalDrmCount;	//Byte Number of original DRM copies
} GET_OBJECT_METADATA_DATA_ALL_FIXED;

#define MAXIMUM_GET_OBJECT_METADATA_DATA_SIZE	(sizeof (GET_OBJECT_METADATA_DATA_ALL_FIXED) + sizeof (DFSNAME) + sizeof (DFSMIME))
#define GET_OBJECT_METADATA_OVERHEAD				(sizeof (U16))				// ByteCount is always there


/*---- STRUCTURE ----
Name : GET_MEDIA_METADATA_DATA_EXAMPLE1
Purp : Example of get media metadata return structure.
Notes: This structure represents the return structure for the case where MediaMetadata has the
		 following bits set (this mimics mimics the data returned by the GetMediaInfo command):
		 	METADATA_ROOT_HANDLE + METADATA_MEDIA_FLAGS + METADATA_TOTAL_BYTES + METADATA_REMAINING_BYTES 
			+ METADATA_ROOT_DFSNAME
---------------------*/
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	DFSHANDLE	RootHandle;						// Doubleword	Handle of root directory
	MEDIAFLAGS	MediaFlags;						// Word			Media flags
	U16			AlignmentPad;					//	
	U64			TotalBytes;						// Quadword		Total bytes on medium
	U64			RemainingBytes;				// Quadword		Remaining writable bytes
	DFSNAME		RootName;						// Word			Name of root directory
} GET_MEDIA_METADATA_DATA_EXAMPLE1;


/*---- STRUCTURE ----
Name : GET_MEDIA_METADATA_DATA_ALL_FIXED
Purp : Example of get media metadata return structure.
Notes: This structure represents the return structure for the case where MediaMetadata has the
		 following bits set (this returns all fixed length data):
---------------------*/
#define	GET_MEDIA_METADATA_DATA_ALL_FIXED_MASK	\
				 	METADATA_ROOT_HANDLE + METADATA_MEDIA_FLAGS + METADATA_ROOT_NAMEINFO + METADATA_TOTAL_BYTES \
					+ METADATA_REMAINING_BYTES + METADATA_CONTENT_SIDE_ID + METADATA_UNIQUE_MEDIA_ID \
					+ METADATA_CONTENT_COLLECTION_ID + METADATA_DATA_BLOCK_SIZE
typedef struct 
{														// Alignment	Description
														// ---------   -----------
	DFSHANDLE	RootHandle;						// Doubleword	Handle of root directory
	MEDIAFLAGS	MediaFlags;						// Word			Media flags
	NAMEINFO		NameInfo;						// Word			Name information
	U64			TotalBytes;						// Quadword		Total bytes on medium
	U64			RemainingBytes;				// Quadword		Remaining writable bytes
	U8				ContentSideId[6];				// Word			Mastered side identifier
	U128			CollectionContentId;			// word			Collection Content ID
	U128			UniqueMediaId;					// Word			Unique media identifier (don't use unless really needed - see documentation)
	U16			DataBlockSize;					// Word			Block size of media, in bytes
} GET_MEDIA_METADATA_DATA_ALL_FIXED;

#define MAXIMUM_GET_MEDIA_METADATA_DATA_SIZE	(sizeof (GET_MEDIA_METADATA_DATA_ALL_FIXED) + sizeof (DFSNAME))

#endif	// of #ifndef __dpi_host_h__
