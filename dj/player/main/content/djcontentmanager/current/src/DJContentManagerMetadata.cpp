//
// DJContentManagerMetadata.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "DJContentManagerMetadata.h"

#include <content/common/Metadata.h>
#include <content/metadatatable/MetadataTable.h>
#include <datasource/common/DataSource.h>
#include <datastream/input/InputStream.h>
#include <datastream/output/OutputStream.h>
#include <playlist/common/Playlist.h>

#include <string.h> /* tstrncpy */
#include <stdlib.h> /* free */
#include <stdio.h>  /* sprintf */
#include <util/debug/debug.h>
#include <util/diag/diag.h>

DEBUG_USE_MODULE(DJCONTENTMANAGER);

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJContentManagerMetadata
//////////////////////////////////////////////////////////////////////////////////////////

CDJContentManagerMetadata::MetadataTypeMap CDJContentManagerMetadata::sm_smMetadataToUse;

CDJContentManagerMetadata::CDJContentManagerMetadata()
{
}

CDJContentManagerMetadata::~CDJContentManagerMetadata()
{
    FreeAttributes();
}

// Add a metadata type to be used by all CDJContentManagerMetadata objects.
void CDJContentManagerMetadata::AddAttribute(int iAttributeID)
{
    int iAttributeType = CMetadataTable::GetInstance()->FindMetadataTypeByID(iAttributeID);
    if (iAttributeType != MDT_INVALID_TYPE)
    {
        sm_smMetadataToUse.AddEntry(iAttributeID, iAttributeType);
    }
}

// Removes all metadata types from the list.
void CDJContentManagerMetadata::RemoveAllAttributes()
{
    sm_smMetadataToUse.Clear();
}

// IMetadata functions
IMetadata* CDJContentManagerMetadata::Copy() const
{
    CDJContentManagerMetadata* pCopy = new CDJContentManagerMetadata;
    pCopy->MergeAttributes(this, true);
    return pCopy;
}

bool CDJContentManagerMetadata::UsesAttribute(int iAttributeID) const
{
    int iAttributeType;
    if (sm_smMetadataToUse.FindEntry(iAttributeID, &iAttributeType))
        return true;
    else
        return false;
}

ERESULT CDJContentManagerMetadata::SetAttribute(int iAttributeID, void* pAttributeValue)
{
    int iAttributeType;
    if (sm_smMetadataToUse.FindEntry(iAttributeID, &iAttributeType))
        return SetAttribute(iAttributeID, iAttributeType, pAttributeValue);
    else
        return METADATA_NOT_USED;
}

ERESULT CDJContentManagerMetadata::UnsetAttribute(int iAttributeID)
{
    md_value_holder_t mdValue;
    if (m_smMetadataValues.RemoveEntryByKey(iAttributeID, &mdValue))
    {
        if (mdValue.iAttributeType == MDT_TCHAR)
            free(mdValue.pAttributeValue);
        return METADATA_NO_ERROR;
    }
    else
    {
        // No value found, so check if the value is not used or not set.
        if (sm_smMetadataToUse.FindEntry(iAttributeID, &mdValue.iAttributeType))
            return METADATA_NO_VALUE_SET;
        else
            return METADATA_NOT_USED;
    }
}

ERESULT CDJContentManagerMetadata::GetAttribute(int iAttributeID, void** ppAttributeValue) const
{
    md_value_holder_t mdValue;
    if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
    {
        *ppAttributeValue = mdValue.pAttributeValue;
        return METADATA_NO_ERROR;
    }
    else
    {
        // No value found, so check if the value is not used or not set.
        if (sm_smMetadataToUse.FindEntry(iAttributeID, &mdValue.iAttributeType))
            return METADATA_NO_VALUE_SET;
        else
            return METADATA_NOT_USED;
    }
}

void CDJContentManagerMetadata::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false)
{
    for (int i = 0; i < sm_smMetadataToUse.Size(); ++i)
    {
        int iAttributeID;
        int iAttributeType;
        if (sm_smMetadataToUse.GetEntry(i, &iAttributeID, &iAttributeType))
        {
            // If we're not in overwrite mode, then check to see if a value exists before setting.
            if (!bOverwrite)
            {
                md_value_holder_t mdValue;
                if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
                    continue;
            }

            void* pAttributeValue;
            if (SUCCEEDED(pMetadata->GetAttribute(iAttributeID, &pAttributeValue)))
                SetAttribute(iAttributeID, iAttributeType, pAttributeValue);
        }
    }
}

void CDJContentManagerMetadata::ClearAttributes()
{
    FreeAttributes();
    m_smMetadataValues.Clear();
}

ERESULT CDJContentManagerMetadata::SetAttribute(int iAttributeID, int iAttributeType, void* pAttributeValue)
{
    switch (iAttributeType)
    {
        case MDT_INT:
        {
            // No memory is allocated to store this value, so go ahead and write
            // a record without checking to see if a previous value has already been stored.
            md_value_holder_t mdValue;
            mdValue.iAttributeType = iAttributeType;
            mdValue.pAttributeValue = pAttributeValue;
            m_smMetadataValues.AddEntry(iAttributeID, mdValue);
            return METADATA_NO_ERROR;
        }

        case MDT_TCHAR:
        {
            // If a value for this entry has already been stored, then first free that memory.
            md_value_holder_t mdValue;
            if (m_smMetadataValues.FindEntry(iAttributeID, &mdValue))
                free(mdValue.pAttributeValue);
            else
                mdValue.iAttributeType = iAttributeType;
            mdValue.pAttributeValue = (void*)tstrdup((TCHAR*)pAttributeValue);
            m_smMetadataValues.AddEntry(iAttributeID, mdValue);
            return METADATA_NO_ERROR;
        }
    }
    return METADATA_ERROR;
}

// Frees any memory allocated to hold attribute values.
// Doesn't clear the attribute list, though.
void CDJContentManagerMetadata::FreeAttributes()
{
    // Free any stored strings.
    for (int i = 0; i < m_smMetadataValues.Size(); ++i)
    {
        int key;
        md_value_holder_t mdValue;
        if (m_smMetadataValues.GetEntry(i, &key, &mdValue) && (mdValue.iAttributeType == MDT_TCHAR))
            free(mdValue.pAttributeValue);
    }
}



