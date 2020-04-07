//
// UserInterface.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#include <main/main/DJPlayerState.h>
#include <main/ui/ContentDeleteEvents.h>    // For content_delete_info_t
#include <main/ui/PlaylistLoadEvents.h>  // For playlist_load_info_t
#include <main/ui/SystemMessageString.h>
#include <playlist/common/Playlist.h>   // For PlaylistMode
#include <main/iml/iml/IML.h>
#include <stdarg.h>                     // ...

// fdecl
class IPlaylistEntry;
typedef struct set_stream_event_data_s set_stream_event_data_t;

//! This is an abstract base class used for simple demo UI purposes.
//! Functions are provided for notifying the user of player events.
//! It's not part of the SDK proper, but exists to give an example of
//! what a basic user interface might look like.
class IUserInterface
{
public:
    virtual ~IUserInterface()  {}

    // Basic UI configuration
    virtual void SetBanner(const TCHAR* szBanner) = 0;

    virtual void NotifyPlaying() = 0;
    virtual void NotifyPaused()  = 0;
    virtual void NotifyStopped() = 0;
    
    virtual void NotifyNextTrack() = 0;
    virtual void NotifyPreviousTrack() = 0;

    virtual void NotifySeekFwd() = 0;
    virtual void NotifySeekRew() = 0;

    virtual void NotifyCDTrayOpened() = 0;
    virtual void NotifyCDTrayClosed() = 0;

    virtual void NotifyMediaInserted(int iMediaIndex) = 0;
    virtual void NotifyMediaRemoved(int iMediaIndex) = 0;

    virtual void NotifyNetworkUp()              = 0;
    virtual void NotifyNetworkDown()            = 0;
    virtual void NotifyIMLFound()               = 0;
    virtual void NotifyIMLRemoved(CIML *pIML)   = 0;
	virtual void NotifyIMLAvailable(CIML *pIML) = 0;

    virtual void NotifyContentUpdateBegin(int iDataSourceID) = 0;
    virtual void NotifyContentUpdate(int iDataSourceID, int iProgress, int iTotal) = 0;
    virtual void NotifyContentUpdateEnd(int iDataSourceID, int iTotal) = 0;

    virtual void NotifyMetadataUpdateBegin(int iDataSourceID) = 0;
    virtual void NotifyMetadataUpdate(int iDataSourceID, int iProgress, int iTotal) = 0;
    virtual void NotifyMetadataUpdateEnd(int iDataSourceID, int iTotal) = 0;

    virtual void NotifyCDDBUpdateDownloading(int iCurrentIndex, int iTotalFiles) = 0;
    virtual void NotifyCDDBUpdateProcessing(int iCurrentIndex, int iTotalFiles) = 0;
    virtual void NotifyCDDBUpdateError(int error) = 0;
    virtual void NotifyCDDBUpdateEnd() = 0;

    virtual void NotifyADCClipDetected() = 0;

    virtual void NotifySourceChanged(CDJPlayerState::ESourceMode eSource) = 0;

    virtual void NotifyPlaylistCleared(bool bDisplaySelectTracksMessage = false) = 0;

    virtual void NotifyPlaylistLoadBegin(int iTotalTracks) = 0;
    virtual void NotifyPlaylistLoadProgess(playlist_load_info_t* pLoadInfo) = 0;
    virtual void NotifyPlaylistLoadEnd() = 0;

    virtual void NotifyDeleteContent(content_delete_info_t* pDeleteInfo) = 0;

    // Set track data
    virtual void SetTrack(set_stream_event_data_t*, IMediaContentRecord* pCR, int iTrackCount = 0) = 0;
    virtual void ClearTrack() = 0;
    virtual void RefreshTrack() = 0;
    virtual void SetTrackTime(int iSeconds) = 0;

    virtual void SetPlaylistMode(IPlaylist::PlaylistMode mode) = 0;

#ifndef DISABLE_VOLUME_CONTROL
    virtual void SetVolume(int iVolume) = 0;
    virtual void SetBass(int iBass) = 0;
    virtual void SetTreble(int iTreble) = 0;
#endif // DISABLE_VOLUME_CONTROL

    // Set a one-line text message to be displayed to the user
    virtual void SetMessage(const TCHAR* szMessage, int iMessageType = CSystemMessageString::INFO) = 0;

    // Tell the ui that we need to choose between a list of possible cd matches
    virtual void CDMultipleMetadata(void* pData) = 0;

    virtual void SetUIViewMode(CDJPlayerState::EUIViewMode eUIViewMode, bool bKeyPress = false) = 0;

};


//@}

#endif  // USERINTERFACE_H_

