//
// Metadata.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The IMetadata class is used by the Dadio(tm) system to associate a set of key/value
//! pairs with a media item.  IMetadata records are created mainly by two sources:
//! the data source during content update, and the media player when a track is set.
//! The content manager provides its own IMetadata subclass, and the media player can
//! be told what type of IMetadata subclass to produce.
/** \addtogroup Metadata Metadata */
//@{

#ifndef METADATA_H_
#define METADATA_H_

#include <util/eresult/eresult.h>

//////////////////////////////////////////////////////////////////////////////////////////
//	Metadata
//////////////////////////////////////////////////////////////////////////////////////////

//
//! The metadata error zone for eresult error codes
//
#define METADATA_TYPE_ID   0x0e

#define MAKE_METADATA_RESULT( x, y ) MAKE_ERESULT( x, METADATA_TYPE_ID, y )

//! No error was encountered during the operation. \hideinitializer
const ERESULT METADATA_NO_ERROR     = MAKE_METADATA_RESULT( SEVERITY_SUCCESS, 0x0000 );
//! An unspecified error occurred during the operation. \hideinitializer
const ERESULT METADATA_ERROR        = MAKE_METADATA_RESULT( SEVERITY_SUCCESS, 0x0001 );
//! The attribute is not used. \hideinitializer
const ERESULT METADATA_NOT_USED     = MAKE_METADATA_RESULT( SEVERITY_FAILED,  0x0002 );
//! The attribute is used but no value has been set. \hideinitializer
const ERESULT METADATA_NO_VALUE_SET = MAKE_METADATA_RESULT( SEVERITY_FAILED,  0x0003 );

#define MDA_INVALID_ID          0   /*!< 0 is reserved for indicating an invalid metadata ID. */
#define MDA_TITLE               1   /*!< TCHAR.  The title of the track. \hideinitializer */
#define MDA_ARTIST              2   /*!< TCHAR.  The track's artist. \hideinitializer */
#define MDA_ALBUM               3   /*!< TCHAR.  The album the track belongs to. \hideinitializer */
#define MDA_GENRE               4   /*!< TCHAR.  The genre of the track. \hideinitializer */
#define MDA_FILE_NAME           5   /*!< TCHAR.  The track's file name in the data source. \hideinitializer */
#define MDA_FILE_SIZE           6   /*!< int.  The size of the track in bytes. \hideinitializer */
#define MDA_ALBUM_TRACK_NUMBER  7   /*!< int.  The 1-based index of the track on the album. \hideinitializer */
#define MDA_COMMENT             8   /*!< TCHAR.  Comments on the track stored in the codec. \hideinitializer */
#define MDA_YEAR                9   /*!< int.  Year of the track. \hideinitializer */
#define MDA_HAS_DRM             10  /*!< int.  1 = the track has drm, 0 = no drm. \hideinitializer */
#define MDA_DURATION            11  /*!< int.  Duration of the track in seconds. \hideinitializer */
#define MDA_SAMPLING_FREQUENCY  12  /*!< int.  Sampling frequency of the track in hz. \hideinitializer */
#define MDA_CHANNELS            13  /*!< int.  Number of channels. \hideinitializer */
#define MDA_BITRATE             14  /*!< int.  Bitrate of the track, in bits per second. \hideinitializer */

#define MDT_INVALID_TYPE    0   /*!< 0 is reserved for indicating an invalid metadata type */
#define MDT_INT             1   /*!< Integer type \hideinitializer */
#define MDT_TCHAR           2   /*!< TCHAR type \hideinitializer */

//! The metadata class is an abstract representation of a collection of key/value pairs for a track.
//! Functions are provided for setting, getting, and clearing attributes.
class IMetadata
{
public:

    virtual ~IMetadata()
        { }

    //! Makes a copy of the metadata object.
    virtual IMetadata* Copy() const = 0;

    //! Returns true if the attribute is used by this metadata object, false otherwise.
    virtual bool UsesAttribute(int iAttributeID) const = 0;

    //! Sets a value of an attribute.
    //! \retval METADATA_NO_ERROR The value was set successfully.
    //! \retval METADATA_NOT_USED The attribute isn't used by this metadata object.
    virtual ERESULT SetAttribute(int iAttributeID, void* pAttributeValue) = 0;

    //! Unsets a value of an attribute and frees any memory associated with it.
    //! \retval METADATA_NO_ERROR The value was unset successfully.
    //! \retval METADATA_NOT_USED The attribute isn't used by this metadata object.
    virtual ERESULT UnsetAttribute(int iAttributeID) = 0;

    //! Gets the value of an attribute.
    //! \retval METADATA_NO_ERROR The value was retrieved successfully.
    //! \retval METADATA_NOT_USED The attribute isn't used by this metadata object.
    //! \retval METADATA_NO_VALUE_SET The attribute is used by this metadata object but no value has been set.
    virtual ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const = 0;

    //! Clears the value of all attributes.
    virtual void ClearAttributes() = 0;

    //! Merges the attributes used by this metadata object with the values for those attributes
    //! from the metadata object passed in.  Attributes used in the passed-in metadata that are
    //! not used by this object are not copied.
    //! \param bOverwrite If false, only attributes in this object that have no value set are merged.
    //! \param bOverwrite If true, all attributes in this object are merged, even if a previous value has been set.
    virtual void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false) = 0;
};

#endif	// METADATA_H_

//@}
