// SimpleGUIUserInterface.cpp: demo user interface over serial port
// danc@fullplaymedia.com 07/28/01
// (c) Fullplay Media Systems

#include <main/ui/PEGUserInterface.h>
#include <playlist/common/Playlist.h>          // for IPlaylistEntry
#include <cyg/infra/diag.h>                    // for diag_printf
#include <codec/common/Codec.h>                // for set_stream_event_data_t

// peg headers
#include <gui/peg/peg.hpp>
#include <main/ui/UI.h>
#include <main/ui/Messages.h>
#include <main/ui/Keys.h>

#include <stdio.h>      /* sprintf */

static const char* szDefaultBanner = "iObjects Dadio SDK";

CPEGUserInterface::CPEGUserInterface() 
{
}

CPEGUserInterface::~CPEGUserInterface() 
{}

void CPEGUserInterface::SetBanner( const char* pBanner ) 
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

void CPEGUserInterface::NotifyMediaInserted( int MediaIndex ) 
{
}

void CPEGUserInterface::NotifyMediaRemoved( int MediaIndex ) 
{
}

void CPEGUserInterface::NotifyStreamEnd( ) 
{
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->UpdateTrackDuration();
}

void CPEGUserInterface::SetTrack( set_stream_event_data_t* pSSED, int iTrackCount )
{
    IMediaContentRecord* pContentRecord = pSSED->pPlaylistEntry->GetContentRecord();
    set_track_message_t* stm = new set_track_message_t;
    stm->fileref = pSSED->pPlaylistEntry->GetContentRecord()->GetFileNameRef();
    stm->duration = pSSED->streamInfo.Duration;
    stm->bitrate = pSSED->streamInfo.Bitrate;
    stm->metadata = pContentRecord;

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
            msg.lData = 0;
            break;

        case IPlaylist::REPEAT_ALL:
            msg.lData = 2;
            break;

        case IPlaylist::RANDOM:
            msg.lData = 1;
            break;

        case IPlaylist::REPEAT_RANDOM:
            msg.lData = 3;
            break;

        case IPlaylist::REPEAT_TRACK:
            msg.lData = 4;
            break;
    }
    pt->MessageQueue()->Push(msg);

}

void CPEGUserInterface::SetVolume( int Volume )
{
    char szScratch[12];
    sprintf(szScratch, "Volume: %2d", Volume);
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_VOLUME);
    msg.lData = Volume;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::SetBass( int iBass ){}

void CPEGUserInterface::SetTreble( int iTreble ){}

void CPEGUserInterface::SetMessage( const char* szFormat, ... ) 
{
    static char szBuf[256];
    va_list va;
    va_start(va, szFormat);
    vsnprintf(szBuf, 256, szFormat, va);
    va_end(va);

    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_PLAYLIST_LOADED);
    msg.lData = (long)szBuf;
    pt->MessageQueue()->Push(msg);
}

void CPEGUserInterface::ClearTrack()
{
    PegThing* pt = 0;
    PegMessage msg(CPlayerScreen::GetPlayerScreen(), IOPM_CLEAR_TRACK);
    pt->MessageQueue()->Push(msg);
}
