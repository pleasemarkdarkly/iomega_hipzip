//
// File Name: IML.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/iml/iml/IML.h>
#include <main/iml/query/QueryResult.h>
#include <main/iml/query/QueryResultManager.h>

#include <stdlib.h>
#include <main/djupnp/djupnp.h>
#include <main/iml/manager/IMLManager.h>
#include <main/djupnp/UPnPEvents.h>
#include <main/djupnp/sample_util.h>

#include <util/upnp/api/upnp.h>
#include <util/upnp/genlib/encode.h>

#include <codec/codecmanager/codecmanager.h>

#include <util/debug/debug.h>


// macros for getting the device number and query number from the callback cookie
#define GET_COOKIE_DEVICE_NUMBER(cookie)            (int)((int)(cookie)&0x0000FFFF)
#define GET_COOKIE_QUERY_ID(cookie)                 (int)(((int)(cookie)&0xFFFF0000)>>16)
#define MAKE_QUERY_COOKIE(device_number, query_id)  (void*)((device_number)+((query_id)<<16))


DEBUG_MODULE_S(DJIML, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(DJIML);

static int s_iMP3CodecID = 0;   // Hacky way of storing codec IDs for use in media item parsing.
static int s_iWMACodecID = 0;
static int s_iWAVCodecID = 0;

CIML::CIML(int iDeviceNumber, const char* szMediaBaseURL, const char* szFriendlyName, const char* szUDN)
    : m_iDeviceNumber(iDeviceNumber),
    m_eCacheStatus(NOT_CACHED),
    m_bShuttingDown(false),
    m_pArtists(0),
    m_pAlbums(0),
    m_pGenres(0),
    m_pPlaylists(0),
    m_pTracks(0),
    m_pStations(0)
{
    // Get the codec IDs if they haven't yet been initialized.
    if (!s_iMP3CodecID)
    {
        s_iMP3CodecID = CCodecManager::GetInstance()->FindCodecID("mp3");
        s_iWMACodecID = CCodecManager::GetInstance()->FindCodecID("wma");
        s_iWAVCodecID = CCodecManager::GetInstance()->FindCodecID("wav");
    }

    m_szMediaBaseURL = new char[strlen(szMediaBaseURL) + strlen(szUDN) + 1];
    strcpy(m_szMediaBaseURL, szMediaBaseURL);

    // If the UDN isn't in the format we like, then just leave it blank.
    // The event handler will check it out and remove the IML if it's missing.
    m_szUDN = 0;
    const char* pchBegin = strchr(szUDN, '_');
    if (pchBegin)
    {
        const char* pchEnd = strchr(pchBegin + 1, '_');
        if (pchEnd)
        {
            strcat(m_szMediaBaseURL, "f");
            strncat(m_szMediaBaseURL, pchBegin + 1, pchEnd - pchBegin - 1);
            m_szUDN = new char[strlen(szUDN) + 1];
            strcpy(m_szUDN, szUDN);
        }
        else
            DEBUGP( DJIML, DBGLEV_WARNING, "IML: Unknown UDN format: %s\n", szUDN);
    }
    else
        DEBUGP( DJIML, DBGLEV_WARNING, "IML: Unknown UDN format: %s\n", szUDN);

    m_szFriendlyName = new TCHAR[strlen(szFriendlyName) + 1];
    CharToTchar(m_szFriendlyName, szFriendlyName);
}


CIML::~CIML()
{
    // Remove all cached queries from the manager.
    m_bShuttingDown = true;

    CQueryResultManager::GetInstance()->RemoveQueryResultsFromIML(this);

    delete [] m_szMediaBaseURL;
    delete [] m_szFriendlyName;
    delete [] m_szUDN;
}

#define INITIAL_QUERY_SIZE    20

//! Asks the iML to start caching top-level query results.
void
CIML::StartQueryCaching()
{
    m_eCacheStatus = CACHING;
    CEventQueue::GetInstance()->PutEvent(EVENT_IML_CACHING_BEGIN, (void*)m_iDeviceNumber);
    m_pArtists = DoInitialQueryArtists(CMK_ALL, CMK_ALL, INITIAL_QUERY_SIZE);
    m_pArtists->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
}

bool CIML::QueryCacheFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    ((CIML*)pUserData)->QueryCacheFunc(pQR, qs, iStartIndex, iItemCount);
    return false;
}

