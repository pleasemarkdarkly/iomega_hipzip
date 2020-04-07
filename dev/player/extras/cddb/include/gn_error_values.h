/*
* Copyright (c) 2001 Gracenote.
*
* This software may not be used in any way or distributed without
* permission. All rights reserved.
*
* Some code herein may be covered by US and international patents.
*/

/*
* gn_error_values.h - Error values, used and returned by each package.
*/

#ifndef _GN_ERROR_VALUES_H_
#define _GN_ERROR_VALUES_H_

/*
* Dependencies
*/

/* see gn_errors.h for method of dependency inclusion */


#ifdef __cplusplus
extern "C"{
#endif


/*
* Constants
*/

#define DIAG_PARAM_UNKNOWN					0xFFFF


/* Generic errors */
#define	GENERR_NoError						0
#define	GENERR_NoMemory						GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_NoMemory)
#define	GENERR_NotFound						GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_NotFound)
#define	GENERR_IOError						GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_IOError)
#define	GENERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_InvalidArg)
#define	GENERR_Busy							GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_Busy)
#define	GENERR_NotInited					GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_NotInited)
#define	GENERR_OVERFLOW						GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_OVERFLOW)
#define	GENERR_Unknown						GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_Unknown)
#define	GENERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_IckyError)
#define	GENERR_BufferTooSmall				GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_BufferTooSmall)
#define	GENERR_InvalidFormat				GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_InvalidFormat)
#define	GENERR_InitFailed					GNERR_MAKE_VALUE(GNPKG_Generic, GNERR_InitFailed)


/* Errors returned from the memory management subsystem */
#define MEMERR_NoError						0
#define MEMERR_Busy							GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_Busy)
#define MEMERR_Nomem						GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_NoMemory)
#define MEMERR_Leak							GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemLeak)
#define MEMERR_Noinit						GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_NotInited)
#define MEMERR_Corrupt						GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemCorrupt)
#define MEMERR_Invalid_mem					GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemInvalid)
#define MEMERR_Invalid_mem_heap				GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemInvalidHeap)
#define MEMERR_No_free_heaps				GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemNoFreeHeaps)
#define MEMERR_Invalid_heap_size			GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_MemInvalidHeapSize)
#define MEMERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_MemoryMgr, GNERR_InvalidArg)


/* Errors returned from the file subsystem */
#define FSERR_NoError						0
#define FSERR_InvalidAccess					GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileInvalidAccess)
#define FSERR_NotFound 						GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileNotFound)
#define FSERR_FileExists					GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileExists)
#define FSERR_NoSpace						GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileNoSpace)
#define FSERR_Toomanyopen					GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileTooManyOpen)
#define FSERR_Invalidhandle					GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileInvalidHandle)
#define FSERR_Invalidfilename				GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_FileInvalidName)
#define FSERR_EOF							GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_EOF)
#define FSERR_Busy							GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_Busy)
#define FSERR_NotInited						GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_NotInited)
#define FSERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_InvalidArg)
#define FSERR_IckyError						GNERR_MAKE_VALUE(GNPKG_FileSystem, GNERR_IckyError)


/* Errors returned from the Abstract Layer */
#define ABSTERR_NoError						0
#define ABSTERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_InvalidArg)
#define ABSTERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_IckyError)
#define ABSTERR_NotFound					GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_NotFound)
#define ABSTERR_MEM_Nomem					MEMERR_Nomem
#define ABSTERR_MEM_Leak					MEMERR_Leak
#define ABSTERR_MEM_Noinit					MEMERR_Noinit
#define ABSTERR_MEM_Corrupt					MEMERR_Corrupt
#define ABSTERR_MEM_Invalid_mem				MEMERR_Invalid_mem
#define ABSTERR_MEM_Invalid_mem_heap		MEMERR_Invalid_mem_heap
#define ABSTERR_MEM_No_free_heaps			MEMERR_No_free_heaps
#define ABSTERR_MEM_Invalid_heap_size		MEMERR_Invalid_heap_size
#define ABSTERR_MEM_Busy					MEMERR_Busy
#define ABSTERR_FS_InvalidAccess			FSERR_InvalidAccess
#define ABSTERR_FS_NotFound 				FSERR_NotFound
#define ABSTERR_FS_FileExists				FSERR_FileExists
#define ABSTERR_FS_NoSpace					FSERR_NoSpace
#define ABSTERR_FS_Toomanyopen				FSERR_Toomanyopen
#define ABSTERR_FS_Invalidhandle			FSERR_Invalidhandle
#define ABSTERR_FS_Invalidfilename			FSERR_Invalidfilename
#define ABSTERR_FS_EOF						FSERR_EOF
#define ABSTERR_TIMER_UNAVAIL				GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_TimerUnavailable)
#define ABSTERR_LOG_FULL					GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_LogFull)
#define ABSTERR_DIAG_PARAM_UNKNOWN			GNERR_MAKE_VALUE(GNPKG_Abstraction, DIAG_PARAM_UNKNOWN)
#define ABSTERR_CONF_Wrongdata				GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_ConfigWrongData)
#define ABSTERR_CONF_Readonly				GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_ConfigReadOnly)
#define ABSTERR_Busy						GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_Busy)
#define ABSTERR_NoMemory					GNERR_MAKE_VALUE(GNPKG_Abstraction, GNERR_NoMemory)

/* Errors returned from the DB Engine */
#define DBERR_NoError						0
#define DBERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_InvalidArg)
#define DBERR_NotFound						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_NotFound)
#define DBERR_MemoryErr						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_NoMemory)
#define DBERR_IOErr							GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_IOError)
#define DBERR_NestedCall					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_NestedCall)
#define DBERR_PermissionError				GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_PermissionError)
#define DBERR_ReadErr						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_ReadErr)
#define DBERR_WriteErr						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_WriteErr)
#define DBERR_UnknownVersion				GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_UnknownVersion)
#define DBERR_InvalidFile					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_InvalidFile)
#define DBERR_AlreadyAdded					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_AlreadyAdded)
#define DBERR_InvalidBlock					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_InvalidBlock)
#define DBERR_Aborted						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_Aborted)
#define DBERR_FileExists					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_FileExists)
#define DBERR_NoSpace						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_NoSpace)
#define	DBERR_BadIndex						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_BadIndex)
#define	DBERR_BadFreeList					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_BadFreeList)
#define	DBERR_BadRecord						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_BadRecord)
#define	DBERR_BadBlock						GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_BadBlock)
#define	DBERR_IndexPastEOF					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_IndexPastEOF)
#define	DBERR_RecordPastEOF					GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_RecordPastEOF)
#define	DBERR_IndexInFreeList				GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_IndexInFreeList)
#define	DBERR_RecordInFreeList				GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_RecordInFreeList)
#define	DBERR_BlockInFreeList				GNERR_MAKE_VALUE(GNPKG_DBEngine, GNERR_BlockInFreeList)

/* Errors returned from the micro XML subsystem */
#define kXMLNoError							0
#define kXMLOutOfMemoryError				GNERR_MAKE_VALUE(GNPKG_XML, GNERR_NoMemory)
#define kXMLInvalidParamError				GNERR_MAKE_VALUE(GNPKG_XML, GNERR_InvalidArg)
#define kXMLSyntaxError						GNERR_MAKE_VALUE(GNPKG_XML, GNERR_XMLSyntaxError)
#define kIllegalXMLCharacterError			GNERR_MAKE_VALUE(GNPKG_XML, GNERR_IllegalXMLCharacter)
#define kXMLUnexpectedEndOfInputError		GNERR_MAKE_VALUE(GNPKG_XML, GNERR_XMLUnexpectedEndOfInput)

/* Errors returned from Data Translator subsystem */
#define XLTERR_NoError						0
#define XLTERR_NotInited					GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_NotInited)
#define XLTERR_Busy							GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_Busy)
#define XLTERR_InvalidParam					GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_InvalidArg)
#define XLTERR_OutOfMemory					GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_NoMemory)
#define XLTERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_IckyError)
#define XLTERR_BadFormat					GNERR_MAKE_VALUE(GNPKG_Translator, GNERR_BadXMLFormat)

/* Errors returned from Embedded Database subsystem */
#define EDBERR_NoError						0
#define EDBERR_NotInited					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_NotInited)
#define EDBERR_Busy							GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_Busy)
#define EDBERR_NotFound						GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_NotFound)
#define EDBERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_InvalidArg)
#define EDBERR_MemoryError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_NoMemory)
#define EDBERR_IOError						GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_IOError)
#define EDBERR_DataError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_DataError)
#define EDBERR_Decrypt						GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_Decrypt)
#define EDBERR_DecryptFmt					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_DecryptFmt)
#define EDBERR_UnComp						GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_UnComp)
#define EDBERR_UnCompFmt					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_UnCompFmt)
#define EDBERR_InitedForLookups				GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_InitedForLookups)
#define EDBERR_InitedForUpdates				GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_InitedForUpdates)
#define EDBERR_ErrorWritingData				GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_ErrorWritingData)
#define EDBERR_ErrorWritingIndex			GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_ErrorWritingIndex)
#define EDBERR_NoRecordActive				GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_NoRecordActive)
#define EDBERR_IckyError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDB, GNERR_IckyError)

/* Errors returned from TOC lookup subsystem */
#define TLERR_NoError						0
#define TLERR_NotInited						GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_NotInited)
#define TLERR_Busy							GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_Busy)
#define TLERR_MemoryError					GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_NoMemory)
#define TLERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_InvalidArg)
#define TLERR_NotFound						GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_NotFound)
#define TLERR_Overflow						GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_OVERFLOW)
#define TLERR_IckyError						GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_IckyError)
#define TLERR_InvalidTOC					GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_InvalidTOC)
#define TLERR_IDNotFound					GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_IDNotFound)
#define TLERR_BadResultFormat				GNERR_MAKE_VALUE(GNPKG_TOCLookup, GNERR_BadResultFormat)

/* Errors returned from Cryptographic subsystem */
#define CRYPTERR_NoError					0
#define CRYPTERR_NotInited					GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_NotInited)
#define CRYPTERR_Busy						GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_Busy)
#define CRYPTERR_NotFound					GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_NotFound)
#define CRYPTERR_InvalidArg 				GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_InvalidArg)
#define CRYPTERR_MemoryError				GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_NoMemory)
#define CRYPTERR_IOError					GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_IOError)
#define CRYPTERR_UnsupportedAlg				GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_UnsupportedAlg)
#define CRYPTERR_BufferTooSmall				GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_BufferTooSmall)
#define CRYPTERR_InvalidFormat				GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_InvalidFormat)
#define CRYPTERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Crypto, GNERR_IckyError)

/* Errors returned from the Communication subsystem */
#define COMMERR_NoError 					0
#define COMMERR_NotInited					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_NotInited)
#define COMMERR_Busy						GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_Busy)
#define COMMERR_NotFound					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_NotFound)
#define COMMERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_InvalidArg)
#define COMMERR_NoMemory					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_NoMemory)
#define COMMERR_IOError 					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_IOError)
#define COMMERR_AlreadyInitialized			GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_Busy)
#define COMMERR_PlatformInitFailed			GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_PlatformInitFailed)
#define COMMERR_PlatformShutdownFailed		GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_PlatformShutdownFailed)
#define COMMERR_NoMoreConnections			GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_NoMoreConnections)
#define COMMERR_InvalidAddress				GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_CommInvalidAddress)
#define COMMERR_InvalidHandle				GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_CommInvalidHandle)
#define COMMERR_Unsupported					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_Unsupported)
#define COMMERR_Timeout						GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_Timeout)
#define COMMERR_HTTPClientError				GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_HTTPClientError)
#define COMMERR_HTTPServerError				GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_HTTPServerError)
#define COMMERR_HTTPCancelled				GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_HTTPCancelled)
#define COMMERR_ConnectionRefused			GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_ConnectionRefused)
#define COMMERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Communications, GNERR_IckyError)

/* Errors returned from the online protocol module */
#define OPERR_NoError						0
#define OPERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_InvalidArg)
#define OPERR_NoMemory						GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoMemory)
#define OPERR_Busy							GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_Busy)
#define OPERR_NotRegistered					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NotRegistered)
#define OPERR_WrongServerPublicKey			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_WrongServerPublicKey)
#define OPERR_UnknownCompression			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_UnknownCompression)
#define OPERR_UnknownEncryption				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_UnknownEncryption)
#define OPERR_UnknownEncoding				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_UnknownEncoding)
#define OPERR_NotCDDBMSG					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NotCDDBMSG)
#define OPERR_NoPROTOCOLElement				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoPROTOCOLElement)
#define OPERR_NoSTATUSElement				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoSTATUSElement)
#define OPERR_NoDATAElement					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoDATAElement)
#define OPERR_NoDataContents				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoDataContents)
#define OPERR_NoENCODINGElement				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoENCODINGElement)
#define OPERR_NoENCRYPTIONElement			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoENCRYPTIONElement)
#define OPERR_NoCOMPRESSIONElement			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoCOMPRESSIONElement)
#define OPERR_NotRESULTS					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NotRESULTS)
#define OPERR_RESULTNotFound				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_RESULTNotFound)
#define OPERR_Base64Encode					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_Base64Encode)
#define OPERR_Base64Decode					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_Base64Decode)
#define OPERR_ZipCompress					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_ZipCompress)
#define OPERR_ZipUncompress					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_ZipUncompress)
#define OPERR_WrongEncryptionStep			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_WrongEncryptionStep)
#define OPERR_MissingCLNTKEY				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingCLNTKEY)
#define OPERR_MissingSRVRKEY				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingSRVRKEY)
#define OPERR_MissingKeyAlgorithm			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingKeyAlgorithm)
#define OPERR_UnknownKeyAlgorithm			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_UnknownKeyAlgorithm)
#define OPERR_WrongClientPublicKey			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_WrongClientPublicKey)
#define OPERR_MissingKeyValue				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingKeyValue)
#define OPERR_NoServerResponse				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoServerResponse)
#define OPERR_BadRegResultFormat			GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_BadRegResultFormat)
#define OPERR_KeyTooLarge					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_KeyTooLarge)
#define OPERR_MissingSTATUSCODE				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingSTATUSCODE)
#define OPERR_ServerError					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_ServerError)
#define OPERR_NotRESULT						GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NotRESULT)
#define OPERR_MissingRESULTCODE				GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_MissingRESULTCODE)
#define OPERR_NoTransaction					GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_NoTransaction)
#define OPERR_CRCError						GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_CRCError)
#define OPERR_IckyError						GNERR_MAKE_VALUE(GNPKG_OnlineProtocol, GNERR_IckyError)

/* Errors returned from the Dynamic Buffer package */
#define DYN_BUF_NO_ERROR					0
#define DYN_BUF_OUT_OF_MEMORY_ERROR			GNERR_MAKE_VALUE(GNPKG_DynBuf, GNERR_NoMemory)
#define DYN_BUF_INVALID_PARAM_ERROR			GNERR_MAKE_VALUE(GNPKG_DynBuf, GNERR_InvalidArg)

/* Errors returned from the Updater subsystem */
#define UPDERR_NoError						0
#define UPDERR_NotInited					GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_NotInited)
#define UPDERR_Busy							GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_Busy)
#define UPDERR_NotFound						GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_NotFound)
#define UPDERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_InvalidArg)
#define UPDERR_NoMemory						GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_NoMemory)
#define UPDERR_IOError						GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_IOError)
#define UPDERR_InvalidFormat				GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_InvalidFormat)
#define UPDERR_BadResultFormat				GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_BadResultFormat)
#define UPDERR_IncompleteUpdate				GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_IncompleteUpdate)
#define UPDERR_IckyError					GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_IckyError)
#define UPDERR_FileNotFound					GNERR_MAKE_VALUE(GNPKG_Updater, GNERR_FileNotFound)

/* Errors returned from the utilities package */
#define UTILERR_NoError						0
#define UTILERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_Utils, GNERR_InvalidArg)
#define UTILERR_NoMemory					GNERR_MAKE_VALUE(GNPKG_Utils, GNERR_NoMemory)
#define UTILERR_UnknownChecksumAlg			GNERR_MAKE_VALUE(GNPKG_Utils, GNERR_UnknownChecksumAlg)
#define UTILERR_ChecksumMismatch			GNERR_MAKE_VALUE(GNPKG_Utils, GNERR_ChecksumMismatch)

/* Errors returned from the "system" package */
#define SYSERR_NoError						0
#define SYSERR_NotInited					GNERR_MAKE_VALUE(GNPKG_System, GNERR_NotInited)
#define SYSERR_Busy							GNERR_MAKE_VALUE(GNPKG_System, GNERR_Busy)
#define SYSERR_MemoryError					GNERR_MAKE_VALUE(GNPKG_System, GNERR_NoMemory)
#define SYSERR_InitError					GNERR_MAKE_VALUE(GNPKG_System, GNERR_InitFailed)

/* Errors returned from Integrity Manager */
#define EDBIMERR_NoError					0
#define EDBIMERR_NoMemory					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoMemory)
#define EDBIMERR_NotFound					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NotFound)
#define EDBIMERR_IOError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_IOError)
#define EDBIMERR_InvalidArg					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_InvalidArg)
#define EDBIMERR_Busy						GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_Busy)
#define EDBIMERR_NotInited					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NotInited)
#define EDBIMERR_OVERFLOW					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_OVERFLOW)
#define EDBIMERR_Unknown					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_Unknown)
#define EDBIMERR_CorruptData				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_CorruptData)
#define EDBIMERR_InvalidFormat				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_InvalidFormat)
#define EDBIMERR_InvalidData				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_InvalidData)
#define EDBIMERR_LostBackup					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_LostBackup)
#define EDBIMERR_IckyError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_IckyError)
#define EDBIMERR_CRCError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_CRCError)
#define EDBIMERR_NoLiveUpdateDB				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoLiveUpdateDB)
#define EDBIMERR_NoBackupUpdateDB			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoBackupUpdateDB)
#define EDBIMERR_NoUpdateDB					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoUpdateDB)
#define EDBIMERR_BadUpdateDB				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BadUpdateDB)
#define EDBIMERR_BackupOutOfSync			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BackupOutOfSync)
#define EDBIMERR_CoreDBError				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_CoreDBError)
#define EDBIMERR_LiveUpdateDBError			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_LiveUpdateDBError)
#define EDBIMERR_BackupUpdateDBError		GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BackupUpdateDBError)
#define EDBIMERR_UpdateDBError				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_UpdateDBError)
#define EDBIMERR_NoLiveCache				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoLiveCache)
#define EDBIMERR_NoBackupCache				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NoBackupCache)
#define EDBIMERR_LiveCacheError				GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_LiveCacheError)
#define EDBIMERR_BackupCacheError			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BackupCacheError)
#define EDBIMERR_CacheError					GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_CacheError)
#define EDBIMERR_LiveUpdateCRCError			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_LiveUpdateCRCError)
#define EDBIMERR_BackupUpdateCRCError		GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BackupUpdateCRCError)
#define EDBIMERR_LiveCacheCRCError			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_LiveCacheCRCError)
#define EDBIMERR_BackupCacheCRCError		GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_BackupCacheCRCError)
#define EDBIMERR_NotInitializedForUpdates	GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_NotInitializedForUpdates)
#define EDBIMERR_InvalidDataFormatVersion	GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_InvalidDataFormatVersion)
#define EDBIMERR_CodeDataMismatch			GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_CodeDataMismatch)
#define EDBIMERR_InvalidUniversalDataFormat	GNERR_MAKE_VALUE(GNPKG_EmbeddedDBIM, GNERR_InvalidUniversalDataFormat)

/* Errors returned from Local Data Cache subsystem */
#define CACHE_ERR_NoError					0
#define CACHE_ERR_NotInited					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_NotInited)
#define CACHE_ERR_Busy						GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_Busy)
#define CACHE_ERR_NotFound					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_NotFound)
#define CACHE_ERR_InvalidArg				GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_InvalidArg)
#define CACHE_ERR_MemoryError				GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_NoMemory)
#define CACHE_ERR_IOError					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_IOError)
#define CACHE_ERR_DataError					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_CacheDataError)
#define CACHE_ERR_InitedForLookups			GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_CacheInitedForLookups)
#define CACHE_ERR_InitedForUpdates			GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_CacheInitedForUpdates)
#define CACHE_ERR_IckyError					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_IckyError)
#define CACHE_ERR_NoCache					GNERR_MAKE_VALUE(GNPKG_LocalDataCache, GNERR_NoCache)


#ifdef __cplusplus
}
#endif

#endif /* _GN_ERROR_VALUES_H_ */

