//
// File Name: IML.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef IML_H_
#define IML_H_


#include <main/djupnp/DJUPnP.h>

//#include <cyg/kernel/kapi.h>
#include <main/djupnp/UPnPEvents.h>
#include <main/djupnp/XMLDocs.h>
#include <util/datastructures/SimpleList.h>
#include <util/eventq/EventQueueAPI.h>
#include <util/upnp/api/upnp.h>



#include <stdlib.h>
#include <util/debug/debug.h>

//! Forward declarations
class CQueryResult;
class CMediaQueryResult;
class CGeneralQueryResult;
class CRadioStationQueryResult;

// max character length of keys
#define IML_MAX_KEY_LENGTH      6
#define IML_MAX_SORT_LENGTH     20
#define IML_MAX_MIMETYPE_LENGTH 15

//! The CIML class is a convenience class for querying remote iMLs.
class CIML
{
public:

    CIML(int iDeviceNumber, const char* szMediaBaseURL, const char* szFriendlyName, const char* szUDN);
    ~CIML();

    //! Returns the device number of the iML used by the UPnP code.
    int GetDeviceNumber() const
        { return m_iDeviceNumber; }
    //! Returns the base URL of the iML.
    const char* GetMediaBaseURL() const
        { return m_szMediaBaseURL; }
    //! Returns the friendly name of the device.
    const TCHAR* GetFriendlyName() const
        { return m_szFriendlyName; }
    //! Returns the iML's Unique Device Identifier.
    const char* GetUDN() const
        { return m_szUDN; }

    //! Returns an ID unique to the instantiation of the object.
    int GetInstanceID() const
        { return m_iIMLID; }

    //! Asks the iML to start caching top-level query results.
    void StartQueryCaching();

typedef enum ECachedStatus { NOT_CACHED, CACHING, CACHED };

    //! Returns the current stage of the query results cache.
    ECachedStatus GetCachedStatus() const
        { return m_eCacheStatus; }

    //! Sets the current stage of the query results cache.
    void SetCachedStatus(ECachedStatus eCacheStatus)
        { m_eCacheStatus = eCacheStatus; }

	//! Initiates a query for the artist key that matches the artist string passed in.
	//! The response will generate a QUERY_ARTIST_KEY_RESULT event.
	void QueryArtistKey(const TCHAR* szArtist);

	//! Iniaites a query for the album key that matches the album string passed in.
	//! The resposne will generate a QUERY_ALBUM_KEY_RESULT event.
    void QueryAlbumKey(const TCHAR* szAlbum);

    //! Initiates a query for the genre key that matches the genre string passed in.
	//! The response will generate a QUERY_GENRE_KEY_RESULT event;
    void QueryGenreKey(const TCHAR* szGenre);

    //! Initiates a query for the string of the artist with the given key.
    //! The response will generate a QUERY_ARTIST_BY_KEY_RESULT event;
    void QueryArtistByKey(int iArtistKey);

    //! Initiates a query for the string of the album with the given key.
    //! The response will generate a QUERY_ALBUM_BY_KEY_RESULT event;
    void QueryAlbumByKey(int iAlbumKey);

    //! Initiates a query for the genre with the given key.
    //! The response will generate a QUERY_GENRE_BY_KEY_RESULT event;
    void QueryGenreByKey(int iGenreKey);

	//! Searches the remote library for media records that match the artist, album, and genre
    //! keys.
	//! The response from this query will generate an QUERY_LIBRARY_RESULT event.
	//! \param iInitialItemsRequested The number of records requested.  Use 0 to just set up the view.
	//! \param szSortOrder String specifying element order.
	//! \param iPlayListKey If not CMK_ALL, will confine results to items in playlist with key iPlayListKey.
	CMediaQueryResult* InitialQueryLibrary(int iMediaKey = CMK_ALL,
		int iArtistKey = CMK_ALL,
		int iAlbumKey = CMK_ALL,
		int iGenreKey = CMK_ALL,
		int iPlaylistKey = CMK_ALL,
		char* szMediaType = NULL,
		int iInitialItemsRequested = 0,
		char* szSortOrder = NULL);

	//! Searched the remote library for media records that match the artist, album and genre keys.
	//! The response from this query will generate a QUERY_LIBRARY_RESULT event.
	//! \param iStartPosition The starting position
	//! \param iItemsRequested The number of records requested.
	//! \param iViewID  The view ID to use.  This was returned in an event from InitialQueryLibrary
	void ResumeQueryLibrary(int iStartPosition,
		int iItemsRequested,
		int iViewID,
		int iQueryID);

    void RequeryMedia(int iQueryID, char* szRequest);

	//! Releases the view on the database.  Call this when done with a particular query
	//! \param iViewID The view ID returned from the initial query.
	void ReleaseQueryView(int iViewID);

    //! Searches the remote library for artists.
	//! The results will generate an QUERY_ARTISTS_RESULT event

    CGeneralQueryResult* InitialQueryArtists(int iAlbumKey = CMK_ALL,
		int iGenreKey = CMK_ALL,
		int iItemsRequested = 0);

	//! Searches the remote library for artists that have tracks with the specified album, and genre.
	//! The query result will generate a QUERY_ARTISTS_RESULT event
    void ResumeQueryArtists(int iStartPosition,
		int iItemsRequested,
		int iViewID,
		int iQueryID);

    //! Searches the remote library for for albums that have tracks with the specified artist, and genre.
	//! The query result will generate an QUERY_ALBUMS_RESULT event
    CGeneralQueryResult* InitialQueryAlbums(int iArtistKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
		int iItemsRequested=0);
		
