//
// UPnPEvents.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! The following is a list of events sent by the UPnP code to the UI for the dj project.
//! Descriptions of the data parameter filled in by the GetEvent call and
//! the behavior of the default event handler are included.
/** \addtogroup Events Events */
//@{

#ifndef UPNPEVENTS_H_
#define UPNPEVENTS_H_

#include <util/upnp/api/upnp.h>
#include <util/tchar/tchar.h>
#include <content/common/QueryableContentManager.h>

//! Sent from the UPnP thread when a new iML is found.
//! The data parameter is a iml_found_info_t struct.
//! The szFriendlyName and szMediaBaseURL fields of the iml_found_info_t struct should be free()'d
//! by the event handler, and the struct itself should be deleted.
#define EVENT_IML_FOUND         0x100

//! Sent when an iML starts caching its top level query results.
//! The data parameter is the device number of the iML.
#define EVENT_IML_CACHING_BEGIN 0x10D

//! Sent when an iML has finished caching its top level query results.
//! The data parameter is the device number of the iML.
#define EVENT_IML_CACHING_END   0x10E

//! Sent when an error occurs while an iML is caching its top level query results.
//! The data parameter is the device number of the iML.
#define EVENT_IML_CACHING_ERROR 0x10F

//! Sent from the UPnP thread when an iML goes offline.
//! The data parameter is the device number of the iML.
#define EVENT_IML_BYEBYE        0x101

//! Sent by the main thread to delete the iML after the UI is done with it.
//! The data parameter is the pointer to the CIML object to be deleted.
#define EVENT_IML_DELETE_ME     0x110


typedef struct iml_found_info_s
{
    int     iDeviceNumber;  //!< Device number the UPnP code uses to refer to the iML.
    char*   szMediaBaseURL; //!< Base URL of the iML, used to stream media.
    char*   szFriendlyName; //!< Friendly name of the device.
    char*   szUDN;          //!< Unique Device Name
} iml_found_info_t;

typedef enum
{
    QUERY_SUCCESSFUL = 0,   //!< The query was successful
    QUERY_TIMED_OUT,        //!< The query timed out
    QUERY_INVALID_PARAM     //!< Invalid parameters in the query request
} query_status_t;

//! Sent from the UPnP thread when the query artist key query completed.
//! The data parameter is a iml_query_artist_key_info_t struct.
//! and the struct itself should be deleted.
#define QUERY_ARTIST_KEY_RESULT         0x101

typedef struct iml_query_artist_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iArtistKey;         //!< The artist key that matches the artist string passed in, or 0 if no matching artist was found.
} iml_query_artist_key_info_t;



//! Sent from the UPnP thread when the query album key query completed.
//! The data parameter is a iml_query_album_key_info_t struct.
//! and the struct itself should be deleted.
#define QUERY_ALBUM_KEY_RESULT          0x102
typedef struct iml_query_album_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iAlbumKey;          //!< The artist key that matches the album string passed in, or 0 if no matching album was found.
} iml_query_album_key_info_t;


//! Sent from the UPnP thread when the query genre key query completed.
//! The data parameter is a iml_query_genre_key_info_t struct.
//! and the struct itself should be deleted.
#define QUERY_GENRE_KEY_RESULT          0x103

typedef struct iml_query_genre_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iGenreKey;          //!< The artist key that matches the genre string passed in, or 0 if no matching genre was found.
} iml_query_genre_key_info_t;


//! Sent from the UPnP thread when the query artist by key query completed.
//! The data parameter is a iml_query_artist_by_key_info_t struct.
//! The szArtistName field of the iml_query_artist_by_key_info_t struct should be delete[]'d by the event
//! and the struct itself should be deleted.
#define QUERY_ARTIST_BY_KEY_RESULT      0x104

typedef struct iml_query_artist_by_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    TCHAR * szArtistName;   //!< The name of the artist matching the key
} iml_query_artist_by_key_info_t;


//! Sent from the UPnP thread when the query album by key query completed.
//! The data parameter is a iml_query_album_by_key_info_t struct.
//! The szAlbumName field of the iml_query_album_by_key_info_t struct should be delete[]'d by the event
//! and the struct itself should be deleted.
#define QUERY_ALBUM_BY_KEY_RESULT       0x105

typedef struct iml_query_album_by_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    TCHAR * szAlbumName;    //!< The name of the album matching the key
} iml_query_album_by_key_info_t;