void CIML::QueryCacheFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount)
{
    if (qs != QUERY_SUCCESSFUL)
    {
        // Tell the system that there's been an error while caching.
        DEBUG(DJIML, DBGLEV_ERROR, "iML %d (%w): Error (%s) while caching queries\n", m_iDeviceNumber, m_szFriendlyName,
            qs == QUERY_TIMED_OUT ? "timeout" : qs == QUERY_INVALID_PARAM ? "invalid param" : "unknown");
        CEventQueue::GetInstance()->PutEvent(EVENT_IML_CACHING_ERROR, (void*)m_iDeviceNumber);
        return;
    }

    if (pQR == m_pArtists)
    {
        if ((m_pArtists->GetFilledItemCount() < m_pArtists->GetTotalItemCount()) &&
            (m_pArtists->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Artists Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pArtists->GetFilledItemCount(), m_pArtists->GetTotalItemCount());
            m_pArtists->QueryValues(m_pArtists->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pArtists->GetFilledItemCount() ? m_pArtists->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pArtists->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Artists Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pArtists->GetFilledItemCount(), m_pArtists->GetTotalItemCount());
            m_pArtists->SetNewResultsCallback(0);
            m_pAlbums = DoInitialQueryAlbums(CMK_ALL, CMK_ALL, INITIAL_QUERY_SIZE);
            m_pAlbums->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
        }
    }
    else if (pQR == m_pAlbums)
    {
        if ((m_pAlbums->GetFilledItemCount() < m_pAlbums->GetTotalItemCount()) &&
            (m_pAlbums->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Albums Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pAlbums->GetFilledItemCount(), m_pAlbums->GetTotalItemCount());
            m_pAlbums->QueryValues(m_pAlbums->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pAlbums->GetFilledItemCount() ? m_pAlbums->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pAlbums->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Albums Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pAlbums->GetFilledItemCount(), m_pAlbums->GetTotalItemCount());
            m_pAlbums->SetNewResultsCallback(0);
            m_pGenres = DoInitialQueryGenres(CMK_ALL, CMK_ALL, INITIAL_QUERY_SIZE);
            m_pGenres->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
        }
    }
    else if (pQR == m_pGenres)
    {
        if ((m_pGenres->GetFilledItemCount() < m_pGenres->GetTotalItemCount()) &&
            (m_pGenres->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Genres Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pGenres->GetFilledItemCount(), m_pGenres->GetTotalItemCount());
            m_pGenres->QueryValues(m_pGenres->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pGenres->GetFilledItemCount() ? m_pGenres->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pGenres->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Genres Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pGenres->GetFilledItemCount(), m_pGenres->GetTotalItemCount());
            m_pGenres->SetNewResultsCallback(0);
            m_pPlaylists = DoInitialQueryPlaylists(CMK_ALL, INITIAL_QUERY_SIZE);
            m_pPlaylists->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
        }
    }
    else if (pQR == m_pPlaylists)
    {
        if ((m_pPlaylists->GetFilledItemCount() < m_pPlaylists->GetTotalItemCount()) &&
            (m_pPlaylists->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Playlists Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pPlaylists->GetFilledItemCount(), m_pPlaylists->GetTotalItemCount());
            m_pPlaylists->QueryValues(m_pPlaylists->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pPlaylists->GetFilledItemCount() ? m_pPlaylists->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pPlaylists->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Playlists Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pPlaylists->GetFilledItemCount(), m_pPlaylists->GetTotalItemCount());
            m_pPlaylists->SetNewResultsCallback(0);
            m_pTracks = DoInitialQueryLibrary(CMK_ALL, CMK_ALL, CMK_ALL, CMK_ALL, CMK_ALL, NULL, INITIAL_QUERY_SIZE, "MEN");
            m_pTracks->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
        }
    }
    else if (pQR == m_pTracks)
    {
        if ((m_pTracks->GetFilledItemCount() < m_pTracks->GetTotalItemCount()) &&
            (m_pTracks->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Tracks Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pTracks->GetFilledItemCount(), m_pTracks->GetTotalItemCount());
            m_pTracks->QueryValues(m_pTracks->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pTracks->GetFilledItemCount() ? m_pTracks->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pTracks->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Tracks Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pTracks->GetFilledItemCount(), m_pTracks->GetTotalItemCount());
            m_pTracks->SetNewResultsCallback(0);
            m_pStations = DoInitialQueryRadioStations(INITIAL_QUERY_SIZE);
            m_pStations->SetNewResultsCallback(QueryCacheFuncCB, (void*)this);
        }
    }
    else if (pQR == m_pStations)
    {
        if ((m_pStations->GetFilledItemCount() < m_pStations->GetTotalItemCount()) &&
            (m_pStations->GetFilledItemCount() <= INITIAL_QUERY_SIZE))
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Stations Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pStations->GetFilledItemCount(), m_pStations->GetTotalItemCount());
            m_pStations->QueryValues(m_pStations->GetTotalItemCount() - INITIAL_QUERY_SIZE > m_pStations->GetFilledItemCount() ? m_pStations->GetTotalItemCount() - INITIAL_QUERY_SIZE : m_pStations->GetFilledItemCount(), INITIAL_QUERY_SIZE);
        }
        else
        {
            DEBUGP(DJIML, DBGLEV_INFO, "iML %d (%w): %d of %d Stations Cached\n", m_iDeviceNumber, m_szFriendlyName, m_pStations->GetFilledItemCount(), m_pStations->GetTotalItemCount());
            m_pStations->SetNewResultsCallback(0);
            // Done, tell the system that this iML is open for business
            DEBUGP(DJIML, DBGLEV_INFO, "Finished caching queries for iML %d (%w)\n", m_iDeviceNumber, m_szFriendlyName);
            CEventQueue::GetInstance()->PutEvent(EVENT_IML_CACHING_END, (void*)m_iDeviceNumber);
        }
    }
    else
    {
        // Help me JR, I'm lost
        DEBUG(DJIML, DBGLEV_ERROR, "iML %d (%w): Unknown query result to cache\n", m_iDeviceNumber, m_szFriendlyName);
        CEventQueue::GetInstance()->PutEvent(EVENT_IML_CACHING_ERROR, (void*)m_iDeviceNumber);
    }

}

/*  Query reference

ME - media context
MEN - media name
MEK - media key
MET - media type (mime type)
MEU - media URL  (i left this one out before)

GR - genre context
GRN - genre name
GRK - genre key

AL - album context
ALN - album name
ALK - album key

AR - artist context
ARN - artist name
ARK - artist key

PL - playlist context
PLN - playlist name
PLK - playlist key

RS - radiostation context
RSN - radio station name
RSK - radio station key


Queries are in this format:

  <![CDATA[%c%f]]>

  %r is the context
  %f is the optional filter.  Looks like this:
  filter            = "[" booleanExpression [operator booleanExpression] "]"
  booleanExpression = token comparator value
  comparator        = "=" | "<" | ">" | "^"
  operator          = "and"

*/


void CIML::QueryArtistKey(const TCHAR* szArtist)
{

    int ret = 0;

    // Query for an artist context with the artist name equal to szArtist
    // The artist context has 

    const char* szQueryBase = "<queryRequest><queryContext>AR</queryContext><queryExpression>ARN=%s</queryExpression><queryReturnContext>AR</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";


    char szQuery[tstrlen(szArtist)*sizeof(TCHAR) + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,szArtist);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryArtistKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }
    

}


void CIML::QueryAlbumKey(const TCHAR* szAlbum)
{

    int ret = 0;

    // Query for an artist context with the artist name equal to szArtist
    // The artist context has 

    const char* szQueryBase = "<queryRequest><queryContext>AL</queryContext><queryExpression>ALN=%s</queryExpression><queryReturnContext>AL</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";

    char szQuery[tstrlen(szAlbum)*sizeof(TCHAR) + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,szAlbum);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryAlbumKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }

    
}



void CIML::QueryGenreKey(const TCHAR* szGenre)
{
        int ret = 0;

    // Query for an artist context with the artist name equal to szArtist
    // The artist context has 

    const char* szQueryBase = "<queryRequest><queryContext>GR</queryContext><queryExpression>GRN=%s</queryExpression><queryReturnContext>GR</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";


    char szQuery[tstrlen(szGenre)*sizeof(TCHAR) + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,szGenre);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryGenreKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }

    
}


void CIML::QueryArtistByKey(int iArtistKey)
{
    int ret = 0;

    const char* szQueryBase = "<queryRequest><queryContext>AR</queryContext><queryExpression>ARK=%d</queryExpression><queryReturnContext>AR</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,iArtistKey);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryArtistByKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }

}


void CIML::QueryAlbumByKey(int iAlbumKey)
{

    int ret = 0;

    const char* szQueryBase = "<queryRequest><queryContext>AL</queryContext><queryExpression>ALK=%d</queryExpression><queryReturnContext>AL</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,iAlbumKey);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryAlbumByKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }

}

void CIML::QueryGenreByKey(int iGenreKey)
{
    
    int ret = 0;

    const char* szQueryBase = "<queryRequest><queryContext>GR</queryContext><queryExpression>GRK=%d</queryExpression><queryReturnContext>GR</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>false</returnView><count>1</count></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,iGenreKey);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {

        char* param_name = "Request";
        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryGenreByKeyCallback, (void*)m_iDeviceNumber);

        delete [] szEncodedQuery;
    }

}