	//! Searches the remote library for for albums that have tracks with the specified artist, and genre.
	//! The query result will generate a QUERY_ALBUMS_RESULT event
    void ResumeQueryAlbums(int iStartPosition,
		int iItemsRequested,
		int iViewID,
		int iQueryID);

    //! Searches the remote library for genres that have tracks with the specified artist, and album.
	//! The query result will generate an QUERY_GENRES_RESULT event.

    CGeneralQueryResult* InitialQueryGenres(int iArtistKey = CMK_ALL,
        int iAlbumKey = CMK_ALL,
		int iItemsRequested = 0);

	//! Searches the remote library for genres that have tracks with the specified artist, and album.
	//! The query result will generate a QUERY_GENRES_RESULT event
    void ResumeQueryGenres(int iStartPosition,
		int iItemsRequested,
		int iViewID,
		int iQueryID);

    void RequeryGeneral(int iQueryID, char* szRequest, Upnp_FunPtr pCallback);

	//! Searches the remote media library for any radio stations it knows about.
	//! The query result will generate a  QUERY_RADIO_STATIONS_RESULT event
	//! The keys returned can be used as mediaKeys to get details of the radio stations with the
	//!    InitialQueryLibrary function.
	CRadioStationQueryResult* InitialQueryRadioStations(int iItemsRequested=0);

	void ResumeQueryRadioStations(int iStartPosition,
		int iItemsRequested,
		int iViewID,
		int iQueryID);

    void RequeryRadioStations(int iQueryID, char* szRequest);

	//! Searches the remote media library for playlists that it knows about.
	//! The query result will generate a QUERY_PLAYLISTS_RESULT event
	CGeneralQueryResult* InitialQueryPlaylists(int playlistKey = CMK_ALL,
		int iItemsRequested= 0);

	void ResumeQueryPlaylists(int iStartPosition, 
		int iItemsRequested, 
		int iViewID,
		int iQueryID);


    //! After all use has been extracted from a query, call this function to free the memory.
    //! Top-level cached queries will not be freed until the iML is deleted.
    //! Returns true if the query was deleted, false otherwise.
    bool ReleaseQuery(CQueryResult* pQueryResult);

protected:

    int     m_iDeviceNumber;    // The device number of the iML used by the UPnP code.
    char*   m_szMediaBaseURL;   // Base URL of the iML.
    TCHAR*  m_szFriendlyName;   // Friendly name of the device.
    char*   m_szUDN;            // Unique Device Identifier.
    int     m_iIMLID;           // The instance ID assigned by the iML manager.
    ECachedStatus   m_eCacheStatus; // Current state of top-level query result caching.
    bool    m_bShuttingDown;    // True when the class is being destroyed..

    // The following query results are used to cache top level queries to speed up menu navigation.
    CGeneralQueryResult*        m_pArtists;
    CGeneralQueryResult*        m_pAlbums;
    CGeneralQueryResult*        m_pGenres;
    CGeneralQueryResult*        m_pPlaylists;
    CMediaQueryResult*          m_pTracks;
    CRadioStationQueryResult*   m_pStations;

    friend class CIMLManager;

    //! Called by the iML manager to assign an ID to this iML.
    void SetInstanceID(int iIMLID)
        { m_iIMLID = iIMLID; }

public:

	// callbacks for processing the responses from the queries that are initiated
	// these all run on a upnp worker thread and all post events

	static int QueryArtistKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryAlbumKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryGenreKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryArtistByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryAlbumByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryGenreByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int QueryLibraryCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int ReleaseQueryViewCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int QueryArtistsCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int QueryAlbumsCallback(Upnp_EventType EventType, void *Event,void *Cookie);
    static int QueryGenresCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int QueryStationsCallback(Upnp_EventType EventType, void *Event,void *Cookie);
	static int QueryPlaylistsCallback(Upnp_EventType EventType, void *Event,void *Cookie);

	// helper functions

private:

    static void QueryInfoProcessCallback(Upnp_EventType EventType, void *Event, void *Cookie, char *szContext, 
        char * szKeyTag, char * szValueTag, int iEventCode, Upnp_FunPtr pCallback);

    // Callback functions used in caching query results.
    static bool QueryCacheFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);
    void QueryCacheFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount);

    // Internal functions for querying that bypass the cached query result check.
    CGeneralQueryResult* DoInitialQueryArtists(int iAlbumKey, int iGenreKey, int iItemsRequested);
    CGeneralQueryResult* DoInitialQueryAlbums(int iArtistKey, int iGenreKey, int iItemsRequested);
    CGeneralQueryResult* DoInitialQueryGenres(int iArtistKey, int iAlbumKey, int iItemsRequested);
    void DoInitialQueryGeneral(CGeneralQueryResult* pQR, int iArtistKey, int iAlbumKey, int iGenreKey, const char* szReturnContext, int iItemsRequested, const char* szSortOrder, Upnp_FunPtr pCallbackFunction);
    CGeneralQueryResult* DoInitialQueryPlaylists(int playlistKey, int iItemsRequested);
    CMediaQueryResult* DoInitialQueryLibrary(int iMediaKey, int iArtistKey, int iAlbumKey, int iGenreKey,
        int iPlaylistKey, char* szMediaType, int iInitialItemsRequested, char* szSortOrder);
    CRadioStationQueryResult* DoInitialQueryRadioStations(int iItemsRequested);

	void DoViewItems(int iStartPosition, int iItemsRequested, int iViewID, Upnp_FunPtr pCallbackFunction, int iQueryID);

};



typedef void (CIML::*PFNResumeQuery)(int iStartPosition, int iItemsRequested, int iViewID, int iQueryID);



#endif	// IML_H_