//! Sent from the UPnP thread when the query genre by key query completed.
//! The data parameter is a iml_query_genre_by_key_info_t struct.
//! The szGenreName field of the iml_query_genre_by_key_info_t struct should be delete[]'d by the event handler
//! and the struct itself should be deleted.
#define QUERY_GENRE_BY_KEY_RESULT       0x106

typedef struct iml_query_genre_by_key_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    TCHAR * szGenreName;    //!< The name of the genre matching the key
} iml_query_genre_by_key_info_t;
    


//! Sent from the UPnP thread when the QueryLibrary query completes.
//! The data parameter is a iml_query_library_info_t struct.
//! The MediaRecordList should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_LIBRARY_RESULT            0x107

typedef struct iml_media_info_s
{
    int     iMediaKey;
    TCHAR*  szMediaTitle;
    TCHAR*  szArtistName;
    int     iCodecID;
} iml_media_info_t;

typedef SimpleVector<iml_media_info_t> IMLMediaInfoVector;

typedef struct iml_query_library_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iStartIndex;        //!< The index of the first record returned
    int iTotalItems;        //!< The total number of items on the media library
    int iItemsReturned;     //!< The number of media records returned
    int iViewID;            //!< The view ID associated with this query
    int iQueryID;           //!< The ID associated with this query
    IMLMediaInfoVector  *pRecords;  //!<
} iml_query_library_info_t;



typedef struct iml_query_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iStartIndex;        //!< The index of the first record returned
    int iTotalItems;        //!< The total number of items on the media library
    int iItemsReturned;     //!< The number of records returned
    int iViewID;            //!< The view ID associated with this query
    int iQueryID;           //!< The ID associated with this query
    Upnp_FunPtr pCallback;  //!< The callback function to use if we need to requery
    ContentKeyValueVector* pKeyValues;
} iml_query_info_t;


//! Sent from the UPnP thread when the QueryArtists query completes.
//! The data parameter is a iml_query_info_t struct.
//! The ContentKeyValueVector should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_ARTISTS_RESULT            0x108

//! Sent from the UPnP thread when the QueryArtists query completes.
//! The data parameter is a iml_query_info_t struct.
//! The ContentKeyValueVector should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_ALBUMS_RESULT             0x109

//! Sent from the UPnP thread when the QueryGenres query completes.
//! The data parameter is a iml_query_info_t struct.
//! The ContentKeyValueVector should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_GENRES_RESULT             0x10A

//! Sent from the UPnP thread when the QueryRadioStations query completes.
//! The data parameter is a iml_query_radio_stations_info_t struct.
//! The ContentKeyValueVector should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_RADIO_STATIONS_RESULT     0x10B

typedef struct iml_radio_station_info_s
{
    TCHAR*  szStationName;
    char*   szURL;
    int     iCodecID;
} iml_radio_station_info_t;

typedef SimpleVector<iml_radio_station_info_t> IMLRadioStationInfoVector;

typedef struct iml_query_radio_stations_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iStartIndex;        //!< The index of the first record returned
    int iTotalItems;        //!< The total number of items on the media library
    int iItemsReturned;     //!< The number of records returned
    int iViewID;            //!< The view ID associated with this query
    int iQueryID;           //!< The ID associated with this query
    IMLRadioStationInfoVector* pStations;   //!< Array of radio station info.
} iml_query_radio_stations_info_t;


//! Sent from the UPnP thread when the QueryPlaylists query completes.
//! The data parameter is a iml_query_playlists_info_t struct.
//! The ContentKeyValueVector should be deleted when processed
//! The struct itself should be deleted.
#define QUERY_PLAYLISTS_RESULT          0x10C

typedef struct iml_query_playlists_info_s
{
    query_status_t status;  //!< Status of the query
    int iDeviceNumber;      //!< Device number the UPnP code uses to refer to the iML.
    int iStartIndex;        //!< The index of the first record returned
    int iTotalItems;        //!< The total number of matching items on the media library
    int iItemsReturned;     //!< The number of records returned
    Upnp_FunPtr pCallback;  //!< The callback function to use if we need to requery
    ContentKeyValueVector* pKeyValues;  //!<Key is playlist key, value is the name of the playlist.
} iml_query_playlists_info_t;



//@}

#endif  // UPNPEVENTS_H_
