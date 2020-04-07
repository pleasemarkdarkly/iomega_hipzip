// SimpleGUIUserInterface.cpp: demo user interface over serial port
// danc@iobjects.com 07/28/01
// (c) Interactive Objects

#include <main/ui/PEGUserInterface.h>
#include <playlist/common/Playlist.h>          // for IPlaylistEntry
#include <cyg/infra/diag.h>                    // for diag_printf
#include <codec/common/Codec.h>                // for set_stream_event_data_t

// peg headers
//#include <gui/peg/peg.hpp>
#include <main/ui/Messages.h>
#include <main/ui/Keys.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/LibraryEntryMenuScreen.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/SystemToolsMenuScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>

#include <main/iml/iml/IML.h>

//#include <stdio.h>      /* sprintf */

//static const char* szDefaultBanner = "iObjects Dadio SDK";

CPEGUserInterface::CPEGUserInterface() 
{
}

CPEGUserInterface::~CPEGUserInterface() 
{}

void CPEGUserInterface::SetBanner( const TCHAR* pBanner ) 
{
}

void CPEGUserInterface::ShowDirections() 
{
}

void CPEGUserInterface::NotifyPlaying() 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_PLAY);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyPaused() 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_PAUSE);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyStopped() 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_STOP);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyNextTrack() 
{
}

void CPEGUserInterface::NotifyPreviousTrack() 
{
}

void CPEGUserInterface::NotifySeekFwd() 
{
}

void CPEGUserInterface::NotifySeekRew() 
{
}

void CPEGUserInterface::NotifyCDTrayOpened() 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_CD_TRAY_OPENED);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyCDTrayClosed() 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_CD_TRAY_CLOSED);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyMediaInserted( int MediaIndex ) 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_MEDIA_INSERTED);
    msg.iData = MediaIndex;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyMediaRemoved( int MediaIndex ) 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_MEDIA_REMOVED);
    msg.iData = MediaIndex;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyNetworkUp()
{
    PegThing* pt = 0;
    PegMessage msg(CSourceMenuScreen::GetSourceMenuScreen(), IOPM_NETWORK_UP);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyNetworkDown()
{
    PegThing* pt = 0;
    PegMessage msg(CSourceMenuScreen::GetSourceMenuScreen(), IOPM_NETWORK_DOWN);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyIMLFound()
{
    PegThing* pt = 0;
    PegMessage msg(CSourceMenuScreen::GetSourceMenuScreen(), IOPM_IML_FOUND);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyIMLRemoved(CIML *pIML)
{
    PegThing* pt = 0;
    PegMessage msg(CSourceMenuScreen::GetSourceMenuScreen(), IOPM_IML_REMOVED);
    msg.pData = (void*)pIML;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyIMLAvailable(CIML *pIML)
{
	PegThing* pt = 0;
    PegMessage msg(CSourceMenuScreen::GetSourceMenuScreen(), IOPM_IML_AVAILABLE);
    msg.pData = (void*)pIML;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyContentUpdateBegin(int iDataSourceID)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_CONTENT_UPDATE_BEGIN);
    msg.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CONTENT_UPDATE_BEGIN);
    msg2.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_CONTENT_UPDATE_BEGIN);
    msg3.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyContentUpdate(int iDataSourceID, int iProgress, int iTotal)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_CONTENT_UPDATE);
    msg.iData = iDataSourceID;
    msg.iUserData[0] = iProgress;
    msg.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CONTENT_UPDATE);
    msg2.iData = iDataSourceID;
    msg2.iUserData[0] = iProgress;
    msg2.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_CONTENT_UPDATE);
    msg3.iData = iDataSourceID;
    msg3.iUserData[0] = iProgress;
    msg3.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyContentUpdateEnd(int iDataSourceID, int iTotal)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_CONTENT_UPDATE_END);
    msg.iData = iDataSourceID;
    msg.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CONTENT_UPDATE_END);
    msg2.iData = iDataSourceID;
    msg2.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_CONTENT_UPDATE_END);
    msg3.iData = iDataSourceID;
    msg3.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyMetadataUpdateBegin(int iDataSourceID)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_METADATA_UPDATE_BEGIN);
    msg.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_METADATA_UPDATE_BEGIN);
    msg2.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_METADATA_UPDATE_BEGIN);
    msg3.iData = iDataSourceID;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyMetadataUpdate(int iDataSourceID, int iProgress, int iTotal)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_METADATA_UPDATE);
    msg.iData = iDataSourceID;
    msg.iUserData[0] = iProgress;
    msg.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_METADATA_UPDATE);
    msg2.iData = iDataSourceID;
    msg2.iUserData[0] = iProgress;
    msg2.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_METADATA_UPDATE);
    msg3.iData = iDataSourceID;
    msg3.iUserData[0] = iProgress;
    msg3.iUserData[1] = iTotal;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyMetadataUpdateEnd(int iDataSourceID, int iTotal)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_METADATA_UPDATE_END);
    msg.iData = iDataSourceID;
    msg.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg);

    PegMessage msg2(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_METADATA_UPDATE_END);
    msg2.iData = iDataSourceID;
    msg2.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg2);

    PegMessage msg3(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_METADATA_UPDATE_END);
    msg3.iData = iDataSourceID;
    msg3.iUserData[0] = iTotal;
    pt->MessageQueue()->Push(msg3);
}

