/*
 * Copyright (c) 2001 Gracenote.
 *
 * This software may not be used in any way or distributed without
 * permission. All rights reserved.
 *
 * Some code herein may be covered by US and international patents.
 */

/*
 *	gn_error_code_strings.c - Repository of error strings. 
 */


/*
 * Dependencies.
 */

#include <extras/cddb/gn_error_codes.h>
#include <extras/cddb/gn_errors.h>


/*
 * Typedefs
 */

/* Maps error code to string */
typedef struct gn_error_desc
{
	gn_uint16_t		code;
	gn_cstr_t		string;
}
gn_error_desc_t;


/*
 * Globals
 */

/* Descriptive text of each functional package supported by eCDDB */
static
gn_error_desc_t package_code_strings[] = {

	BASE_GNERR_PKG_ID,					"",
	GNPKG_Abstraction,					"Abstraction Layer",
	GNPKG_DBEngine,						"Database Engine",
	GNPKG_XML,							"MicroXML Library",
	GNPKG_Translator,					"Data Structure Translator",
	GNPKG_EmbeddedDB,					"Embedded Database",
	GNPKG_TOCLookup,					"TOC Lookup",
	GNPKG_Crypto,						"Cryptographic Subsystem",
	GNPKG_Communications,				"Communications Subsystem",
	GNPKG_OnlineProtocol,				"Online Protocol",
	GNPKG_DynBuf,						"Dynamic Buffer",
	GNPKG_Updater,						"Updater Subsystem",
	GNPKG_Utils,						"Utility Package",
	GNPKG_System,						"System Initialization",
	GNPKG_EmbeddedDBIM,					"Database Integrity Manager",
	GNPKG_LocalDataCache,				"Local Data Cache",
	GNPKG_MemoryMgr,					"Memory Manager",
	GNPKG_FileSystem,					"File Subsystem",

	0,									GN_NULL

};

