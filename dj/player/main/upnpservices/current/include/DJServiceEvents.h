//
// DJServiceEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of events sent by the UPnP test harness code to the UI for the dj project.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef DJSERVICEEVENTS_H_
#define DJSERVICEEVENTS_H_

//! Sent from the UPnP thread when the clear content function is invoked.
#define EVENT_CLEAR_CONTENT     0x300

//! Sent from the UPnP thread when the refresh content function is invoked.
#define EVENT_REFRESH_CONTENT   0x301

//! Sent from the UPnP thread when the clear CD cache function is invoked.
#define EVENT_CLEAR_CD_CACHE    0x302

//! Sent from the UPnP thread when the reset CDDB function is invoked.
#define EVENT_RESET_CDDB        0x303


//@}

#endif  // DJSERVICEEVENTS_H_