CMediaQueryResult* CIML::InitialQueryLibrary(int iMediaKey,
        int iArtistKey,
        int iAlbumKey,
        int iGenreKey,
        int iPlaylistKey,       
        char* szMediaType,  
        int iInitialItemsRequested,
        char* szSortOrder)
{
    // If asking for all genres, then return the cached query result.
    if ((iMediaKey == CMK_ALL) && (iArtistKey == CMK_ALL) && (iAlbumKey == CMK_ALL) && (iGenreKey == CMK_ALL)
        && (iPlaylistKey == CMK_ALL) && (szMediaType == NULL) && szSortOrder && !strcmp(szSortOrder, "MEN"))
    {
//        return m_pTracks->Copy();
        return m_pTracks;
    }
    else
    {
        return DoInitialQueryLibrary(iMediaKey, iArtistKey, iAlbumKey, iGenreKey, iPlaylistKey, szMediaType, iInitialItemsRequested, szSortOrder);
    }
}

CMediaQueryResult* CIML::DoInitialQueryLibrary(int iMediaKey,
        int iArtistKey,
        int iAlbumKey,
        int iGenreKey,
        int iPlaylistKey,       
        char* szMediaType,  
        int iInitialItemsRequested,
        char* szSortOrder)
{
    CMediaQueryResult* pQR = CQueryResultManager::GetInstance()->CreateMediaQueryResult(this);

    char * sMediaType = szMediaType;
    char * sSortOrder = szSortOrder;

    if (sMediaType == NULL) sMediaType = "CMK_ALL";
    if (sSortOrder == NULL) sSortOrder = "";

    const char* szQueryBase = "<queryRequest><queryContext>ME</queryContext><queryExpression>MEK=%d and ARK=%d and ALK=%d and GRK=%d and PLK=%d and MET=%s</queryExpression><queryReturnContext>ME</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>true</returnView><count>%d</count><sortOrder>%s</sortOrder></viewRequest></queryRequest>";
    const char* szQueryPlaylistBase = "<queryRequest><queryContext>ME</queryContext><queryExpression>MEK=%d and ARK=%d and ALK=%d and GRK=%d and PLK=%d and MET=%s</queryExpression><queryReturnContext>ME</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>true</returnView><count>%d</count><sortOrder>PLO=%d</sortOrder></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH*5 + IML_MAX_SORT_LENGTH + IML_MAX_MIMETYPE_LENGTH + strlen(szQueryPlaylistBase) + 1];

    if (iPlaylistKey)
        sprintf(szQuery,szQueryPlaylistBase,iMediaKey,iArtistKey,iAlbumKey,iGenreKey,iPlaylistKey,sMediaType,iInitialItemsRequested,iPlaylistKey);
    else
        sprintf(szQuery,szQueryBase,iMediaKey,iArtistKey,iAlbumKey,iGenreKey,iPlaylistKey,sMediaType,iInitialItemsRequested,sSortOrder);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {
        pQR->SetInitialRequest(szEncodedQuery);

        char* param_name = "Request";
        DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryLibraryCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, pQR->GetQueryResultID()));
    }

    return pQR;
}

void CIML::DoViewItems(int iStartPosition, int iItemsRequested, int iViewID, Upnp_FunPtr pCallbackFunction, int iQueryID)
{
    int ret = 0;
    char *param_name[3];
    char *param_val[3];

    char* param_name_a = "viewID";
    char param_val_a[8];
    sprintf(param_val_a, "%d", iViewID);
    param_name[0] = param_name_a;
    param_val[0] = param_val_a;

    char* param_name_b = "startIndex";
    char param_val_b[8];
    sprintf(param_val_b, "%d", iStartPosition);
    param_name[1] = param_name_b;
    param_val[1] = param_val_b;

    char* param_name_c = "count";
    char param_val_c[8];
    sprintf(param_val_c, "%d", iItemsRequested);
    param_name[2] = param_name_c;
    param_val[2] = param_val_c;

    ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "ViewItems", param_name, param_val, 3, pCallbackFunction, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
}





void CIML::ResumeQueryLibrary(int iStartPosition,
        int iItemsRequested,
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryLibraryCallback, iQueryID);
}



void CIML::ReleaseQueryView(int iViewID)
{
    if (!m_bShuttingDown)
    {
        int ret = 0;

        char* param_name = "ViewID";
        char param_val_a[8];
        sprintf(param_val_a, "%d", iViewID);
        char* param_val = param_val_a;

        ret = DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "ReleaseView", &param_name, &param_val, 1, ReleaseQueryViewCallback, (void*)(m_iDeviceNumber));
    }
}

CGeneralQueryResult* CIML::InitialQueryArtists(int iAlbumKey = CMK_ALL,
        int iGenreKey = CMK_ALL,
        int iItemsRequested = 0)
{
    // If asking for all artists, then return the cached query result.
    if ((iAlbumKey == CMK_ALL) && (iGenreKey == CMK_ALL))
        return m_pArtists;
    else
        return DoInitialQueryArtists(iAlbumKey, iGenreKey, iItemsRequested);
}

CGeneralQueryResult* CIML::DoInitialQueryArtists(int iAlbumKey, int iGenreKey, int iItemsRequested)
{
    CGeneralQueryResult *pQR = CQueryResultManager::GetInstance()->CreateGeneralQueryResult(this, &CIML::ResumeQueryArtists);
    DoInitialQueryGeneral(pQR, CMK_ALL, iAlbumKey, iGenreKey, "AR", iItemsRequested, "ARN", QueryArtistsCallback);
    return pQR;
}

void CIML::ResumeQueryArtists(int iStartPosition,
        int iItemsRequested,
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryArtistsCallback,iQueryID);
}



CGeneralQueryResult* CIML::InitialQueryAlbums(int iArtistKey,
        int iGenreKey,
        int iItemsRequested)
{
    // If asking for all albums, then return the cached query result.
    if ((iArtistKey == CMK_ALL) && (iGenreKey == CMK_ALL))
        return m_pAlbums;
    else
        return DoInitialQueryAlbums(iArtistKey, iGenreKey, iItemsRequested);
}

CGeneralQueryResult* CIML::DoInitialQueryAlbums(int iArtistKey, int iGenreKey, int iItemsRequested)
{
    CGeneralQueryResult *pQR = CQueryResultManager::GetInstance()->CreateGeneralQueryResult(this, &CIML::ResumeQueryAlbums);
    DoInitialQueryGeneral(pQR, iArtistKey, CMK_ALL, iGenreKey, "AL", iItemsRequested, "ALN", QueryAlbumsCallback);
    return pQR;
}


void CIML::ResumeQueryAlbums(int iStartPosition,
        int iItemsRequested,    
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryAlbumsCallback, iQueryID);
}

