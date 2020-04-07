//
// CDDBEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of events sent by CDDB to the UI for the dj project.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef CDDBEVENTS_H_
#define CDDBEVENTS_H_

//! Sent from the CDDB query thread when an online update starts.
#define EVENT_CDDB_ONLINE_UPDATE_BEGIN          0x400

//! Sent from the CDDB query thread when an online update finishes.
#define EVENT_CDDB_ONLINE_UPDATE_END            0x401

//! Sent from the CDDB query thread when an error occurs during an online update.
//! The data parameter is the gn_error_t error code.
#define EVENT_CDDB_ONLINE_UPDATE_ERROR          0x402

//! Sent from the CDDB query thread when we start downloading an update file.
//! The data paramater is an integer.
//! The lower 16 bits is the 1-based index of the current file.
//! The upper 16 bits is the total number of files to download.
#define EVENT_CDDB_ONLINE_UPDATE_DOWNLOADING    0x403

//! Sent from the CDDB query thread when we start processing an update file.
//! The data paramater is an integer.
//! The lower 16 bits is the 1-based index of the current file.
//! The upper 16 bits is the total number of files to download.
#define EVENT_CDDB_ONLINE_UPDATE_PROCESSING     0x404

//@}

#endif  // CDDBEVENTS_H_
