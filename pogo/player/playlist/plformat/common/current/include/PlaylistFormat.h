//
// PlaylistFormat.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Dadio(tm) uses a registration interface to maintain a list of playlist formats
//! it can read and write.  New playlist formats can be added by using the macro
//! REGISTER_PLAYLIST_FORMAT and providing functions for loading and saving playlist
//! to and from streams.
/** \addtogroup PlaylistFormat Playlist Format */
//@{

#ifndef PLAYLISTFORMAT_H_
#define PLAYLISTFORMAT_H_

#include <content/common/ContentManager.h>
#include <util/ident/IdentifiableObject.h>
#include <util/registry/Registry.h>
#include <util/eresult/eresult.h>

//
//! Define the playlist format error zone for ERESULT error codes
//
#define PLAYLIST_FORMAT_TYPE_ID   0x06

#define MAKE_PLAYLIST_FORMAT_RESULT( x, y ) MAKE_ERESULT( x, PLAYLIST_FORMAT_TYPE_ID, y )

//! No error was encountered during the operation. \hideinitializer
const ERESULT PLAYLIST_FORMAT_NO_ERROR          = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_SUCCESS, 0x0000 );
//! An error occurred opening the file. \hideinitializer
const ERESULT PLAYLIST_FORMAT_FILE_OPEN_ERROR   = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0001 );
//! No matching playlist format was found. \hideinitializer
const ERESULT PLAYLIST_FORMAT_UNKNOWN_FORMAT    = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0002 );
//! An error occurred when loading the playlist file. \hideinitializer
const ERESULT PLAYLIST_FORMAT_BAD_FORMAT        = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0003 );
//! An error occurred reading the file. \hideinitializer
const ERESULT PLAYLIST_FORMAT_READ_ERROR        = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0004 );
//! An error occurred writing the file. \hideinitializer
const ERESULT PLAYLIST_FORMAT_WRITE_ERROR       = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0005 );
//! No corresponding data source was found for the URL. \hideinitializer
const ERESULT PLAYLIST_FORMAT_BAD_URL           = MAKE_PLAYLIST_FORMAT_RESULT( SEVERITY_FAILED,  0x0006 );

//
// fdecl
//
class IPlaylist;

//! The function prototype for loading playlists.
//! \param records A list of media_content_t vectors which will be filled by the function.
//! \param szURL The URL of the file to open for reading.
//! \param iMaxTracks The maximum number of tracks to load from the playlist.
//! If 0 then all tracks from the playlist should be loaded.
typedef ERESULT FNLoadPlaylist( IMediaRecordInfoVector& records, const char* szURL, int iMaxTracks );

//! The function prototype for saving playlists.
//! \param szURL The URL of the file to open for writing.
//! \param pPlaylist The playlist whose entries are to be written to file.
typedef ERESULT FNSavePlaylist( const char* szURL, IPlaylist* pPlaylist );

// Short table of static functions that a playlist format must expose.
// This is auto-generated by the registration interface
//!\if doxygen_should_ignore_this
typedef struct
{
    const char* (*GetPlaylistFormatName)();
    const char*const* (*GetExtensionList)();
    FNLoadPlaylist* pfnLoadPlaylist;
    FNSavePlaylist* pfnSavePlaylist;
} playlist_format_function_map_t;
//!\endif

//
// Registration interface
//

// This should be used in your implementation
//! Fill out the playlist format class definition with additional members
//! necessary for playlist formats to work in Dadio(tm). Additionally, this
//! satisfies the underlying requirements of the ident (IIdentifiableObject) baseclass.
//! 
//! The usage would be like:
//!\code
//!  REGISTER_PLAYLIST_FORMAT( "Winamp M3U playlist format", 97, LoadM3UPlaylist, SaveM3UPlaylist, "m3u" );
//!\endcode
//!\code
//!  REGISTER_PLAYLIST_FORMAT( "Dadio playlist format", 98, LoadDPLPlaylist, SaveDPLPlaylist, "dpl" );
//!\endcode
//!
//!\arg formatnamestring    A string name for the playlist format
//!\arg playlistformatID    A unique non-zero identifier for the playlist format
//!\arg loadplaylist        The function used to load this playlist format
//!\arg saveplaylist        The function used to save this playlist format
//!\arg extensions...       A variable length list of strings that
//!                         indicate which extensions are supported by
//!                         this playlist format. This is case insensitive.
//!\hideinitializer
#define REGISTER_PLAYLIST_FORMAT( formatnamestring, playlistformatID, loadplaylist, saveplaylist, extensions... )  \
        const char* plformat_##playlistformatID##_GetPlaylistFormatName() { return formatnamestring; } \
        static const char*const plformat_##playlistformatID##_spExtensions[] = { extensions, 0 }; \
        const char*const* plformat_##playlistformatID##_GetExtensionList() { return plformat_##playlistformatID##_spExtensions; } \
        static const playlist_format_function_map_t plformat_##playlistformatID##_class_fmap = {          \
                plformat_##playlistformatID##_GetPlaylistFormatName,     \
                plformat_##playlistformatID##_GetExtensionList,          \
                loadplaylist,                          \
                saveplaylist,                          \
        };                                              \
        static const CRegisterObject class_reg(               \
                ASSEMBLE_ID( PLAYLIST_FORMAT_TYPE_ID, playlistformatID ),  \
                (void*) &plformat_##playlistformatID##_class_fmap );

//@}

#endif  // PLAYLISTFORMAT_H_