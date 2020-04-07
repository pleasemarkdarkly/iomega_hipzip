//
// UserInterface.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef USERINTERFACE_H_
#define USERINTERFACE_H_

#include <playlist/common/Playlist.h>   // For PlaylistMode
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
    virtual void SetBanner(const char* szBanner) = 0;

    virtual void NotifyPlaying() = 0;
    virtual void NotifyPaused()  = 0;
    virtual void NotifyStopped() = 0;
    
    virtual void NotifyNextTrack() = 0;
    virtual void NotifyPreviousTrack() = 0;

    virtual void NotifySeekFwd() = 0;
    virtual void NotifySeekRew() = 0;

    virtual void NotifyMediaInserted(int iMediaIndex) = 0;
    virtual void NotifyMediaRemoved(int iMediaIndex) = 0;

    virtual void NotifyStreamEnd() = 0;
    // Set track data
    virtual void SetTrack(set_stream_event_data_t*, int iTrackCount = 0) = 0;
    virtual void ClearTrack() = 0;
    virtual void SetTrackTime(int iSeconds) = 0;

    virtual void SetPlaylistMode(IPlaylist::PlaylistMode mode) = 0;

    virtual void SetVolume(int iVolume) = 0;
    virtual void SetBass(int iBass) = 0;
    virtual void SetTreble(int iTreble) = 0;

    // Set a one-line text message to be displayed to the user
    virtual void SetMessage(const char* szMessage, ...) = 0;

};


//@}

#endif  // USERINTERFACE_H_

