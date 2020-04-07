//
// SimplerMetadata.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "SimplerMetadata.h"

#include <util/debug/debug.h>
#include <stdlib.h> // free

CSimplerMetadata::CSimplerMetadata()
    : m_tszTitle(0),
    m_tszArtist(0)
{
}

CSimplerMetadata::~CSimplerMetadata()
{
    free(m_tszTitle);
    free(m_tszArtist);
}

IMetadata*
CSimplerMetadata::Copy() const
{
    CSimplerMetadata* pCopy = new CSimplerMetadata;
    pCopy->MergeAttributes(this, true);
    return pCopy;
}

bool
CSimplerMetadata::UsesAttribute(int iAttributeID) const
{
    if ((iAttributeID == MDA_TITLE) ||
            (iAttributeID == MDA_ARTIST) ||
            (iAttributeID == MDA_ALBUM) ||
            (iAttributeID == MDA_GENRE) ||
            (iAttributeID == MDA_ALBUM_TRACK_NUMBER))
    {
        return true;
    }
    else
    {
        return false;
    }
}

ERESULT
CSimplerMetadata::SetAttribute(int iAttributeID, void* pAttributeValue)
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
            if (m_tszTitle)
                free(m_tszTitle);
            m_tszTitle = pAttributeValue ? tstrdup((TCHAR*)pAttributeValue) : 0;
            return METADATA_NO_ERROR;

        case MDA_ARTIST:
            if (m_tszArtist)
                free(m_tszArtist);
            m_tszArtist = pAttributeValue ? tstrdup((TCHAR*)pAttributeValue) : 0;
            return METADATA_NO_ERROR;

        default:
            return METADATA_NOT_USED;
    };
}

ERESULT
CSimplerMetadata::UnsetAttribute(int iAttributeID)
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
            free(m_tszTitle);
            m_tszTitle = 0;
            return METADATA_NO_ERROR;

        case MDA_ARTIST:
            free(m_tszArtist);
            m_tszArtist = 0;
            return METADATA_NO_ERROR;

        default:
            return METADATA_NOT_USED;
    };
}

static ERESULT GetTcharAttribute(TCHAR* tszAttribute, TCHAR** ptszAttributeValue)
{
    if (tszAttribute)
    {
        *ptszAttributeValue = tszAttribute;
        return METADATA_NO_ERROR;
    }
    else
    {
        return METADATA_NO_VALUE_SET;
    }
}

ERESULT
CSimplerMetadata::GetAttribute(int iAttributeID, void** ppAttributeValue) const
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
            return GetTcharAttribute(m_tszTitle, (TCHAR**)ppAttributeValue);

        case MDA_ARTIST:
            return GetTcharAttribute(m_tszArtist, (TCHAR**)ppAttributeValue);

        default:
            return METADATA_NOT_USED;
    };
}

void
CSimplerMetadata::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite)
{
    TCHAR* tszValue;
    if ((!m_tszTitle || bOverwrite) && SUCCEEDED(pMetadata->GetAttribute(MDA_TITLE, (void**)&tszValue)))
    {
        free(m_tszTitle);
        m_tszTitle = tszValue ? tstrdup(tszValue) : 0;
    }
    if ((!m_tszArtist || bOverwrite) && SUCCEEDED(pMetadata->GetAttribute(MDA_ARTIST, (void**)&tszValue)))
    {
        free(m_tszArtist);
        m_tszArtist = tszValue ? tstrdup(tszValue) : 0;
    }
}

void
CSimplerMetadata::ClearAttributes()
{
    free(m_tszTitle);
    free(m_tszArtist);
    m_tszTitle = m_tszArtist = 0;
}
