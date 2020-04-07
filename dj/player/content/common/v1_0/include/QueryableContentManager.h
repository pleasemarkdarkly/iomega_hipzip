//
// QueryableContentManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

/** \addtogroup ContentManager Content Manager */
//@{

#ifndef QUERYABLECONTENTMANAGER_H_
#define QUERYABLECONTENTMANAGER_H_

#include <content/common/ContentManager.h>

//////////////////////////////////////////////////////////////////////////////////////////
//	Key/value pair struct
//////////////////////////////////////////////////////////////////////////////////////////

//! Requests that all values for the given key parameter be returned. \hideinitializer
#define CMK_ALL 0

//! Contains a key/value pair from the content manager.
typedef struct
{
    int             iKey;
    const TCHAR*    szValue;
} cm_key_value_record_t;

typedef SimpleVector<cm_key_value_record_t> ContentKeyValueVector;

//////////////////////////////////////////////////////////////////////////////////////////
//	IQueryableContentManager
//////////////////////////////////////////////////////////////////////////////////////////

//! This class extends the IContentManager by providing functions for querying
//! content based on artist, album, and genre keys.
class IQueryableContentManager : public IContentManager
{
public:

	virtual ~IQueryableContentManager()
        { }

    //! Returns the artist key that matches the artist string passed in, or 0 if no matching artist was found.
    virtual int GetArtistKey(const TCHAR* szArtist) const = 0;
    //! Returns the album key that matches the album string passed in, or 0 if no matching album was found.
    virtual int GetAlbumKey(const TCHAR* szAlbum) const = 0;
    //! Returns the genre key that matches the genre string passed in, or 0 if no matching genre was found.
    virtual int GetGenreKey(const TCHAR* szGenre) const = 0;

    //! Returns the string of the artist with the given key.
    //! Returns 0 if no artist with a matching key was found.
    virtual const TCHAR* GetArtistByKey(int iArtistKey) const = 0;
    //! Returns the string of the album with the given key.
    //! Returns 0 if no album with a matching key was found.
    virtual const TCHAR* GetAlbumByKey(int iAlbumKey) const = 0;
    //! Returns the string of the genre with the given key.
    //! Returns 0 if no genre with a matching key was found.
    virtual const TCHAR* GetGenreByKey(int iGenreKey) const = 0;

    //! Searches the content manager for media records that match the artist, album, genre, and data source
    //! keys.
    //! \param records List used to append matching records.
    virtual void GetMediaRecords(MediaRecordList& records,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const = 0;

    //! Searches the content manager for artists that have tracks with the specified album, genre,
    //! and data source keys.
    //! \param keyValues List used to append matching artist key/value pairs.
    virtual void GetArtists(ContentKeyValueVector& keyValues,
        int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const = 0;
    //! Searches the content manager for albums that have tracks with the specified artist, genre,
    //! and data source keys.
    //! \param keyValues List used to append matching album key/value pairs.
    virtual void GetAlbums(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const = 0;
    //! Searches the content manager for genre that have tracks with the specified artist, album,
    //! and data source keys.
    //! \param keyValues List used to append matching genre key/value pairs.
    virtual void GetGenres(ContentKeyValueVector& keyValues,
        int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
        int iDataSourceID = CMK_ALL) const = 0;
};

//@}

#endif	// QUERYABLECONTENTMANAGER_H_
