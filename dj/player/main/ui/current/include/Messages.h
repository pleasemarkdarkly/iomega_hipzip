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

class IMetadata;

//! Used to propagate information about the track out of the
//! ui to the screens in a set track event.
typedef struct set_track_message_s
{
    IMetadata* metadata;
    unsigned long duration;
    unsigned long bitrate;
} set_track_message_t;

// Sent by the media player to confirm playing, pausing, or stopping.
#define	IOPM_PLAY                                 FIRST_USER_MESSAGE + 1
#define	IOPM_PAUSE                                FIRST_USER_MESSAGE + 2
#define	IOPM_STOP                                 FIRST_USER_MESSAGE + 3
#define	IOPM_TRACK_PROGRESS                       FIRST_USER_MESSAGE + 10
#define IOPM_NEW_TRACK                            FIRST_USER_MESSAGE + 11
#define IOPM_TRACK_END                            FIRST_USER_MESSAGE + 12
#define IOPM_DISK_INFO                            FIRST_USER_MESSAGE + 13
#define IOPM_NO_DISK                              FIRST_USER_MESSAGE + 14
#define IOPM_PLAYLIST_LOADED                      FIRST_USER_MESSAGE + 15
#define IOPM_VOLUME                               FIRST_USER_MESSAGE + 16
#define IOPM_PLAYMODE                             FIRST_USER_MESSAGE + 17
#define IOPM_CLEAR_TRACK                          FIRST_USER_MESSAGE + 18
#define IOPM_SYSTEM_MESSAGE                       FIRST_USER_MESSAGE + 19
#define IOPM_REFRESH_METADATA                     FIRST_USER_MESSAGE + 20
#define IOPM_MULTIPLE_METADATA                    FIRST_USER_MESSAGE + 21
#define IOPM_CD_TRAY_OPENED                       FIRST_USER_MESSAGE + 22
#define IOPM_CD_TRAY_CLOSED                       FIRST_USER_MESSAGE + 23
#define IOPM_MEDIA_INSERTED                       FIRST_USER_MESSAGE + 24
#define IOPM_MEDIA_REMOVED                        FIRST_USER_MESSAGE + 25
#define IOPM_IML_REMOVED                          FIRST_USER_MESSAGE + 26
#define IOPM_IML_AVAILABLE                        FIRST_USER_MESSAGE + 27
#define IOPM_ADC_CLIP_DETECTED                    FIRST_USER_MESSAGE + 28
#define IOPM_CANCEL_QUERIES                       FIRST_USER_MESSAGE + 29
#define IOPM_SET_UI_VIEW_MODE                     FIRST_USER_MESSAGE + 30
#define IOPM_MUSIC_SOURCE_CHANGED                 FIRST_USER_MESSAGE + 31
#define IOPM_CDDB_UPDATE_END                      FIRST_USER_MESSAGE + 32
#define IOPM_CDDB_UPDATE_ERROR                    FIRST_USER_MESSAGE + 33
#define IOPM_CDDB_UPDATE_DOWNLOADING              FIRST_USER_MESSAGE + 34
#define IOPM_CDDB_UPDATE_PROCESSING               FIRST_USER_MESSAGE + 35
#define IOPM_IML_FOUND                            FIRST_USER_MESSAGE + 36
#define IOPM_NETWORK_UP                           FIRST_USER_MESSAGE + 37
#define IOPM_NETWORK_DOWN                         FIRST_USER_MESSAGE + 38
#define IOPM_CONTENT_UPDATE_BEGIN                 FIRST_USER_MESSAGE + 39
#define IOPM_CONTENT_UPDATE                       FIRST_USER_MESSAGE + 40
#define IOPM_CONTENT_UPDATE_END                   FIRST_USER_MESSAGE + 41
#define IOPM_METADATA_UPDATE_BEGIN                FIRST_USER_MESSAGE + 42
#define IOPM_METADATA_UPDATE                      FIRST_USER_MESSAGE + 43
#define IOPM_METADATA_UPDATE_END                  FIRST_USER_MESSAGE + 44
#define IOPM_PLAYLIST_LOAD_BEGIN                  FIRST_USER_MESSAGE + 45
#define IOPM_PLAYLIST_LOAD_PROGRESS               FIRST_USER_MESSAGE + 46
#define IOPM_PLAYLIST_LOAD_END                    FIRST_USER_MESSAGE + 47
#define IOPM_PLAYLIST_CLEARED                     FIRST_USER_MESSAGE + 48
#define IOPM_DELETE_CONTENT                       FIRST_USER_MESSAGE + 49


#endif	// MESSAGES_H_
