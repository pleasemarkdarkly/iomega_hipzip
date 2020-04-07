//
// SystemEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of system events used by the SDK.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef SYSTEMEVENTS_H_
#define SYSTEMEVENTS_H_

//! Sent from the keyboard driver when a key that's in the press map is pressed.
//! The data parameter is the value of the key in the press map.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_KEY_PRESS        0x01
//! Sent from the keyboard driver when a key that's in the hold map is held down.
//! The data parameter is the value of the key in the hold map.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_KEY_HOLD         0x02
//! Sent from the keyboard driver when a key that's in the release map is released.
//! The data parameter is the value of the key in the release map.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_KEY_RELEASE      0x03

//! Sent from the media player when an entry is successfully set.
//! The data parameter is a pointer to a set_stream_event_data_t struct.
//! The default event handler simply deletes the pMediaPlayerMetadata pointer,
//! then deletes the set_stream_event_data_t itself.  Thus, it's easiest to copy
//! whatever metadata information needed for display before passing the event
//! to the default handler for cleanup. \hideinitializer
#define EVENT_STREAM_SET       0x10
//! Sent from the media player when the end of a song is reached during playback.
//! The default event handler sets the next track and continues playback. \hideinitializer
#define EVENT_STREAM_END       0x11
//! Sent from the media player if playback is interrupted because of an error.
//! The default event handler sets the next track and continues playback. \hideinitializer
#define EVENT_STREAM_FAIL      0x12
//! Sent from the media player during playback to notify the interface of the current track time.
//! The data parameter is an integer that specifies the track time in seconds. \hideinitializer
//! The default event handler ignores this event.
#define EVENT_STREAM_PROGRESS  0x13
//! Sent from the media player when playback begins.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_STREAM_PLAYING   0x14
//! Sent from the media player when playback is paused.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_STREAM_PAUSED    0x15
//! Sent from the media player when playback is stopped.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_STREAM_STOPPED   0x16
//! Sent from the media player when the current track finishes and the player has
//! automatically transitioned to the next song. Data is the same as for EVENT_STREAM_SET
#define EVENT_STREAM_AUTOSET   0x17

//! The data scan ID is used by the event system during a content scan.
//! It's a combination of the instance ID of the data source that's being scanned and
//! the scan ID assigned by the data source manager.
#define MAKE_DATA_SCAN_ID( iDataSourceID, usScanID ) \
    (unsigned int)(((unsigned int)(usScanID) << 16) | (unsigned int)(iDataSourceID))
//! Given a data scan ID this macro will extract the data source's instance ID.
#define GET_DATA_SOURCE_ID( uiDataScanID )  ((int)(uiDataScanID) & 0xFFFF)
//! Given a data scan ID this macro will extract the scan ID.
#define GET_SCAN_ID( uiDataScanID )         ((int)(uiDataScanID) >> 16)

//! Sent from the data source when starting a content update.
//! The data parameter is the data scan ID.
//! The default event handler calls MarkRecordsFromDataSourceUnverified in the content manager. \hideinitializer
#define EVENT_CONTENT_UPDATE_BEGIN          0x20
//! Sent from the data source during a content update when the number of records in the chunk size has been reached.
//! The data parameter is a pointer to a content_record_update_t struct populated with content.
//! The default event handler calls NotifyContentUpdate, which calls AddContentRecords in the
//! content manager to add the new records, then either deletes the content_record_update_t struct or
//! passes it back to the data source manager through the GetContentMetadata in a two-pass system. \hideinitializer
#define EVENT_CONTENT_UPDATE                0x21
//! Sent from the data source when a content update is completed.
//! The data parameter is the data scan ID.
//! The default event handler calls DeleteUnverifiedRecordsFromDataSource in the content manager. \hideinitializer
#define EVENT_CONTENT_UPDATE_END            0x22
//! Sent from the data source when an error occurs during a content update.
//! The data parameter is the data scan ID.
//! The default event handler calls DeleteUnverifiedRecordsFromDataSource in the content manager. \hideinitializer
#define EVENT_CONTENT_UPDATE_ERROR          0x23
//! Sent from the data source after the first pass of a content update (only if there's going to be a second pass).
//! The data parameter is the data scan ID.
//! The default event handler tells the data source manager to send the EVENT_CONTENT_METADATA_UPDATE_END
//! message when the second pass is completed. \hideinitializer
#define EVENT_CONTENT_METADATA_UPDATE_BEGIN 0x24
//! Sent from the data source during the second pass of a content update.
//! The data parameter is a pointer to a content_record_update_t struct populated with content.
//! The default event handler calls NotifyContentMetadataUpdate, which calls AddContentRecords in the
//! content manager to add the new records.  When finished it deletes the content_record_update_t struct. \hideinitializer
#define EVENT_CONTENT_METADATA_UPDATE       0x25
//! Sent from the data source manager after the second pass of a content update is finished.
//! The data parameter is the data scan ID.
//! The default event handler ignores this event. \hideinitializer
#define EVENT_CONTENT_METADATA_UPDATE_END   0x26

//! Sent from the data source when it detects its media (e.g., CD-ROM, CF card) has been removed.
//! The data parameter is an integer that specifies the data source's instance ID.
//! The default event handler calls NotifyContentMetadataUpdate in the content manager,
//! which stops the track playing on the media player if its data source is the removed media,
//! and then removes all of the content records for that data source from the play list. \hideinitializer
#define EVENT_MEDIA_REMOVED    0x30
//! Sent from the data source when it detects its media (e.g., CD-ROM, CF card) has been inserted.
//! The data parameter is an integer that specifies the data source's instance ID.
//! The default event handler calls NotifyMediaInserted in the content manager,
//! which tells the data source manager to begin a content update using the data source's
//! default parameters. \hideinitializer
#define EVENT_MEDIA_INSERTED   0x31
//! Sent from the data source when it detects corrupt or improper media (e.g., DVD in a CD-ROM drive).
//! The data parameter is an integer that specifies the data source's instance ID.
//! The default event handler calls NotifyMediaInserted in the content manager,
//! which tells the data source manager to begin a content update using the data source's
//! default parameters. \hideinitializer
#define EVENT_MEDIA_BAD        0x32

//! Sent from the NetDataSource when the network is up
//! The data parameter is an integer that specifies the data source's instance ID.
#define EVENT_NETWORK_UP				0x50

//! Sent from the NetDataSource when the network goes down (unplugged etc)
//! The data parameter is an integer that specifies the data source's instance ID.
#define EVENT_NETWORK_DOWN				0x51

//! An active recording has run down free drive space to the breaking point.
//! Stop all recording and notify the user.
#define EVENT_SPACE_LOW                 0x60

//! An active recording has run down free drive space to the breaking point.
//! Stop all recording and notify the user.
#define EVENT_RECORDING_TIMELIMIT       0x61

//! Sent from the serial control module to request that its callback function
//! be called in the UI thread's context
#define EVENT_CONTROL_SERVICE_REQUEST	0x62

//! Sent from the web control module to request that its callback function
//! be called in the UI thread's context
#define EVENT_WEBCONTROL_SERVICE_REQUEST	0x63

//! The system has been signalled to shut down
#define EVENT_SYSTEM_SHUTDOWN           0xFF
//@}

#endif  // SYSTEMEVENTS_H_
