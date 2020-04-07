//
// ConfigurableMetadata.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CONFIGURABLEMETADATA_H_
#define CONFIGURABLEMETADATA_H_

#include <content/common/Metadata.h>
#include <content/metadatatable/MetadataTable.h>
#include <util/datastructures/SimpleMap.h>
#include <util/tchar/tchar.h>

//! The configurable metadata class allows clients to specify which attributes
//! should be stored by this class.  Attributes added to the class are added to
//! all instances of the CConfigurableMetadata.
class CConfigurableMetadata : public IMetadata
{
public:

    CConfigurableMetadata();
    ~CConfigurableMetadata();

    //! Add a metadata type to be used by all CConfigurableMetadata objects.
    static void AddAttribute(int iAttributeID);
    //! Removes all metadata types from the list.
    static void RemoveAllAttributes();

    // IMetadata functions
    IMetadata* Copy() const;
    bool UsesAttribute(int iAttributeID) const;
    ERESULT SetAttribute(int iAttributeID, void* pAttributeValue);
    ERESULT UnsetAttribute(int iAttributeID);
    ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const;
    void ClearAttributes();
    void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false);

private:

typedef SimpleMap<int /* Attribute ID */, int /* Attribute type */> MetadataTypeMap;

    static MetadataTypeMap    sm_smMetadataToUse;

typedef struct md_value_holder_s
{
    int     iAttributeType;
    void*   pAttributeValue;
} md_value_holder_t;
typedef SimpleMap<int /* Attribute ID */, md_value_holder_t> MetadataValueMap;

    MetadataValueMap  m_smMetadataValues;

    ERESULT SetAttribute(int iAttributeID, int iAttributeType, void* pAttributeValue);
    // Frees any memory allocated to hold attribute values.
    // Doesn't clear the attribute list, though.
    void FreeAttributes();

};


#endif	// CONFIGURABLEMETADATA_H_

