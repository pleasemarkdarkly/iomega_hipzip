//
// CDMetadataEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CDMETADATAEVENTS_H_
#define CDMETADATAEVENTS_H_

#include <util/datastructures/SimpleVector.h>

// fdecl
class CCDDataSource;
class IDiskInfo;
typedef SimpleVector<IDiskInfo*> DiskInfoVector;

//! The structure that accompanies a EVENT_CD_METADATA_MULTIPLE_HITS event.
//! Contains the freedb disk ID and the list of matching discs.
typedef struct cd_multiple_hit_event_s
{
    int             iDataSourceID;  //!< ID of the CD data source that sent this event.
    unsigned int    uiDiskID;       //!< The disk ID calculated for freedb.
    DiskInfoVector  svDisks;        //!< Array of discs that match the ID.
} cd_multiple_hit_event_t;

//! Sent when a disk matches multiple records in the metadata database.
//! The data parameter is a cd_multiple_hit_event_t struct, which contains the disk ID and a
//! list of matching disk records.
//! The default event handler clears the disk record list and frees the cd_multiple_hit_event_t struct.
//! \hideinitializer
#define EVENT_CD_METADATA_MULTIPLE_HITS         0x40

//! Sent when a disk has no matches in the metadata database.
//! The data parameter is the disk ID sent in the cd_multiple_hit_event_t event. \hideinitializer
#define EVENT_CD_METADATA_NO_HITS               0x41

//! Sent when an online lookup fails.
//! The data parameter is an error code. \hideinitializer
#define EVENT_CD_METADATA_ONLINE_LOOKUP_ERROR   0x42

//! Sent when the user rejects all choices for CD metadata selection.
//! The data parameter is the disk ID sent in the cd_multiple_hit_event_t event. \hideinitializer
#define EVENT_CD_METADATA_NO_SELECTION          0x43

//! Sent when the user makes a selection for CD metadata
//! The data parameter is the disk ID sent in the cd_multiple_hit_event_t even. \hideinitializer
#define EVENT_CD_METADATA_SELECTED              0x44

#endif // CDMETADATAEVENTS_H_
