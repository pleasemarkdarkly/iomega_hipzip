//
// SerialUserInterface.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef SERIALUSERINTERFACE_H_
#define SERIALUSERINTERFACE_H_

#include <main/ui/common/UserInterface.h>

//! The CSerialUserInterface class implents the IUserInterface class
//! to provide text-based updates of the player state over the serial line
//! for the demo players. It is not part of the SDK proper, but is provided
//! to support the demo applications.

class CSerialUserInterface : public IUserInterface
{
public:
    CSerialUserInterface();
    ~CSerialUserInterface();

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
    void ClearTrack() { }
    void SetTrackTime(int iSeconds);

    void SetPlaylistMode(IPlaylist::PlaylistMode mode);

    void SetVolume(int iVolume);
    void SetBass(int iBass);
    void SetTreble(int iTreble);

    // Set a one-line text message to be displayed to the user
    void SetMessage(const char* szFormat, ...);
    
private:
    const char* m_szBanner;
};

//@}

#endif  // SERIALUSERINTERFACE_H_
