//
// SimpleMetadata.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <content/simplemetadata/SimpleMetadata.h>

#include <util/debug/debug.h>
#include <stdlib.h> // free

CSimpleMetadata::CSimpleMetadata()
    : m_tszTitle(0),
    m_tszArtist(0),
    m_tszAlbum(0),
    m_tszGenre(0),
    m_iAlbumTrackNumber(0)
{
}

CSimpleMetadata::~CSimpleMetadata()
{
    free(m_tszTitle);
    free(m_tszArtist);
    free(m_tszAlbum);
    free(m_tszGenre);
}

IMetadata*
CSimpleMetadata::Copy() const
{
    CSimpleMetadata* pCopy = new CSimpleMetadata;
    pCopy->MergeAttributes(this, true);
    return pCopy;
}

bool
CSimpleMetadata::UsesAttribute(int iAttributeID) const
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
CSimpleMetadata::SetAttribute(int iAttributeID, void* pAttributeValue)
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

        case MDA_ALBUM:
            if (m_tszAlbum)
                free(m_tszAlbum);
            m_tszAlbum = pAttributeValue ? tstrdup((TCHAR*)pAttributeValue) : 0;
            return METADATA_NO_ERROR;

        case MDA_ALBUM_TRACK_NUMBER:
            m_iAlbumTrackNumber = (int)pAttributeValue;
            return METADATA_NO_ERROR;

        case MDA_GENRE:
            if (m_tszGenre)
                free(m_tszGenre);
            m_tszGenre = pAttributeValue ? tstrdup((TCHAR*)pAttributeValue) : 0;
            return METADATA_NO_ERROR;

        default:
            return METADATA_NOT_USED;
    };
}

ERESULT
CSimpleMetadata::UnsetAttribute(int iAttributeID)
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

        case MDA_ALBUM:
            free(m_tszAlbum);
            m_tszAlbum = 0;
            return METADATA_NO_ERROR;

        case MDA_ALBUM_TRACK_NUMBER:
            m_iAlbumTrackNumber = 0;
            return METADATA_NO_ERROR;

        case MDA_GENRE:
            free(m_tszGenre);
            m_tszGenre = 0;
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
CSimpleMetadata::GetAttribute(int iAttributeID, void** ppAttributeValue) const
{
    switch (iAttributeID)
    {
        case MDA_TITLE:
            return GetTcharAttribute(m_tszTitle, (TCHAR**)ppAttributeValue);

        case MDA_ARTIST:
            return GetTcharAttribute(m_tszArtist, (TCHAR**)ppAttributeValue);

        case MDA_ALBUM:
            return GetTcharAttribute(m_tszAlbum, (TCHAR**)ppAttributeValue);

        case MDA_ALBUM_TRACK_NUMBER:
            if (m_iAlbumTrackNumber)
            {
                *ppAttributeValue = (void*)m_iAlbumTrackNumber;
                return METADATA_NO_ERROR;
            }
            else
                return METADATA_NO_VALUE_SET;

        case MDA_GENRE:
            return GetTcharAttribute(m_tszGenre, (TCHAR**)ppAttributeValue);

        default:
            return METADATA_NOT_USED;
    };
}

void
CSimpleMetadata::MergeAttributes(const IMetadata* pMetadata, bool bOverwrite)
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
    if ((!m_tszAlbum || bOverwrite) && SUCCEEDED(pMetadata->GetAttribute(MDA_ALBUM, (void**)&tszValue)))
    {
        free(m_tszAlbum);
        m_tszAlbum = tszValue ? tstrdup(tszValue) : 0;
    }
    if (!m_iAlbumTrackNumber || bOverwrite)
        pMetadata->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&m_iAlbumTrackNumber);
    if ((!m_tszGenre || bOverwrite) && SUCCEEDED(pMetadata->GetAttribute(MDA_GENRE, (void**)&tszValue)))
    {
        free(m_tszGenre);
        m_tszGenre = tszValue ? tstrdup(tszValue) : 0;
    }
}

void
CSimpleMetadata::ClearAttributes()
{
    free(m_tszTitle);
    free(m_tszArtist);
    free(m_tszAlbum);
    free(m_tszGenre);
    m_tszTitle = m_tszArtist = m_tszAlbum = m_tszGenre = 0;
    m_iAlbumTrackNumber = 0;
}
