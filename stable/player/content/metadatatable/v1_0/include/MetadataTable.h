//
// MetadataTable.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

/** \addtogroup Metadata Metadata */
//@{

#ifndef METADATATABLE_H_
#define METADATATABLE_H_

#include <content/common/Metadata.h>
#include <util/datastructures/SimpleMap.h>

//! The ID for the given attribute type is not valid. \hideinitializer
const ERESULT METADATA_TABLE_INVALID_TYPE = MAKE_METADATA_RESULT( SEVERITY_FAILED,  0x0004 );
//! The ID for the metadata attribute is already in use. \hideinitializer
const ERESULT METADATA_TABLE_ID_IN_USE    = MAKE_METADATA_RESULT( SEVERITY_FAILED,  0x0005 );

//! The CMetadataTable class matches metadata attribute IDs to types.
//! It's a singleton class that when created adds all the standard metadata types to its table.
//! If client code wants to add a new metadata attribute, then it must pick an attribute ID not
//! currently in use and add it to this table along with a type for the metadata.
class CMetadataTable
{
public:

    //! Returns a pointer to the global metadata table.
    static CMetadataTable* GetInstance();

    //! Destroy the singleton global metadata table.
    static void Destroy();

    //! Given an attribute ID this function returns the type for that attribute
    //! or MDA_INVALID_ID if no matching metadata ID was found.
    int FindMetadataTypeByID(int iAttributeID) const;

    //! Adds a metadata attribute with the specified ID and type to the table.
    //! \retval METADATA_NO_ERROR The attribute was added successfully.
    //! \retval METADATA_TABLE_ID_IN_USE There already exists and attribute with the same ID.
    //! \retval METADATA_TABLE_INVALID_TYPE The type passed in is invalid.
    ERESULT AddMetadataAttribute(int iAttributeID, int iAttributeType);

private:

    CMetadataTable();
    ~CMetadataTable();

    static CMetadataTable* m_pSingleton;

typedef SimpleMap<int /* Attribute ID */, int /* Attribute type */> MetadataTypeMap;

    MetadataTypeMap m_smMetadataTypeMap;
};


#endif	// METADATATABLE_H_

//@}