/* Descriptive text of each error code (not value) supported by eCDDB */
static
gn_error_desc_t error_code_strings[] = {

	GNERR_NoError,						"No error",
	GNERR_NoMemory,						"Memory allocation failure",
	GNERR_NotFound,						"Item not found",
	GNERR_IOError,						"Error reading or writing",
	GNERR_InvalidArg,					"Invalid argument",
	GNERR_Busy,							"Busy", /*"System is busy",*/
	GNERR_NotInited,					"Not initialzed", /*"System not initialized",*/
	GNERR_OVERFLOW,						"Result too large",
	GNERR_Unknown,						"Parameter Unknown",
	GNERR_IckyError,					"Bad error that can't be fathomed",

	GNERR_BufferTooSmall,				"Input buffer is too small for available data",
	GNERR_InvalidFormat,				"Invalid format in file or data structure",
	GNERR_InitFailed,					"Initialization failed",
	GNERR_ChecksumMismatch,				"Checksums do not match",
	GNERR_UnknownChecksumAlg,			"Unknown checksum algorithm",
	GNERR_MemLeak,						"Memory Leak",
	GNERR_MemCorrupt,					"Memory Corrupt",
	GNERR_MemInvalid,					"Invalid Memory",
	GNERR_MemInvalidHeap,				"Invalid Memory heap",
	GNERR_MemNoFreeHeaps,				"No free Heaps",
	GNERR_MemInvalidHeapSize,			"Invalid heap size",

	GNERR_FileInvalidAccess,			"Invalid file access",
	GNERR_FileNotFound,					"File not found",
	GNERR_FileExists,					"File already exists",
	GNERR_FileNoSpace,					"No space left on device",
	GNERR_FileTooManyOpen,				"Too many files open",
	GNERR_FileInvalidHandle,			"Invalid file handle",
	GNERR_FileInvalidName,				"Invalid file name",
	GNERR_EOF,							"Reached End of file",

	GNERR_TimerUnavailable,				"Timer unavailable",
	GNERR_LogFull,						"Log full",
	GNERR_ConfigWrongData,				"Wrong value of configuration data",
	GNERR_ConfigReadOnly,				"Configuration data can't be written",

	GNERR_CacheDataError,				"Error in data",
	GNERR_CacheInitedForLookups,		"Cache is initialized for lookups",
	GNERR_CacheInitedForUpdates,		"Cache is initialized for updates",
	GNERR_NoCache,						"There is no cache",

	GNERR_PlatformInitFailed,			"The platform-specific initialization failed",
	GNERR_PlatformShutdownFailed,		"The platform-specific shutdown failed",
	GNERR_NoMoreConnections,			"No more connections are available",
	GNERR_CommInvalidAddress,			"The address is invalid",
	GNERR_CommInvalidHandle,			"The connection handle is invalid",
	GNERR_Unsupported,					"The requested connection type is unsupported",
	GNERR_Timeout,						"The connection timed out",
	GNERR_HTTPClientError,				"HTTP Client Error",
	GNERR_HTTPServerError,				"HTTP Server Error",
	GNERR_HTTPCancelled,				"HTTP Cancelled",
	GNERR_ConnectionRefused,			"The connection was refused by the server",

	GNERR_UnsupportedAlg,				"Unsupported cryptographic algorithm",

	GNERR_CorruptData,					"Data is possibly corrupt (CRC calculation failure)",
	GNERR_InvalidData,					"Database is invalid for this program",
	GNERR_LostBackup,					"Backup database has been lost",
	GNERR_NoLiveUpdateDB,				"The live update DB does not exist",
	GNERR_NoBackupUpdateDB,				"The backup update DB does not exist",
	GNERR_NoUpdateDB,					"The update DB does not exist",
	GNERR_BadUpdateDB,					"There is an error with the update DB",
	GNERR_BackupOutOfSync,				"The backup files are out of sync",
	GNERR_CoreDBError,					"There is an error with the core DB files",
	GNERR_LiveUpdateDBError,			"There is an error with the live update DB files",
	GNERR_BackupUpdateDBError,			"There is an error with the backup update DB files",
	GNERR_UpdateDBError,				"There is an error with the update DB files",
	GNERR_NoLiveCache,					"The live cache does not exist",
	GNERR_NoBackupCache,				"The backup cache does not exist",
	GNERR_LiveCacheError,				"There is an error with the live cache",
	GNERR_BackupCacheError,				"There is an error with the backup cache",
	GNERR_CacheError,					"There is an error with the cache",
	GNERR_LiveUpdateCRCError,			"Live update database CRC error",
	GNERR_BackupUpdateCRCError,			"Backup update database CRC error",
	GNERR_LiveCacheCRCError,			"Live cache CRC error",
	GNERR_BackupCacheCRCError,			"Backup cache CRC error",
	GNERR_NotInitializedForUpdates,		"The system is not initialized for updates",
	GNERR_InvalidDataFormatVersion,		"The database file format is invalid",
	GNERR_CodeDataMismatch,				"The database files cannot be used with this software",
	GNERR_InvalidUniversalDataFormat,	"The database file header format is invalid",

	GNERR_InvalidTOC,					"Invalid TOC",
	GNERR_IDNotFound,					"ID not found",
	GNERR_BadResultFormat,				"Bad 'result' format",

	GNERR_BadXMLFormat,					"Bad XML format",

/*	GNERR_BadResultFormat,				"Badly formed XML", */
	GNERR_IncompleteUpdate,				"Incomplete package update. Refer to detailed errors.",

	GNERR_XMLSyntaxError,				"XML syntax error",
	GNERR_IllegalXMLCharacter,			"Illegal XML character error",
	GNERR_XMLUnexpectedEndOfInput,		"Unexpected end of XML input",

	GNERR_NestedCall,					"Trying to make nested call",
	GNERR_PermissionError,				"Permission error",
	GNERR_ReadErr,						"Error reading db file",
	GNERR_WriteErr,						"Error writing db file",
	GNERR_UnknownVersion,				"Unknown database version",
	GNERR_InvalidFile,					"Invalid file",
	GNERR_AlreadyAdded,					"Record already added",
	GNERR_InvalidBlock,					"Invalid block read",
	GNERR_Aborted,						"Fetch operation aborted",
/*	GNERR_FileExists,					"File already exists", */
	GNERR_NoSpace,						"No space for file",
	GNERR_BadIndex,						"Invalid index node",
	GNERR_BadFreeList,					"Corrupt free list",
	GNERR_BadRecord,					"Corrupt db record",
	GNERR_BadBlock,						"Corrupt db hash block",
	GNERR_IndexPastEOF,					"Index offset past EOF",
	GNERR_RecordPastEOF,				"Record/Block offset past EOF",
	GNERR_IndexInFreeList,				"Index in free list",
	GNERR_RecordInFreeList,				"Record in free list",
	GNERR_BlockInFreeList,				"Hash block in free list",

	GNERR_DataError,					"Error in data",
	GNERR_Decrypt,						"Error decrypting record",
	GNERR_DecryptFmt,					"Unknown encryption format",
	GNERR_UnComp,						"Error uncompressing record data",
	GNERR_UnCompFmt,					"Unknown compression format",
	GNERR_InitedForLookups,				"Database initialized for lookups",
	GNERR_InitedForUpdates,				"Database initialized for updates",
	GNERR_ErrorWritingData,				"Error writing data",
	GNERR_ErrorWritingIndex,			"Error writing index",
	GNERR_NoRecordActive,				"No active data record",

	GNERR_NotRegistered,				"Not registered",
	GNERR_WrongServerPublicKey,			"Wrong Server Public Key",
	GNERR_UnknownCompression,			"Unknown Compression",
	GNERR_UnknownEncryption,			"Unknown Encryption",
	GNERR_UnknownEncoding,				"Unknown Encoding",
	GNERR_NotCDDBMSG,					"Not CDDBMSG",
	GNERR_NoPROTOCOLElement,			"No PROTOCOL Element",
	GNERR_NoSTATUSElement,				"No STATUS Element",
	GNERR_NoDATAElement,				"No DATA Element",
	GNERR_NoDataContents,				"No Data Contents",
	GNERR_NoENCODINGElement,			"No ENCODING Element",
	GNERR_NoENCRYPTIONElement,			"No ENCRYPTION Element",
	GNERR_NoCOMPRESSIONElement,			"No COMPRESSION Element",
	GNERR_NotRESULTS,					"Not RESULTS",
	GNERR_RESULTNotFound,				"RESULT Not Found",
	GNERR_Base64Decode,					"Base64 Decode Error",
	GNERR_Base64Encode,					"Base64 Encode Error",
	GNERR_ZipCompress,					"Zip Compress Error",
	GNERR_ZipUncompress,				"Zip Uncompress Error",
	GNERR_WrongEncryptionStep,			"Wrong Encryption Step",
	GNERR_MissingCLNTKEY,				"Missing CLNTKEY",
	GNERR_MissingSRVRKEY,				"Missing SRVRKEY",
	GNERR_MissingKeyAlgorithm,			"Missing Key Algorithm",
	GNERR_UnknownKeyAlgorithm,			"Unknown Key Algorithm",
	GNERR_WrongClientPublicKey,			"Wrong Client Public Key",
	GNERR_MissingKeyValue,				"Missing Key Value",
	GNERR_NoServerResponse,				"No Server Response",
	GNERR_BadRegResultFormat,			"Bad Reg Result Format",
	GNERR_KeyTooLarge,					"Key Too Large",
	GNERR_MissingSTATUSCODE,			"Missing STATUS CODE",
	GNERR_ServerError,					"Server Error",
	GNERR_NotRESULT,					"Not RESULT",
	GNERR_MissingRESULTCODE,			"Missing RESULT CODE",
	GNERR_NoTransaction,				"No Transaction",
	GNERR_CRCError,						"CRC Error",

	0,									GN_NULL

};


/*
 * Implementations
 */

gn_cstr_t 
gnerr_get_code_desc(gn_uint16_t error_code)
{
	gn_uint32_t	i = 0;

	for (i = 0; error_code_strings[i].string != GN_NULL; i++)
	{
		if (error_code_strings[i].code == error_code)
			return error_code_strings[i].string;
	}

	return GN_NULL;
}


gn_cstr_t
gnerr_get_package_desc(gn_uint16_t package_id)
{
	gn_uint32_t	i = 0;

	for (i = 0; package_code_strings[i].string != GN_NULL; i++)
	{
		if (package_code_strings[i].code == package_id)
			return package_code_strings[i].string;
	}

	return GN_NULL;
}