CGeneralQueryResult* CIML::InitialQueryGenres(int iArtistKey,
        int iAlbumKey,
        int iItemsRequested)
{
    // If asking for all genres, then return the cached query result.
    if ((iArtistKey == CMK_ALL) && (iAlbumKey == CMK_ALL))
        return m_pGenres;
    else
        DoInitialQueryGenres(iArtistKey, iAlbumKey, iItemsRequested);
}

CGeneralQueryResult* CIML::DoInitialQueryGenres(int iArtistKey, int iAlbumKey, int iItemsRequested)
{
    CGeneralQueryResult *pQR = CQueryResultManager::GetInstance()->CreateGeneralQueryResult(this, &CIML::ResumeQueryGenres);
    DoInitialQueryGeneral(pQR, iArtistKey, iAlbumKey, CMK_ALL, "GR", iItemsRequested, "GRN", QueryGenresCallback);
    return pQR;
}

void CIML::ResumeQueryGenres(int iStartPosition,
        int iItemsRequested,
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryGenresCallback, iQueryID);
}

void CIML::DoInitialQueryGeneral(CGeneralQueryResult* pQR, int iArtistKey, int iAlbumKey, int iGenreKey, const char* szReturnContext, int iItemsRequested, const char* szSortOrder, Upnp_FunPtr pCallbackFunction)
{
    const char* szQueryBase = "<queryRequest><queryContext>ME</queryContext><queryExpression>ARK=%d and ALK=%d and GRK=%d and PLK=CMK_ALL</queryExpression><queryReturnContext>%s</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>true</returnView><count>%d</count><sortOrder>%s</sortOrder></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH*5 + IML_MAX_SORT_LENGTH + strlen(szQueryBase) + 1];

    sprintf(szQuery, szQueryBase, iArtistKey, iAlbumKey, iGenreKey, szReturnContext, iItemsRequested, szSortOrder);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {
        pQR->SetInitialRequest(szEncodedQuery);

        char* param_name = "Request";
        DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, pCallbackFunction, MAKE_QUERY_COOKIE(m_iDeviceNumber, pQR->GetQueryResultID()));
    }
}

void CIML::RequeryGeneral(int iQueryID, char* szRequest, Upnp_FunPtr pCallback)
{
    char* param_name = "Request";
    DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szRequest, 1, pCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
}

CRadioStationQueryResult* CIML::InitialQueryRadioStations(int iItemsRequested)
{
    return m_pStations;
}

CRadioStationQueryResult* CIML::DoInitialQueryRadioStations(int iItemsRequested)
{
    CRadioStationQueryResult *pQR = CQueryResultManager::GetInstance()->CreateRadioStationQueryResult(this);
    int iQueryID = pQR->GetQueryResultID();

    const char* szQueryBase = "<queryRequest><queryContext>RS</queryContext><queryExpression>RSN=CMK_ALL</queryExpression><queryReturnContext>RS</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>true</returnView><count>%d</count><sortOrder>RSN</sortOrder></viewRequest></queryRequest>";

    char szQuery[IML_MAX_KEY_LENGTH*2 + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,iItemsRequested);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {
        pQR->SetInitialRequest(szEncodedQuery);

        char* param_name = "Request";
        DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryStationsCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
    }

    return pQR;
}

void CIML::ResumeQueryRadioStations(int iStartPosition,
        int iItemsRequested,
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryStationsCallback, iQueryID);
}

void CIML::RequeryRadioStations(int iQueryID, char* szRequest)
{
    char* param_name = "Request";
    DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szRequest, 1, QueryStationsCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
}

CGeneralQueryResult* CIML::InitialQueryPlaylists(int playlistKey = CMK_ALL,
        int iItemsRequested)
{
    // If asking for all playlists, then return the cached query result.
    if (playlistKey == CMK_ALL)
        return m_pPlaylists;
    else
        return DoInitialQueryPlaylists(playlistKey, iItemsRequested);

}

CGeneralQueryResult* CIML::DoInitialQueryPlaylists(int playlistKey, int iItemsRequested)
{
    CGeneralQueryResult *pQR = CQueryResultManager::GetInstance()->CreateGeneralQueryResult(this, &CIML::ResumeQueryPlaylists);
    int iQueryID = pQR->GetQueryResultID();


    const char* szQueryBase = "<queryRequest><queryContext>PL</queryContext><queryExpression>PLK=%d</queryExpression><queryReturnContext>PL</queryReturnContext>"
                        "<viewRequest><returnCount>true</returnCount><returnView>true</returnView><count>%d</count><sortOrder>PLN</sortOrder></viewRequest></queryRequest>";
    char szQuery[IML_MAX_KEY_LENGTH*2 + strlen(szQueryBase) + 1];

    sprintf(szQuery,szQueryBase,playlistKey,iItemsRequested);

    char* szEncodedQuery;
    if (HrBlobTo64Sz((unsigned char*)szQuery, strlen(szQuery) + 1, &szEncodedQuery))
    {
        pQR->SetInitialRequest(szEncodedQuery);

        char* param_name = "Request";
        DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szEncodedQuery, 1, QueryPlaylistsCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
    }

    return pQR;
}

void CIML::ResumeQueryPlaylists(int iStartPosition, 
        int iItemsRequested, 
        int iViewID,
        int iQueryID)
{
    DoViewItems(iStartPosition, iItemsRequested, iViewID, QueryPlaylistsCallback, iQueryID);
}

void CIML::RequeryMedia(int iQueryID, char* szRequest)
{
    char* param_name = "Request";
    DJUPNPSendAction(IML_SERVICE, m_iDeviceNumber, "Query", &param_name, &szRequest, 1, QueryLibraryCallback, MAKE_QUERY_COOKIE(m_iDeviceNumber, iQueryID));
}

bool CIML::ReleaseQuery(CQueryResult* pQueryResult)
{
    DBASSERT(DJIML, pQueryResult->GetIML() == this, "Releasing a query from a different parent iML");
    if (m_bShuttingDown ||
        ((pQueryResult != m_pArtists) &&
        (pQueryResult != m_pAlbums) &&
        (pQueryResult != m_pGenres) &&
        (pQueryResult != m_pPlaylists) &&
        (pQueryResult != m_pTracks) &&
        (pQueryResult != m_pStations)))
    {
        delete pQueryResult;
        return true;
    }
    else
        return false;
}


// ####################################################################################################################
//                      Callbacks into CIML when query results come in
// ####################################################################################################################

