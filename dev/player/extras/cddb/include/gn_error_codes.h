/*
* Copyright (c) 2001 Gracenote.
*
* This software may not be used in any way or distributed without
* permission. All rights reserved.
*
* Some code herein may be covered by US and international patents.
*/

/*
* gn_error_codes.h - Package code definitions and string access.
*/

/*
 * Note: all values are expressed in hexadecimal form.
 */


#ifndef	_GN_ERROR_CODES_H_
#define _GN_ERROR_CODES_H_

/*
* Dependencies
*/

#include <extras/cddb/gn_defines.h>


#ifdef __cplusplus
extern "C"{
#endif


/*
* Constants
*/

/* Package/library identifiers. */
#define BASE_GNERR_PKG_ID				0

#define GNPKG_Generic					(BASE_GNERR_PKG_ID+0x00)
#define GNPKG_Abstraction				(BASE_GNERR_PKG_ID+0x01)
#define GNPKG_DBEngine					(BASE_GNERR_PKG_ID+0x02)
#define GNPKG_XML						(BASE_GNERR_PKG_ID+0x03)
#define GNPKG_Translator				(BASE_GNERR_PKG_ID+0x04)
#define GNPKG_EmbeddedDB				(BASE_GNERR_PKG_ID+0x05)
#define GNPKG_TOCLookup					(BASE_GNERR_PKG_ID+0x06)
#define GNPKG_Crypto					(BASE_GNERR_PKG_ID+0x07)
#define GNPKG_Communications			(BASE_GNERR_PKG_ID+0x08)
#define GNPKG_OnlineProtocol			(BASE_GNERR_PKG_ID+0x09)
#define GNPKG_DynBuf					(BASE_GNERR_PKG_ID+0x0A)
#define GNPKG_Updater					(BASE_GNERR_PKG_ID+0x0B)
#define GNPKG_Utils						(BASE_GNERR_PKG_ID+0x0C)
#define GNPKG_System					(BASE_GNERR_PKG_ID+0x0D)
#define GNPKG_EmbeddedDBIM				(BASE_GNERR_PKG_ID+0x0E)
#define GNPKG_LocalDataCache			(BASE_GNERR_PKG_ID+0x0F)
#define GNPKG_MemoryMgr					(BASE_GNERR_PKG_ID+0x10)
#define GNPKG_FileSystem				(BASE_GNERR_PKG_ID+0x11)

#define	MAX_GNERR_PKG_ID				BASE_GNERR_PKG_ID+0x11


/* Error Codes */
/*
 * Codes are grouped by the package that usually (if not exclusively)
 * returns the code. For example, GNERR_MemLeak is grouped with the
 * Abstraction Layer codes.
 * Values for each group of error codes begin with a multiple of 0x40.
 * This allows for numbering consistency when defining new codes within
 * a group. Note that this scheme is motivated by aesthetic (not functional)
 * reasons; error code origination is determined by the package id field
 * of the error value, not by the value of the error code field.
 */

#define	BASE_GNERR_CODE					0

/* General Errors */
#define	GNERR_NoError					(BASE_GNERR_CODE)
#define	GNERR_NoMemory					(BASE_GNERR_CODE+0x0001)	/* Memory allocation failure */
#define	GNERR_NotFound					(BASE_GNERR_CODE+0x0002)	/* Item not found */
#define	GNERR_IOError					(BASE_GNERR_CODE+0x0003)	/* Error reading or writing */
#define	GNERR_InvalidArg				(BASE_GNERR_CODE+0x0004)	/* Invalid argument */
#define	GNERR_Busy						(BASE_GNERR_CODE+0x0005)	/* System is busy */
#define	GNERR_NotInited					(BASE_GNERR_CODE+0x0006)	/* System not initialized */
#define	GNERR_OVERFLOW					(BASE_GNERR_CODE+0x0007)	/* Result too large */
#define	GNERR_Unknown					(BASE_GNERR_CODE+0x0008)	/* Unknown Parameter */
#define	GNERR_IckyError					(BASE_GNERR_CODE+0x0009)	/* Really, really bad error which can't be fathomed */
#define	GNERR_BufferTooSmall			(BASE_GNERR_CODE+0x000A)	/* Input buffer is too small for available data */
#define	GNERR_InvalidFormat				(BASE_GNERR_CODE+0x000B)	/* Invalid format in file or data structure */
#define	GNERR_InitFailed				(BASE_GNERR_CODE+0x000C)
#define	GNERR_UnknownChecksumAlg		(BASE_GNERR_CODE+0x000D)
#define	GNERR_ChecksumMismatch			(BASE_GNERR_CODE+0x000E)

/* Abstraction Layer */
#define	GNERR_MemLeak					(BASE_GNERR_CODE+0x0041)
#define	GNERR_MemCorrupt				(BASE_GNERR_CODE+0x0042)
#define	GNERR_MemInvalid				(BASE_GNERR_CODE+0x0043)
#define	GNERR_MemInvalidHeap			(BASE_GNERR_CODE+0x0044)
#define	GNERR_MemNoFreeHeaps			(BASE_GNERR_CODE+0x0045)
#define	GNERR_MemInvalidHeapSize		(BASE_GNERR_CODE+0x0046)
#define	GNERR_FileInvalidAccess			(BASE_GNERR_CODE+0x0047)
#define	GNERR_FileNotFound				(BASE_GNERR_CODE+0x0048)
#define	GNERR_FileExists				(BASE_GNERR_CODE+0x0049)
#define	GNERR_FileNoSpace				(BASE_GNERR_CODE+0x004A)
#define	GNERR_FileTooManyOpen			(BASE_GNERR_CODE+0x004B)
#define	GNERR_FileInvalidHandle			(BASE_GNERR_CODE+0x004C)
#define	GNERR_FileInvalidName			(BASE_GNERR_CODE+0x004D)
#define	GNERR_EOF						(BASE_GNERR_CODE+0x004E)
#define	GNERR_TimerUnavailable			(BASE_GNERR_CODE+0x004F)
#define	GNERR_LogFull					(BASE_GNERR_CODE+0x0050)
#define	GNERR_ConfigWrongData			(BASE_GNERR_CODE+0x0051)
#define	GNERR_ConfigReadOnly			(BASE_GNERR_CODE+0x0052)

/* Local Data Cache */
#define	GNERR_CacheDataError			(BASE_GNERR_CODE+0x0081)
#define	GNERR_CacheInitedForLookups		(BASE_GNERR_CODE+0x0082)
#define	GNERR_CacheInitedForUpdates		(BASE_GNERR_CODE+0x0083)
#define	GNERR_NoCache					(BASE_GNERR_CODE+0x0084)

/* Remote Communications Management */
#define	GNERR_PlatformInitFailed		(BASE_GNERR_CODE+0x0121)
#define	GNERR_PlatformShutdownFailed	(BASE_GNERR_CODE+0x0122)
#define	GNERR_NoMoreConnections			(BASE_GNERR_CODE+0x0123)
#define	GNERR_CommInvalidAddress		(BASE_GNERR_CODE+0x0124)
#define	GNERR_CommInvalidHandle			(BASE_GNERR_CODE+0x0125)
#define	GNERR_Unsupported				(BASE_GNERR_CODE+0x0126)
#define	GNERR_Timeout					(BASE_GNERR_CODE+0x0127)
#define	GNERR_HTTPClientError			(BASE_GNERR_CODE+0x0128)
#define	GNERR_HTTPServerError			(BASE_GNERR_CODE+0x0129)
#define	GNERR_HTTPCancelled				(BASE_GNERR_CODE+0x012A)
#define	GNERR_ConnectionRefused			(BASE_GNERR_CODE+0x012B)

/* Crytographic Subsystem */
#define	GNERR_UnsupportedAlg			(BASE_GNERR_CODE+0x0161)

/* Embedded Database Integrity Management */
#define	GNERR_CorruptData				(BASE_GNERR_CODE+0x0201)
#define	GNERR_InvalidData				(BASE_GNERR_CODE+0x0202)
#define	GNERR_LostBackup				(BASE_GNERR_CODE+0x0203)
#define	GNERR_NoLiveUpdateDB			(BASE_GNERR_CODE+0x0205)
#define	GNERR_NoBackupUpdateDB			(BASE_GNERR_CODE+0x0206)
#define	GNERR_NoUpdateDB				(BASE_GNERR_CODE+0x0207)
#define	GNERR_BadUpdateDB				(BASE_GNERR_CODE+0x0208)
#define	GNERR_BackupOutOfSync			(BASE_GNERR_CODE+0x0209)
#define	GNERR_CoreDBError				(BASE_GNERR_CODE+0x020A)
#define	GNERR_LiveUpdateDBError			(BASE_GNERR_CODE+0x020B)
#define	GNERR_BackupUpdateDBError		(BASE_GNERR_CODE+0x020C)
#define	GNERR_UpdateDBError				(BASE_GNERR_CODE+0x020D)
#define	GNERR_NoLiveCache				(BASE_GNERR_CODE+0x020E)
#define	GNERR_NoBackupCache				(BASE_GNERR_CODE+0x020F)
#define	GNERR_LiveCacheError			(BASE_GNERR_CODE+0x0210)
#define	GNERR_BackupCacheError			(BASE_GNERR_CODE+0x0211)
#define	GNERR_CacheError				(BASE_GNERR_CODE+0x0212)
#define	GNERR_LiveUpdateCRCError		(BASE_GNERR_CODE+0x0213)
#define	GNERR_BackupUpdateCRCError		(BASE_GNERR_CODE+0x0214)
#define	GNERR_LiveCacheCRCError			(BASE_GNERR_CODE+0x0215)
#define	GNERR_BackupCacheCRCError		(BASE_GNERR_CODE+0x0216)
#define	GNERR_NotInitializedForUpdates	(BASE_GNERR_CODE+0x0217)
#define	GNERR_InvalidDataFormatVersion	(BASE_GNERR_CODE+0x0218)
#define	GNERR_CodeDataMismatch			(BASE_GNERR_CODE+0x0219)
#define	GNERR_InvalidUniversalDataFormat	(BASE_GNERR_CODE+0x021A)

/* 7 TOC Lookup Package */
#define	GNERR_InvalidTOC				(BASE_GNERR_CODE+0x0241)
#define	GNERR_IDNotFound				(BASE_GNERR_CODE+0x0242)
#define	GNERR_BadResultFormat			(BASE_GNERR_CODE+0x0243)

/* Data Translator */
#define	GNERR_BadXMLFormat				(BASE_GNERR_CODE+0x0281)

/* Updater Subsystem */
#define	GNERR_IncompleteUpdate			(BASE_GNERR_CODE+0x0321)

/* Micro XML Package */
#define	GNERR_XMLSyntaxError			(BASE_GNERR_CODE+0x0361)
#define	GNERR_IllegalXMLCharacter		(BASE_GNERR_CODE+0x0362)
#define	GNERR_XMLUnexpectedEndOfInput	(BASE_GNERR_CODE+0x0363)

/* Database Engine */
#define	GNERR_NestedCall				(BASE_GNERR_CODE+0x0401)
#define	GNERR_PermissionError			(BASE_GNERR_CODE+0x0402)
#define	GNERR_ReadErr					(BASE_GNERR_CODE+0x0403)
#define	GNERR_WriteErr					(BASE_GNERR_CODE+0x0404)
#define	GNERR_UnknownVersion			(BASE_GNERR_CODE+0x0405)
#define	GNERR_InvalidFile				(BASE_GNERR_CODE+0x0406)
#define	GNERR_AlreadyAdded				(BASE_GNERR_CODE+0x0407)
#define	GNERR_InvalidBlock				(BASE_GNERR_CODE+0x0408)
#define	GNERR_Aborted					(BASE_GNERR_CODE+0x0409)
#define	GNERR_NoSpace					(BASE_GNERR_CODE+0x040A)
#define	GNERR_BadIndex					(BASE_GNERR_CODE+0x040B)
#define	GNERR_BadFreeList				(BASE_GNERR_CODE+0x040C)
#define	GNERR_BadRecord					(BASE_GNERR_CODE+0x040D)
#define	GNERR_BadBlock					(BASE_GNERR_CODE+0x040E)
#define	GNERR_IndexPastEOF				(BASE_GNERR_CODE+0x040F)
#define	GNERR_RecordPastEOF				(BASE_GNERR_CODE+0x0410)
#define	GNERR_IndexInFreeList			(BASE_GNERR_CODE+0x0411)
#define	GNERR_RecordInFreeList			(BASE_GNERR_CODE+0x0412)
#define	GNERR_BlockInFreeList			(BASE_GNERR_CODE+0x0413)

/* Embedded Database Interface */
#define	GNERR_DataError					(BASE_GNERR_CODE+0x0441)
#define	GNERR_Decrypt					(BASE_GNERR_CODE+0x0442)
#define	GNERR_DecryptFmt				(BASE_GNERR_CODE+0x0443)
#define	GNERR_UnComp					(BASE_GNERR_CODE+0x0444)
#define	GNERR_UnCompFmt					(BASE_GNERR_CODE+0x0445)
#define	GNERR_InitedForLookups			(BASE_GNERR_CODE+0x0446)
#define	GNERR_InitedForUpdates			(BASE_GNERR_CODE+0x0447)
#define	GNERR_ErrorWritingData			(BASE_GNERR_CODE+0x0448)
#define	GNERR_ErrorWritingIndex			(BASE_GNERR_CODE+0x0449)
#define	GNERR_NoRecordActive			(BASE_GNERR_CODE+0x044A)

/* Online Protocol */
#define	GNERR_NotRegistered				(BASE_GNERR_CODE+0x0481)
#define	GNERR_WrongServerPublicKey		(BASE_GNERR_CODE+0x0482)
#define	GNERR_UnknownCompression		(BASE_GNERR_CODE+0x0483)
#define	GNERR_UnknownEncryption			(BASE_GNERR_CODE+0x0484)
#define	GNERR_UnknownEncoding			(BASE_GNERR_CODE+0x0485)
#define	GNERR_NotCDDBMSG				(BASE_GNERR_CODE+0x0486)
#define	GNERR_NoPROTOCOLElement			(BASE_GNERR_CODE+0x0487)
#define	GNERR_NoSTATUSElement			(BASE_GNERR_CODE+0x0488)
#define	GNERR_NoDATAElement				(BASE_GNERR_CODE+0x0489)
#define	GNERR_NoDataContents			(BASE_GNERR_CODE+0x048A)
#define	GNERR_NoENCODINGElement			(BASE_GNERR_CODE+0x048B)
#define	GNERR_NoENCRYPTIONElement		(BASE_GNERR_CODE+0x048C)
#define	GNERR_NoCOMPRESSIONElement		(BASE_GNERR_CODE+0x048D)
#define	GNERR_NotRESULTS				(BASE_GNERR_CODE+0x048E)
#define	GNERR_RESULTNotFound			(BASE_GNERR_CODE+0x048F)
#define	GNERR_Base64Decode				(BASE_GNERR_CODE+0x0490)
#define	GNERR_Base64Encode				(BASE_GNERR_CODE+0x0491)
#define	GNERR_ZipCompress				(BASE_GNERR_CODE+0x0492)
#define	GNERR_ZipUncompress				(BASE_GNERR_CODE+0x0493)
#define	GNERR_WrongEncryptionStep		(BASE_GNERR_CODE+0x0494)
#define	GNERR_MissingCLNTKEY			(BASE_GNERR_CODE+0x0495)
#define	GNERR_MissingSRVRKEY			(BASE_GNERR_CODE+0x0496)
#define	GNERR_MissingKeyAlgorithm		(BASE_GNERR_CODE+0x0497)
#define	GNERR_UnknownKeyAlgorithm		(BASE_GNERR_CODE+0x0498)
#define	GNERR_WrongClientPublicKey		(BASE_GNERR_CODE+0x0499)
#define	GNERR_MissingKeyValue			(BASE_GNERR_CODE+0x049A)
#define	GNERR_NoServerResponse			(BASE_GNERR_CODE+0x049B)
#define	GNERR_BadRegResultFormat		(BASE_GNERR_CODE+0x049C)
#define	GNERR_KeyTooLarge				(BASE_GNERR_CODE+0x049D)
#define	GNERR_MissingSTATUSCODE			(BASE_GNERR_CODE+0x049F)
#define	GNERR_ServerError				(BASE_GNERR_CODE+0x04A0)
#define	GNERR_NotRESULT					(BASE_GNERR_CODE+0x04A1)
#define	GNERR_MissingRESULTCODE			(BASE_GNERR_CODE+0x04A2)
#define	GNERR_NoTransaction				(BASE_GNERR_CODE+0x04A3)
#define	GNERR_CRCError					(BASE_GNERR_CODE+0x04A4)


/*
* Prototypes
*/

/* Return descriptive string associated with error code */
gn_cstr_t
gnerr_get_code_desc(gn_uint16_t error_code);

/* Return descriptive string associated with package id */
gn_cstr_t
gnerr_get_package_desc(gn_uint16_t package_id);


#ifdef __cplusplus
}
#endif

#endif /* _GN_ERROR_CODES_H_ */

