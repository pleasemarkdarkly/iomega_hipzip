//
// PlaylistLoadEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of events sent by the playlist loading code to the UI for the dj project.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef PLAYLISTLOADEVENTS_H_
#define PLAYLISTLOADEVENTS_H_

#include <content/common/ContentManager.h>

//! Sent when playlist loading starts.
//! The data parameter is the total number of items to be added to the playlist.
#define EVENT_PLAYLIST_LOAD_BEGIN   0x600

typedef struct playlist_load_info_s
{
    IMediaRecordInfoVector  records;    //!< List of media records to be added to the playlist.
    int                     index;      //!< Current index in that list that is being processed.
} playlist_load_info_t;

//! Sent during a playlist load update.
//! The data parameter is a pointer to a playlist_load_info_t struct.
//! It should be deleted when done.
#define EVENT_PLAYLIST_LOAD         0x601

//! Sent when playlist loading ends.
#define EVENT_PLAYLIST_LOAD_END     0x602

//@}

#endif  // PLAYLISTLOADEVENTS_H_