int CIML::QueryArtistKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    // we apprently have the results from our query.  Let's check it out.
    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;

            char* szKey = NULL;
            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "AR");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node artistNode = UpnpNodeList_item(NodeList, i);
                        if (artistNode)
                        {
                            szKey = SampleUtil_GetFirstElementItem(artistNode, "ARK");
                            if (szKey)
                            {
                                char* szArtistName = SampleUtil_GetFirstElementItem(artistNode, "ARN");
                                if (szArtistName)
                                {
                                    bValidResponse = true;
                                    Upnpfree(szArtistName);
                                }
                            }
                        }
                        UpnpNode_free(artistNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_artist_key_info_t *pResponseInfo = new iml_query_artist_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szKey, "Missing key value");
                pResponseInfo->iArtistKey = atoi(szKey);
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_ARTIST_KEY_RESULT, (void*)pResponseInfo);

            if (szKey) Upnpfree(szKey);
    }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;
}



int CIML::QueryAlbumKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
        SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;

            char* szKey = NULL;
            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "AL");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node albumNode = UpnpNodeList_item(NodeList, i);
                        if (albumNode)
                        {
                            szKey = SampleUtil_GetFirstElementItem(albumNode, "ALK");
                            if (szKey)
                            {
                                char* szAlbumName = SampleUtil_GetFirstElementItem(albumNode, "ALN");
                                if (szAlbumName)
                                {
                                    bValidResponse = true;
                                    Upnpfree(szAlbumName);
                                }
                            }
                        }
                        UpnpNode_free(albumNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_album_key_info_t *pResponseInfo = new iml_query_album_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szKey, "Missing key value");
                pResponseInfo->iAlbumKey = atoi(szKey);
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_ALBUM_KEY_RESULT, (void*)pResponseInfo);

            if (szKey) Upnpfree(szKey);

        }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;
}





int CIML::QueryGenreKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{

    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;

            char* szKey = NULL;
            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "GR");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node genreNode = UpnpNodeList_item(NodeList, i);
                        if (genreNode)
                        {
                            szKey = SampleUtil_GetFirstElementItem(genreNode, "GRK");
                            if (szKey)
                            {
                                char* szGenreName = SampleUtil_GetFirstElementItem(genreNode, "GRN");
                                if (szGenreName)
                                {
                                    bValidResponse = true;
                                    Upnpfree(szGenreName);
                                }
                            }
                        }
                        UpnpNode_free(genreNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_genre_key_info_t *pResponseInfo = new iml_query_genre_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szKey, "Missing key value");
                pResponseInfo->iGenreKey = atoi(szKey);
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_GENRE_KEY_RESULT, (void*)pResponseInfo);

            if (szKey) Upnpfree(szKey);

    }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;

}




int CIML::QueryArtistByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    // we apprently have the results from our query.  Let's check it out.
    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;
            char* szArtistName = NULL;


            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "AR");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node artistNode = UpnpNodeList_item(NodeList, i);
                        if (artistNode)
                        {
                            char* szKey = SampleUtil_GetFirstElementItem(artistNode, "ARK");
                            if (szKey)
                            {
                                szArtistName = SampleUtil_GetFirstElementItem(artistNode, "ARN");
                                if (szArtistName)
                                {
                                    bValidResponse = true;
                                
                                }
                                Upnpfree(szKey);
                            }
                        }
                        UpnpNode_free(artistNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_artist_by_key_info_t *pResponseInfo = new iml_query_artist_by_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szArtistName , "Missing artist name");
                pResponseInfo->szArtistName = new TCHAR[strlen(szArtistName) + 1];
                CharToTchar(const_cast<TCHAR*>(pResponseInfo->szArtistName), szArtistName);
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_ARTIST_BY_KEY_RESULT, (void*)pResponseInfo);

            if (szArtistName) Upnpfree(szArtistName);

    }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;




}

int CIML::QueryAlbumByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;
            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;
            char* szAlbumName = NULL;

            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "AL");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node albumNode = UpnpNodeList_item(NodeList, i);
                        if (albumNode)
                        {
                            char* szKey = SampleUtil_GetFirstElementItem(albumNode, "ALK");
                            if (szKey)
                            {
                                szAlbumName = SampleUtil_GetFirstElementItem(albumNode, "ALN");
                                if (szAlbumName)
                                {
                                    bValidResponse = true;
                                
                                }
                                Upnpfree(szKey);
                            }
                        }
                        UpnpNode_free(albumNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_album_by_key_info_t *pResponseInfo = new iml_query_album_by_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szAlbumName , "Missing album name");
                pResponseInfo->szAlbumName = new TCHAR[strlen(szAlbumName) + 1];
                CharToTchar(const_cast<TCHAR*>(pResponseInfo->szAlbumName), szAlbumName);
            
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_ALBUM_BY_KEY_RESULT, (void*)pResponseInfo);

            if (szAlbumName) Upnpfree(szAlbumName);
    
        }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;
}


int CIML::QueryGenreByKeyCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{

    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    switch ( EventType) {
  
        /* SOAP Stuff */
        case UPNP_CONTROL_ACTION_COMPLETE:
        {
            struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;

            if (a_event->ErrCode != UPNP_E_SUCCESS)
            {
                DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);
            }

            Upnp_Document pResult = a_event->ActionResult;
            char* szGenreName = NULL;

            if (pResult)
            {
//              DEBUG_XML_DOC(pResult);
                Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "GR");
                if (NodeList)
                {
                    int cItems = UpnpNodeList_getLength(NodeList);

                    for (int i = 0; i < cItems; ++i)
                    {
                        Upnp_Node genreNode = UpnpNodeList_item(NodeList, i);
                        if (genreNode)
                        {
                            char *szKey = SampleUtil_GetFirstElementItem(genreNode, "GRK");
                            if (szKey)
                            {
                                szGenreName = SampleUtil_GetFirstElementItem(genreNode, "GRN");
                                if (szGenreName)
                                {
                                    bValidResponse = true;
                                
                                }
                                Upnpfree(szKey);
                            }
                        }
                        UpnpNode_free(genreNode);
                    }
                    UpnpNodeList_free(NodeList);
                }
            }
            
            iml_query_genre_by_key_info_t *pResponseInfo = new iml_query_genre_by_key_info_t;

            pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;


            if (pResponseInfo->status == QUERY_SUCCESSFUL)
            {
                DBASSERT(DJIML, szGenreName , "Missing genre name");
                pResponseInfo->szGenreName = new TCHAR[strlen(szGenreName) + 1];
                CharToTchar(const_cast<TCHAR*>(pResponseInfo->szGenreName), szGenreName);
                
                pResponseInfo->szGenreName = (TCHAR*)szGenreName;
                pResponseInfo->iDeviceNumber = (int)Cookie&0x000000FF;
            }

            CEventQueue::GetInstance()->PutEvent(QUERY_GENRE_BY_KEY_RESULT, (void*)pResponseInfo);

            if (szGenreName) Upnpfree(szGenreName);


    }
        break;

        case UPNP_DISCOVERY_ADVERTISEMENT_ALIVE:
        case UPNP_DISCOVERY_SEARCH_RESULT:
        case UPNP_DISCOVERY_SEARCH_TIMEOUT:
        case UPNP_DISCOVERY_ADVERTISEMENT_BYEBYE:
        case UPNP_CONTROL_GET_VAR_COMPLETE:
        case UPNP_EVENT_RECEIVED:
        case UPNP_EVENT_SUBSCRIBE_COMPLETE:
        case UPNP_EVENT_UNSUBSCRIBE_COMPLETE:
        case UPNP_EVENT_RENEWAL_COMPLETE:
        case UPNP_EVENT_AUTORENEWAL_FAILED:
        case UPNP_EVENT_SUBSCRIPTION_EXPIRED:
        case UPNP_EVENT_SUBSCRIPTION_REQUEST:
        case UPNP_CONTROL_GET_VAR_REQUEST:
        case UPNP_CONTROL_ACTION_REQUEST:
        default:
            break;
    }
    return 0;

}