void CPEGUserInterface::NotifyCDDBUpdateDownloading(int iCurrentIndex, int iTotalFiles)
{
    PegThing* pt = 0;
    PegMessage msg(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CDDB_UPDATE_DOWNLOADING);
    msg.iData = iCurrentIndex;
    msg.lData = iTotalFiles;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyCDDBUpdateProcessing(int iCurrentIndex, int iTotalFiles)
{
    PegThing* pt = 0;
    PegMessage msg(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CDDB_UPDATE_PROCESSING);
    msg.iData = iCurrentIndex;
    msg.lData = iTotalFiles;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyCDDBUpdateError(int error)
{
    PegThing* pt = 0;
    PegMessage msg(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CDDB_UPDATE_ERROR);
    msg.iData = error;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyCDDBUpdateEnd()
{
    PegThing* pt = 0;
    PegMessage msg(CSystemToolsMenuScreen::GetSystemToolsMenuScreen(), IOPM_CDDB_UPDATE_END);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyADCClipDetected()
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_ADC_CLIP_DETECTED);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifySourceChanged(CDJPlayerState::ESourceMode eSource)
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_MUSIC_SOURCE_CHANGED);
    msg.iData = (int)eSource;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyPlaylistCleared(bool bDisplaySelectTracksMessage)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_CANCEL_QUERIES);
    pt->MessageQueue()->Push(msg);

    if ( bDisplaySelectTracksMessage )
    {
        PegMessage msg1(CPlayerScreen::GetPlayerScreen(), IOPM_PLAYLIST_CLEARED);
        pt->MessageQueue()->Push(msg1);
    }

    PegMessage msg2(CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen(), IOPM_PLAYLIST_CLEARED);
    pt->MessageQueue()->Push(msg2);
}

void CPEGUserInterface::NotifyPlaylistLoadBegin(int iTotalTracks)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_PLAYLIST_LOAD_BEGIN);
    msg.lData = iTotalTracks;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyPlaylistLoadProgess(playlist_load_info_t* pLoadInfo)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_PLAYLIST_LOAD_PROGRESS);
    msg.pData = pLoadInfo;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyPlaylistLoadEnd()
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_PLAYLIST_LOAD_END);
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::NotifyDeleteContent(content_delete_info_t* pDeleteInfo)
{
    PegThing* pt = 0;
    PegMessage msg(CLibraryMenuScreen::GetLibraryMenuScreen(), IOPM_DELETE_CONTENT);
    msg.pData = pDeleteInfo;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::SetTrack( set_stream_event_data_t* pSSED, IMediaContentRecord* pCR, int iTrackCount )
{
    set_track_message_t* stm = new set_track_message_t;
    stm->duration = pSSED->streamInfo.Duration;
    stm->bitrate = pSSED->streamInfo.Bitrate;
    stm->metadata = pCR;    // TODO: copy

    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_NEW_TRACK);
    msg.pData = stm;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::SetTrackTime( int Seconds ) 
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_TRACK_PROGRESS);
    msg.lData = Seconds;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::SetPlaylistMode( IPlaylist::PlaylistMode mode )
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_PLAYMODE);

    switch (mode)
    {
        case IPlaylist::NORMAL:
        case IPlaylist::REPEAT_ALL:
        case IPlaylist::RANDOM:
        case IPlaylist::REPEAT_RANDOM:
            msg.iData = mode;
            break;

        default:
            msg.iData = IPlaylist::NORMAL;
            break;
    }
    pt->MessageQueue()->Push(msg);

}

#ifndef DISABLE_VOLUME_CONTROL
void CPEGUserInterface::SetVolume( int Volume )
{
    //char szScratch[12];
    //sprintf(szScratch, "Volume: %2d", Volume);
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_VOLUME);
    msg.lData = Volume;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::SetBass( int iBass ){}

void CPEGUserInterface::SetTreble( int iTreble ){}
#endif // DISABLE_VOLUME_CONTROL

void CPEGUserInterface::SetMessage( const TCHAR* szMessage, int iMessageType) 
{
    // allocate space for this message string.
    // make sure it's deleted on the receiving end (PlayerScreen)!!!
    TCHAR* szTchar = (TCHAR*)malloc((tstrlen(szMessage) + 1) * sizeof(TCHAR));
    tstrcpy(szTchar, szMessage);

    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_SYSTEM_MESSAGE);
    msg.iData = iMessageType;
    msg.pData = (void*)szTchar;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::ClearTrack()
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_CLEAR_TRACK);
    pt->MessageQueue()->Push(msg);
}


void CPEGUserInterface::RefreshTrack()
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_REFRESH_METADATA);
    pt->MessageQueue()->Push(msg);
}


void CPEGUserInterface::CDMultipleMetadata(void* pData)
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_MULTIPLE_METADATA);
    msg.pData = pData;
    pt->MessageQueue()->Push(msg);
}


void CPEGUserInterface::SetUIViewMode(CDJPlayerState::EUIViewMode eUIViewMode, bool bKeyPress)
{
    PegThing*  pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_SET_UI_VIEW_MODE);
    msg.iData = eUIViewMode;
    msg.lData = (LONG)bKeyPress;
    pt->MessageQueue()->Push(msg);
}

