//
// ContentDeleteEvents.h
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

#ifndef CONTENTDELETEEVENTS_H_
#define CONTENTDELETEEVENTS_H_


typedef struct content_delete_info_s
{
    int iGenreID;   //!< Genre ID of the content to be deleted.
    int iArtistID;  //!< Artist ID of the content to be deleted.
    int iAlbumID;   //!< Album ID of the content to be deleted.
    int nDeleted;   //!< Number of items deleted so far.
} content_delete_info_t;

//! Sent to delete a chunk of content.
//! The data parameter is a pointer to a content_delete_info_t struct.
#define EVENT_DELETE_CONTENT 0x700


//@}

#endif  // CONTENTDELETEEVENTS_H_


