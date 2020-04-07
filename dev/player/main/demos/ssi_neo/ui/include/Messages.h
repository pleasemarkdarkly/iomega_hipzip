//........................................................................................
//........................................................................................
//.. File Name: IomegaMessages.h														..
//.. Date: 7/21/2000																	..
//.. Author(s): Ed Miller																..
//.. Description of content: definitions of IOPM values					 				..
//.. Usage: Sent by the media player to confirm playing, pausing, or stopping.			..
//.. Last Modified By: Todd Malsbary	toddm@iobjects.com								..	
//.. Modification date: 9/5/2000														..
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#ifndef MESSAGES_H_
#define MESSAGES_H_

#include <gui/peg/peg.hpp>

//! Used to propagate information about the track out of the
//! ui to the screens in a set track event.
typedef struct set_track_message_s
{
    IMetadata* metadata;
    unsigned long duration;
    unsigned long bitrate;
    bool metadata_copy;     // target owns metadata, must delete
} set_track_message_t;

// Sent by the media player to confirm playing, pausing, or stopping.
#define	IOPM_PLAY				FIRST_USER_MESSAGE + 1
#define	IOPM_PAUSE				FIRST_USER_MESSAGE + 2
#define	IOPM_STOP				FIRST_USER_MESSAGE + 3
#define	IOPM_TRACK_PROGRESS		FIRST_USER_MESSAGE + 10
#define IOPM_NEW_TRACK			FIRST_USER_MESSAGE + 11
#define IOPM_TRACK_END			FIRST_USER_MESSAGE + 12
#define IOPM_DISK_INFO			FIRST_USER_MESSAGE + 13
#define IOPM_NO_DISK			FIRST_USER_MESSAGE + 14
#define IOPM_PLAYLIST_LOADED	FIRST_USER_MESSAGE + 15
#define IOPM_VOLUME         	FIRST_USER_MESSAGE + 16
#define IOPM_PLAYMODE         	FIRST_USER_MESSAGE + 17


#endif	// MESSAGES_H_
