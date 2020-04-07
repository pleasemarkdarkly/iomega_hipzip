// PEGRestoreUserInterface.h:  interface for the PEG UI in Restore Mode
// dand@iobjects.com 05/01/02
// (c) Interactive Objects

#ifndef __PEG_RESTORE_USERINTERFACE_H__
#define __PEG_RESTORE_USERINTERFACE_H__

#include <main/ui/common/UserInterface.h>

class CPEGRestoreUserInterface : public IUserInterface
{
public:
    CPEGRestoreUserInterface() {}
    ~CPEGRestoreUserInterface() {}

    void SetBanner( const TCHAR* pBanner ) {}
    void ShowDirections() {}
    
    void NotifyPlaying() {}
    void NotifyPaused() {}
    void NotifyStopped() {}
    
    void NotifyNextTrack() {}
    void NotifyPreviousTrack() {}

    void NotifySeekFwd() {}
    void NotifySeekRew() {}

    void NotifyCDTrayOpened();
    void NotifyCDTrayClosed();

    void NotifyMediaInserted( int MediaIndex );
    void NotifyMediaRemoved( int MediaIndex );

    void NotifyNetworkUp() {}
    void NotifyNetworkDown() {}
    void NotifyIMLFound() {}
	void NotifyIMLRemoved(CIML *pIML) {}
	void NotifyIMLAvailable(CIML *pIML) {}

    void NotifyContentUpdateBegin(int iDataSourceID) {}
    void NotifyContentUpdate(int iDataSourceID, int iProgress, int iTotal) {}
    void NotifyContentUpdateEnd(int iDataSourceID, int iTotal) {}

    void NotifyMetadataUpdateBegin(int iDataSourceID) {}
    void NotifyMetadataUpdate(int iDataSourceID, int iProgress, int iTotal) {}
    void NotifyMetadataUpdateEnd(int iDataSourceID, int iTotal) {}

    void NotifyCDDBUpdateDownloading(int iCurrentIndex, int iTotalFiles) {}
    void NotifyCDDBUpdateProcessing(int iCurrentIndex, int iTotalFiles) {}
    void NotifyCDDBUpdateError(int error) {}
    void NotifyCDDBUpdateEnd() {}
    
    void NotifyADCClipDetected() {}

    void NotifySourceChanged(CDJPlayerState::ESourceMode eSource) {}
    
    void NotifyPlaylistCleared(bool bDisplaySelectTracksMessage = false) {}

    void NotifyPlaylistLoadBegin(int iTotalTracks) {}
    void NotifyPlaylistLoadProgess(playlist_load_info_t* pLoadInfo) {}
    void NotifyPlaylistLoadEnd() {}

    void NotifyDeleteContent(content_delete_info_t* pDeleteInfo) {}

    // Set track data
    void SetTrack( set_stream_event_data_t* pSSED, IMediaContentRecord* pCR, int iTrackCount = 0 ) {}
    void SetTrackTime( int Seconds ) {}

    void SetPlaylistMode( IPlaylist::PlaylistMode mode ) {}

#ifndef DISABLE_VOLUME_CONTROL
    void SetVolume( int iVolume ) {}
    void SetBass( int iBass ) {}
    void SetTreble( int iTreble ) {}
#endif // DISABLE_VOLUME_CONTROL

    // Set a one-line text message to be displayed to the user
    void SetMessage( const TCHAR* szMessage, int iMessageType = CSystemMessageString::STATUS) {}
    void ClearTrack() {}
    void RefreshTrack() {}

    // Tell the ui that we need to choose between a list of possible cd matches
    void CDMultipleMetadata(void* pData) {}

    void SetUIViewMode(CDJPlayerState::EUIViewMode eUIViewMode, bool bKeyPress = false) {}

private:
};


#endif // __PEG_RESTORE_USERINTERFACE_H__
