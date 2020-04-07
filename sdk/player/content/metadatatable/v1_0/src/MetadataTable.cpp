//........................................................................................
//........................................................................................
//.. File Name: MetadataTable.cpp                                                       ..
//.. Date: 9/1/2001                                                                     ..
//.. Author(s): Ed Miller                                                               ..
//.. Last Modified By: Ed Miller  edwardm@iobjects.com                                  ..
//.. Modification date: 9/1/2001                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <content/metadatatable/MetadataTable.h>

CMetadataTable* CMetadataTable::m_pSingleton = 0;

CMetadataTable*
CMetadataTable::GetInstance() 
{
    if( m_pSingleton == 0 ) {
        m_pSingleton = new CMetadataTable;
    }
    return m_pSingleton;
}

void
CMetadataTable::Destroy() 
{
    delete m_pSingleton;
}

CMetadataTable::CMetadataTable() 
{
    // Add the standard metadata attributes.
    m_smMetadataTypeMap.AddEntry(MDA_TITLE, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_ARTIST, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_ALBUM, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_GENRE, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_FILE_NAME, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_ALBUM_TRACK_NUMBER, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_COMMENT, MDT_TCHAR);
    m_smMetadataTypeMap.AddEntry(MDA_YEAR, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_HAS_DRM, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_DURATION, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_FILE_SIZE, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_DURATION, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_SAMPLING_FREQUENCY, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_CHANNELS, MDT_INT);
    m_smMetadataTypeMap.AddEntry(MDA_BITRATE, MDT_INT);
}

CMetadataTable::~CMetadataTable() 
{
}

// Adds a metadata attribute with the specified ID and type to the table.
// If there already exists and attribute with the same ID, then it is not added to the
// table and METADATA_TABLE_ID_IN_USE is returned.
// If the type passed in is invalid, then METADATA_TABLE_INVALID_TYPE is returned.
// Otherwise METADATA_NO_ERROR is returned.
ERESULT
CMetadataTable::AddMetadataAttribute(int iAttributeID, int iAttributeType)
{
    // Make sure there's no matching metadata with the same ID.
    if (FindMetadataTypeByID(iAttributeID) == MDA_INVALID_ID)
    {
        // Make sure this is a valid type.
        if ((iAttributeType != MDT_INT) &&
                (iAttributeType != MDT_TCHAR))
            return METADATA_TABLE_INVALID_TYPE;

        // Add the attribute to the table.
        m_smMetadataTypeMap.AddEntry(iAttributeID, iAttributeType);
    }
    else
        return METADATA_TABLE_ID_IN_USE;
}

// Given an attribute ID, this function returns a pointer to the corresponding metadata info struct,
// or MDA_INVALID_ID if no matching metadata ID was found.
int
CMetadataTable::FindMetadataTypeByID(int iAttributeID) const
{
    int iAttributeType;
    if (m_smMetadataTypeMap.FindEntry(iAttributeID, &iAttributeType))
        return iAttributeType;
    else
        return MDA_INVALID_ID;
}
