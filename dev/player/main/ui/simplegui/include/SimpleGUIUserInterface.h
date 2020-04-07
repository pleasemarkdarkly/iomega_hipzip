//
// SimpleGUIUserInterface.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef SIMPLEGUIUSERINTERFACE_H_
#define SIMPLEGUIUSERINTERFACE_H_

#include <main/ui/common/UserInterface.h>
#include <gui/simplegui/show/Show.h>
#include <gui/simplegui/screen/Screen.h>
#include <gui/simplegui/screenelem/textlabel/TextLabel.h>
#include <gui/simplegui/screenelem/bitmap/Bitmap.h>

//! The CSimpleGUIUserInterface class implents the IUserInterface class
//! to provide a simple graphical user interface for the demo players.
//! It is not part of the SDK proper, but is provided to support the demo
//! applications

class CSimpleGUIUserInterface : public IUserInterface
{
  public:
    CSimpleGUIUserInterface();
    ~CSimpleGUIUserInterface();

    void SetBanner(const char* szBanner);
    void ShowDirections();
    
    void NotifyPlaying();
    void NotifyPaused();
    void NotifyStopped();
    
    void NotifyNextTrack();
    void NotifyPreviousTrack();

    void NotifySeekFwd();
    void NotifySeekRew();

    void NotifyMediaInserted(int iMediaIndex);
    void NotifyMediaRemoved(int iMediaIndex);

    void NotifyStreamEnd();
    // Set track data
    void SetTrack(set_stream_event_data_t*, int iTrackCount = 0);
    void ClearTrack();
    void SetTrackTime(int iSeconds);

    void SetPlaylistMode(IPlaylist::PlaylistMode mode);

    void SetVolume(int iVolume);
    void SetBass(int iBass);
    void SetTreble(int iTreble);

    // Set a one-line text message to be displayed to the user
    void SetMessage(const char* szFormat, ...);
    
  private:
    CShow m_show;
    CScreen m_screen;
    CTextLabel m_lblBanner;
    CTextLabel m_lblTitleLabel;
    CTextLabel m_lblTitleValue;
    CTextLabel m_lblArtistLabel;
    CTextLabel m_lblArtistValue;
    CTextLabel m_lblAlbumLabel;
    CTextLabel m_lblAlbumValue;
    CTextLabel m_lblGenreLabel;
    CTextLabel m_lblGenreValue;
    CTextLabel m_lblTrackNumber;
    CTextLabel m_lblPlaylistModeLabel;
    CTextLabel m_lblPlaylistModeValue;
    CTextLabel m_lblTrackTime;
    CTextLabel m_lblVolumeLabel;
    CTextLabel m_lblVolumeValue;
    CTextLabel m_lblBassLabel;
    CTextLabel m_lblBassValue;
    CTextLabel m_lblTrebleLabel;
    CTextLabel m_lblTrebleValue;
    CTextLabel m_lblMessage;
    CBitmap m_bmpPlay;
    CBitmap m_bmpPause;
    CBitmap m_bmpStop;

    int m_iCurrentTrackTime;
};

//@}


#endif  // SIMPLEGUIUSERINTERFACE_H_
