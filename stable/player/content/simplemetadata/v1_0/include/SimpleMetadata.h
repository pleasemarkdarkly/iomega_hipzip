//
// SimpleMetadata.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef SIMPLEMETADATA_H_
#define SIMPLEMETADATA_H_

#include <content/common/Metadata.h>
#include <util/tchar/tchar.h>

//! This class is a simple implementation of the IMetadata class, providing support
//! for the MDA_TITLE, MDA_ARTIST, MDA_ALBUM, MDA_TRACK_NUMBER, and MDA_GENRE fields.
class CSimpleMetadata : public IMetadata
{
public:

    CSimpleMetadata();
    ~CSimpleMetadata();

    // IMetadata functions.
    IMetadata* Copy() const;
    bool UsesAttribute(int iAttributeID) const;
    ERESULT SetAttribute(int iAttributeID, void* pAttributeValue);
    ERESULT UnsetAttribute(int iAttributeID);
    ERESULT GetAttribute(int iAttributeID, void** ppAttributeValue) const;
    void ClearAttributes();
    void MergeAttributes(const IMetadata* pMetadata, bool bOverwrite = false);

    //! Convenience function for getting the title.
    //! \retval 0 No title set.
    const TCHAR* GetTitle() const
        { return m_tszTitle; }
    //! Convenience function for getting the artist.
    //! \retval 0 No artist set.
    const TCHAR* GetArtist() const
        { return m_tszArtist; }
    //! Convenience function for getting the album.
    //! \retval 0 No album set.
    const TCHAR* GetAlbum() const
        { return m_tszAlbum; }
    //! Convenience function for getting the album track number.
    //! \retval 0 No album track number set.
    int GetAlbumTrackNumber() const
        { return m_iAlbumTrackNumber; }
    //! Convenience function for getting the genre.
    //! \retval 0 No genre set.
    const TCHAR* GetGenre() const
        { return m_tszGenre; }

private:

    TCHAR*  m_tszTitle;
    TCHAR*  m_tszArtist;
    TCHAR*  m_tszAlbum;
    TCHAR*  m_tszGenre;
    int     m_iAlbumTrackNumber;
};


#endif	// SIMPLEMETADATA_H_