// called by CIML::QueryLibraryCallback to process the XML into mediarecordlist entries
static IMLMediaInfoVector *CreateMediaInfoVector(Upnp_Document pResult)
{
    IMLMediaInfoVector *newInfoVector = new IMLMediaInfoVector;
    char * szScratch;
    
    Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "ME");
    if (NodeList)
    {
        int cItems = UpnpNodeList_getLength(NodeList);
        for (int i = 0; i < cItems; ++i)
        {
            iml_media_info_t mk;

            Upnp_Node mediaNode = UpnpNodeList_item(NodeList, i);
            if (mediaNode)
            {
                if ( (szScratch = SampleUtil_GetFirstElementItem(mediaNode, "MEK")) )
                {
                    mk.iMediaKey = atoi(szScratch);

                    Upnpfree(szScratch);

                }

                if ( (szScratch = SampleUtil_GetFirstElementItem(mediaNode, "MEN")) )
                {
                    mk.szMediaTitle = new TCHAR[strlen(szScratch) + 1];
        
                    CharToTchar(const_cast<TCHAR*>(mk.szMediaTitle), szScratch);

                    Upnpfree(szScratch);

                }

                if ( (szScratch = SampleUtil_GetFirstElementItem(mediaNode, "MET")) )
                {
                    // This is a quick, hacky way to check codec type.
                    // A string compare would be safer, but speeding up parsing is important.
                    mk.iCodecID = (szScratch[2] == 'v') ? s_iWAVCodecID :
                        (szScratch[7] == '3') ? s_iMP3CodecID : s_iWMACodecID;
                    Upnpfree(szScratch);
                }

                if ( (szScratch = SampleUtil_GetFirstElementItem(mediaNode, "ARN")) )
                {
                    mk.szArtistName = new TCHAR[strlen(szScratch) + 1];
        
                    CharToTchar(const_cast<TCHAR*>(mk.szArtistName), szScratch);

                    Upnpfree(szScratch);

                }
            }

            UpnpNode_free(mediaNode);

            newInfoVector->PushBack(mk);
        }
        UpnpNodeList_free(NodeList);
    }
    return newInfoVector;

}



int CIML::QueryLibraryCallback(Upnp_EventType EventType, void *Event, void *Cookie)
{

    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    char* szViewID = NULL;
    char* szCount = NULL;
    char* szTotalItems = NULL;
    char* szStartIndex = NULL;
    IMLMediaInfoVector *pRecords = NULL;

    if (EventType == UPNP_CONTROL_ACTION_COMPLETE)
    {
        struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;

        if (a_event->ErrCode != UPNP_E_SUCCESS)
        {
            DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);

            // Parse the request for parameters we need to requery.
            if (a_event->ActionRequest)
            {
                szViewID     = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "viewID");
                if (szViewID)
                {
                    szStartIndex = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest,"startIndex");
                    szCount      = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "count");
                }
            }
            
            iml_query_library_info_t *pResponseInfo = new iml_query_library_info_t ;
            pResponseInfo->status                   = a_event->ErrCode == UPNP_E_INVALID_ACTION ? QUERY_INVALID_PARAM : QUERY_TIMED_OUT;
            pResponseInfo->iTotalItems              = -1;
            pResponseInfo->iStartIndex              = szStartIndex ? atoi(szStartIndex) : 0;
            pResponseInfo->iViewID                  = szViewID ? atoi(szViewID) : -1;
            pResponseInfo->iItemsReturned           = szCount ? atoi(szCount) : 0;
            pResponseInfo->pRecords                 = 0;
            pResponseInfo->iDeviceNumber            = GET_COOKIE_DEVICE_NUMBER(Cookie);
            pResponseInfo->iQueryID                 = GET_COOKIE_QUERY_ID(Cookie);

            CEventQueue::GetInstance()->PutEvent(QUERY_LIBRARY_RESULT, (void*)pResponseInfo);
        }
        else
        {
/* Uncomment this to print the request for a media query
            Upnp_Document pRequest = a_event->ActionRequest;
            if (pRequest)
            {
                char* szTheXMLDocument = UpnpNewPrintDocument(pRequest);
                if (szTheXMLDocument)
                {
                    diag_printf("***\nRequest: %s\n***\n", szTheXMLDocument);
                    free(szTheXMLDocument);
                }
            }
*/

            Upnp_Document pResult = a_event->ActionResult;
            if (pResult)
            {
/* Uncomment this to print the response to a media query 
                char* szTheXMLDocument = UpnpNewPrintDocument(pResult);
                if (szTheXMLDocument)
                {
                    diag_printf("***\nResponse: %s\n***\n", szTheXMLDocument);
                    free(szTheXMLDocument);
                }
*/

                szTotalItems = SampleUtil_GetFirstDocumentItem(pResult, "totalItems");
                pRecords     = CreateMediaInfoVector(pResult);
                szViewID     = SampleUtil_GetFirstDocumentItem(pResult, "viewID");
                szCount      = SampleUtil_GetFirstDocumentItem(pResult, "count");
                szStartIndex = SampleUtil_GetFirstDocumentItem(pResult,"startIndex");

                bValidResponse = ((szViewID)&&(szCount)&&(szTotalItems)&&(szStartIndex)&&(pRecords));
                if (bValidResponse)
                {
                
                    iml_query_library_info_t *pResponseInfo = new iml_query_library_info_t;
                    pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;
                    pResponseInfo->iStartIndex      = szStartIndex ? atoi(szStartIndex) : 0;
                    pResponseInfo->iViewID          = szViewID ? atoi(szViewID) : -1;
                    if (pResponseInfo->status == QUERY_SUCCESSFUL)
                    {
                        pResponseInfo->iTotalItems      = atoi(szTotalItems);
                        pResponseInfo->iItemsReturned   = atoi(szCount);
                        pResponseInfo->pRecords         = pRecords;
                    }
                    else
                    {
                        pResponseInfo->iTotalItems      = -1;
                        pResponseInfo->iItemsReturned   = 0;
                        pResponseInfo->pRecords         = 0;
                    }

                    pResponseInfo->iDeviceNumber    = GET_COOKIE_DEVICE_NUMBER(Cookie);
                    pResponseInfo->iQueryID         = GET_COOKIE_QUERY_ID(Cookie);
                    CEventQueue::GetInstance()->PutEvent(QUERY_LIBRARY_RESULT, (void*)pResponseInfo);
                }
            }
        }
        if (szStartIndex) Upnpfree(szStartIndex);
        if (szCount) Upnpfree(szCount);
        if (szTotalItems) Upnpfree(szTotalItems);
        if (szViewID) Upnpfree(szViewID);
    }

    return 0;
}

