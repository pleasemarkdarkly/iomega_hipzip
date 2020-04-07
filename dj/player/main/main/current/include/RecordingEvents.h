//
// RecordingEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of events sent by the recording code to the UI for the dj project.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef RECORDINGEVENTS_H_
#define RECORDINGEVENTS_H_

//! Sent from the idle coder thread when starting to encode a track.
#define EVENT_IDLE_CODING_START     0x200

//! Sent from the idle coder thread when a track is finished encoding.
//! The data parameter is a idle_coder_finish_t struct.
//! The szInURL and szOutURL fields of the idle_coder_finish_t struct should be delete[]'d by the event handler,
//! and the struct itself should be deleted.
#define EVENT_IDLE_CODING_FINISH    0x201

typedef struct idle_coder_finish_s
{
    char*   szInURL;        //!< URL of the track that was encoded.
    char*   szOutURL;       //!< URL of the encoded track.
} idle_coder_finish_t;

//! Sent from the file copier thread when it finishes copying a track.
//! The data parameter is a file_copier_finish_t struct.
//! The szTempURL and szFinalURL fields of the file_copier_finish_t struct should be delete[]'d by the event handler,
//! and the struct itself should be deleted.
#define EVENT_FILE_COPIER_FINISH    0x202

//! Sent form the file copier thread when an error occurs while copying a track.
//! There is no argument for this event
#define EVENT_FILE_COPIER_ERROR     0x203

class IMetadata;
typedef struct file_copier_finish_s
{
    char*       szTempURL;      //!< URL of the file copied to the HD.
    char*       szFinalURL;     //!< URL to move the temp file to.
    int         iCodecID;       //!< Codec ID for the source file.
    IMetadata*  pMetadata;      //!< Metadata for the source file.
    bool        bScanFile;      //!< True if the file should be scanned for metadata.
} file_copier_finish_t;

//! Sent from the main thread when a partial file needs to be deleted.
//! The data parameter is the name of the file, which should be delete[]'d when done.
#define EVENT_DELETE_PARTIAL_RECORDING  0x204

//! Sent from the Line Recording thread to notify the UI that a clip has occured.
#define EVENT_CLIP_DETECTED 0x205

#define EVENT_LINERECORDER_CONTENT_ADDITION 0x206

//@}

#endif  // RECORDINGEVENTS_H_