int CIML::ReleaseQueryViewCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    // do nothing
    return 0;
}


static ContentKeyValueVector *CreateContentKeyValueVector(Upnp_Document pResult, char * szContext, char * szKeyTag, char * szValueTag)
{
    ContentKeyValueVector *newInfoVector = new ContentKeyValueVector;
    char * szScratch;
    
    Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, szContext);
    if( newInfoVector && NodeList )
    {
        int cItems = UpnpNodeList_getLength(NodeList);
        for (int i = 0; i < cItems; ++i)
        {
            cm_key_value_record_t mk;

            Upnp_Node contextNode = UpnpNodeList_item(NodeList, i);
            if (contextNode)
            {
#if 0
                // dc overrun trace, determine if following calls induce overruns
                UpnpNode_free(contextNode);
                return newInfoVector;
#endif
                if ( (szScratch = SampleUtil_GetFirstElementItem(contextNode, szKeyTag)) )
                {
                    mk.iKey = atoi(szScratch);

                    Upnpfree(szScratch);

                }

                if ( (szScratch = SampleUtil_GetFirstElementItem(contextNode, szValueTag)) )
                {
                    mk.szValue = new TCHAR[strlen(szScratch) + 1];
        
                    CharToTchar(const_cast<TCHAR*>(mk.szValue), szScratch);

                    Upnpfree(szScratch);

                }
                UpnpNode_free(contextNode);
            }

            newInfoVector->PushBack(mk);
        }
        UpnpNodeList_free(NodeList);
    }
    return newInfoVector;

}



void CIML::QueryInfoProcessCallback(Upnp_EventType EventType, void *Event,void *Cookie,char * szContext, 
                                char * szKeyTag, char * szValueTag, int iEventCode, Upnp_FunPtr pCallback)
{
    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    char* szViewID = NULL;
    char* szCount = NULL;
    char* szTotalItems = NULL;
    char* szStartIndex = NULL;
    ContentKeyValueVector *pRecords = NULL;

    if (EventType == UPNP_CONTROL_ACTION_COMPLETE )
    {
        struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;


        if (a_event->ErrCode != UPNP_E_SUCCESS)
        {
            DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);

            // Parse the request for parameters we need to requery.
            if (a_event->ActionRequest)
            {
                szViewID     = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "viewID");
                if (szViewID)
                {
                    szStartIndex = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest,"startIndex");
                    szCount      = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "count");
                }
            }

            iml_query_info_t *pResponseInfo = new iml_query_info_t;
            pResponseInfo->status           = a_event->ErrCode == UPNP_E_INVALID_ACTION ? QUERY_INVALID_PARAM : QUERY_TIMED_OUT;
            pResponseInfo->iTotalItems      = -1;
            pResponseInfo->iStartIndex      = szStartIndex ? atoi(szStartIndex) : 0;
            pResponseInfo->iViewID          = szViewID ? atoi(szViewID) : -1;
            pResponseInfo->iItemsReturned   = szCount ? atoi(szCount) : 0;
            pResponseInfo->pKeyValues       = 0;
            pResponseInfo->iDeviceNumber    = GET_COOKIE_DEVICE_NUMBER(Cookie);
            pResponseInfo->iQueryID         = GET_COOKIE_QUERY_ID(Cookie);
            pResponseInfo->pCallback        = pCallback;

            CEventQueue::GetInstance()->PutEvent(iEventCode, (void*)pResponseInfo);
        }
        else
        {
            
            Upnp_Document pResult = a_event->ActionResult;

            if (pResult)
            {
/* Uncomment this to print the response to a media query 
                char* szTheXMLDocument = UpnpNewPrintDocument(pResult);
                if (szTheXMLDocument)
                {
                    diag_printf("***\nResponse: %s\n***\n", szTheXMLDocument);
                    free(szTheXMLDocument);
                }
*/
                pRecords     = CreateContentKeyValueVector(pResult,szContext,szKeyTag,szValueTag);
                szTotalItems = SampleUtil_GetFirstDocumentItem(pResult, "totalItems");
                szViewID     = SampleUtil_GetFirstDocumentItem(pResult, "viewID");
                szCount      = SampleUtil_GetFirstDocumentItem(pResult, "count");
                szStartIndex = SampleUtil_GetFirstDocumentItem(pResult, "startIndex");
                bValidResponse = ((szViewID)&&(szCount)&&(szTotalItems)&&(szStartIndex)&&(pRecords));

                if (bValidResponse)
                {

                    iml_query_info_t *pResponseInfo = new iml_query_info_t;

                    pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;
                    pResponseInfo->iStartIndex      = szStartIndex ? atoi(szStartIndex) : 0;
                    pResponseInfo->iViewID          = szViewID ? atoi(szViewID) : -1;
        
                    if (pResponseInfo->status == QUERY_SUCCESSFUL)
                    {
                        pResponseInfo->iTotalItems      = atoi(szTotalItems);
                        pResponseInfo->iItemsReturned   = atoi(szCount);
                        pResponseInfo->pKeyValues       = pRecords;
                    }
                    else
                    {
                        pResponseInfo->iTotalItems      = -1;
                        pResponseInfo->iItemsReturned   = 0;
                        pResponseInfo->pKeyValues       = 0;
                    }
            
                    pResponseInfo->iDeviceNumber    = GET_COOKIE_DEVICE_NUMBER(Cookie);
                    pResponseInfo->iQueryID         = GET_COOKIE_QUERY_ID(Cookie);

                    CEventQueue::GetInstance()->PutEvent(iEventCode, (void*)pResponseInfo);
                }
            }
        }

        if (szStartIndex) Upnpfree(szStartIndex);
        if (szCount) Upnpfree(szCount);
        if (szTotalItems) Upnpfree(szTotalItems);
        if (szViewID) Upnpfree(szViewID);
    }
}


int CIML::QueryArtistsCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    QueryInfoProcessCallback(EventType,Event,Cookie,"AR","ARK","ARN",QUERY_ARTISTS_RESULT,QueryArtistsCallback);
    return 0;
}

int CIML::QueryAlbumsCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    QueryInfoProcessCallback(EventType,Event,Cookie,"AL","ALK","ALN",QUERY_ALBUMS_RESULT,QueryAlbumsCallback);
    return 0;
}

int CIML::QueryGenresCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    QueryInfoProcessCallback(EventType,Event,Cookie,"GR","GRK","GRN",QUERY_GENRES_RESULT,QueryGenresCallback);
    return 0;
}


// called by CIML::QueryStationsCallback to process the XML into radio station entries
static IMLRadioStationInfoVector *CreateStationInfoVector(Upnp_Document pResult)
{
    IMLRadioStationInfoVector *newInfoVector = new IMLRadioStationInfoVector;

    Upnp_NodeList NodeList = UpnpDocument_getElementsByTagName(pResult, "RS");
    if (NodeList)
    {
        int cItems = UpnpNodeList_getLength(NodeList);
        for (int i = 0; i < cItems; ++i)
        {
            iml_radio_station_info_t rs;

            Upnp_Node stationNode = UpnpNodeList_item(NodeList, i);
            if (stationNode)
            {
                char * szScratch;
                if ( (szScratch = SampleUtil_GetFirstElementItem(stationNode, "RSN")) ) // name
                {
                    rs.szStationName = new TCHAR[strlen(szScratch) + 1];

                    CharToTchar(const_cast<TCHAR*>(rs.szStationName), szScratch);

                    Upnpfree(szScratch);
                }

                rs.szURL = SampleUtil_GetFirstElementItem(stationNode, "RSU");          // url

                if ( (szScratch = SampleUtil_GetFirstElementItem(stationNode, "MET")) ) // mimetype
                {
                    // This is a quick, hacky way to check codec type.
                    // A string compare would be safer, but speeding up parsing is important.
                    rs.iCodecID = (szScratch[2] == 'v') ? s_iWAVCodecID :
                        (szScratch[7] == '3') ? s_iMP3CodecID : s_iWMACodecID;
                    Upnpfree(szScratch);
                }
            }

            UpnpNode_free(stationNode);

            newInfoVector->PushBack(rs);
        }
        UpnpNodeList_free(NodeList);
    }

    return newInfoVector;

}

int CIML::QueryStationsCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{

    SampleUtil_PrintEvent(EventType, Event);
    bool bValidResponse = false;

    char* szViewID = NULL;
    char* szCount = NULL;
    char* szTotalItems = NULL;
    char* szStartIndex = NULL;
    IMLRadioStationInfoVector *pStations = NULL;

    if (EventType == UPNP_CONTROL_ACTION_COMPLETE)
    {
        struct Upnp_Action_Complete *a_event = (struct Upnp_Action_Complete * ) Event;

        if (a_event->ErrCode != UPNP_E_SUCCESS)
        {
            DEBUG(DJIML, DBGLEV_ERROR,"IML %d; Error in Action Complete Callback -- %d\n", GET_COOKIE_DEVICE_NUMBER(Cookie), a_event->ErrCode);

            // Parse the request for parameters we need to requery.
            if (a_event->ActionRequest)
            {
                szViewID     = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "viewID");
                if (szViewID)
                {
                    szStartIndex = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest,"startIndex");
                    szCount      = SampleUtil_GetFirstDocumentItem(a_event->ActionRequest, "count");
                }
            }

            iml_query_radio_stations_info_t *pResponseInfo = new iml_query_radio_stations_info_t ;
            pResponseInfo->status           = a_event->ErrCode == UPNP_E_INVALID_ACTION ? QUERY_INVALID_PARAM : QUERY_TIMED_OUT;
            pResponseInfo->iTotalItems      = -1;
            pResponseInfo->iStartIndex      = szStartIndex ? atoi(szStartIndex) : 0;
            pResponseInfo->iViewID          = szViewID ? atoi(szViewID) : -1;
            pResponseInfo->iItemsReturned   = szCount ? atoi(szCount) : 0;
            pResponseInfo->pStations        = 0;
            pResponseInfo->iDeviceNumber    = GET_COOKIE_DEVICE_NUMBER(Cookie);
            pResponseInfo->iQueryID         = GET_COOKIE_QUERY_ID(Cookie);

            CEventQueue::GetInstance()->PutEvent(QUERY_RADIO_STATIONS_RESULT, (void*)pResponseInfo);
        }
        else
        {
            Upnp_Document pResult = a_event->ActionResult;
            if (pResult)
            {
                szTotalItems = SampleUtil_GetFirstDocumentItem(pResult, "totalItems");
                pStations    = CreateStationInfoVector(pResult);
                szViewID     = SampleUtil_GetFirstDocumentItem(pResult, "viewID");
                szCount      = SampleUtil_GetFirstDocumentItem(pResult, "count");
                szStartIndex = SampleUtil_GetFirstDocumentItem(pResult,"startIndex");
                bValidResponse = ((szViewID)&&(szCount)&&(szTotalItems)&&(szStartIndex)&&(pStations));
                if (bValidResponse)
                {
                
                    iml_query_radio_stations_info_t *pResponseInfo = new iml_query_radio_stations_info_t;
                    pResponseInfo->status = ((a_event->ErrCode == UPNP_E_SUCCESS) && (bValidResponse)) ? QUERY_SUCCESSFUL : QUERY_INVALID_PARAM;
                    pResponseInfo->iStartIndex      = szStartIndex ? atoi(szStartIndex) : 0;
                    pResponseInfo->iViewID          = szViewID ? atoi(szViewID) : -1;
                    if (pResponseInfo->status == QUERY_SUCCESSFUL)
                    {
                        pResponseInfo->iTotalItems      = atoi(szTotalItems);
                        pResponseInfo->iItemsReturned   = atoi(szCount);
                        pResponseInfo->pStations        = pStations;
                    }
                    else
                    {
                        pResponseInfo->iTotalItems      = -1;
                        pResponseInfo->iItemsReturned   = 0;
                        pResponseInfo->pStations        = 0;
                    }

                    pResponseInfo->iDeviceNumber    = GET_COOKIE_DEVICE_NUMBER(Cookie);
                    pResponseInfo->iQueryID         = GET_COOKIE_QUERY_ID(Cookie);
                    CEventQueue::GetInstance()->PutEvent(QUERY_RADIO_STATIONS_RESULT, (void*)pResponseInfo);
                }
            }
        }
        if (szStartIndex) Upnpfree(szStartIndex);
        if (szCount) Upnpfree(szCount);
        if (szTotalItems) Upnpfree(szTotalItems);
        if (szViewID) Upnpfree(szViewID);
    }
    return 0;
}


int CIML::QueryPlaylistsCallback(Upnp_EventType EventType, void *Event,void *Cookie)
{
    QueryInfoProcessCallback(EventType,Event,Cookie,"PL","PLK","PLN",QUERY_PLAYLISTS_RESULT,QueryPlaylistsCallback);
    return 0;
}




