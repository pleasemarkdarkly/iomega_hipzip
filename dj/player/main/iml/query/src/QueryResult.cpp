//
// File Name: QueryResult.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/iml/query/QueryResult.h>
#include <main/djupnp/UPnPEvents.h>
#include <main/iml/iml/IML.h>
#include <util/debug/debug.h>

#include <stdlib.h>

DEBUG_MODULE_S(QUERYRESULT, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(QUERYRESULT);

CQueryResult::CQueryResult(int iQueryResultID, CIML* pIML)
    : m_iQueryResultID(iQueryResultID),
    m_pIML(pIML),
    m_iViewID(-1),
    m_iTotalItems(-1),
    m_pfnNewResultsCallback(0),
    m_pUserData(0),
    m_szRequest(0)
{
}

CQueryResult::~CQueryResult()
{
    if (m_iViewID != -1)
        m_pIML->ReleaseQueryView(m_iViewID);
    delete [] m_szRequest;
}

void
CQueryResult::SetInitialRequest(char* szRequest)
{
    delete [] m_szRequest;
    m_szRequest = szRequest;
}


CGeneralQueryResult::CGeneralQueryResult(int iQueryResultID, CIML* pIML, PFNResumeQuery pfnResumeQuery)
    : CQueryResult(iQueryResultID, pIML),
    m_pfnResumeQuery(pfnResumeQuery)
{
}

CGeneralQueryResult::~CGeneralQueryResult()
{
    SimpleVector<cm_key_value_record_t> svValues;
    m_saValues.GetFilledValues(svValues);
    for (int i = 0; i < svValues.Size(); ++i)
        delete [] const_cast<TCHAR*>(svValues[i].szValue);
}

void
CGeneralQueryResult::ProcessQueryResults(iml_query_info_t* pQueryInfo)
{
    bool bQueryQueued = false;
    // If this is the first result we've received from the FML, then
    // remember to query for all the requested items that have been backlogged.
    if ((m_iViewID == -1) && (pQueryInfo->iViewID != -1))
        bQueryQueued = true;

    m_iViewID = pQueryInfo->iViewID;
    if (pQueryInfo->status == QUERY_SUCCESSFUL)
    {
        m_iTotalItems = pQueryInfo->iTotalItems;
        m_saValues.AddValues(*(pQueryInfo->pKeyValues), pQueryInfo->iStartIndex);
    }
    delete pQueryInfo->pKeyValues;
    m_saQueryState.RemoveValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);

    if (bQueryQueued)
    {
        SimpleVector<int> svRanges;
        m_saQueryState.GetFilledRanges(svRanges);
        for (int i = 0; i < svRanges.Size() - 1; i += 2)
            m_pIML->ResumeQueryLibrary(svRanges[i], svRanges[i + 1], m_iViewID, m_iQueryResultID);
    }

    if (m_pfnNewResultsCallback)
        if (m_pfnNewResultsCallback(this, pQueryInfo->status, pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned, m_pUserData))
            if (pQueryInfo->iViewID == -1)
                m_pIML->RequeryGeneral(m_iQueryResultID, m_szRequest, pQueryInfo->pCallback);
            else
                QueryValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);
}

int
CGeneralQueryResult::GetValues(ContentKeyValueVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false)
{
static cm_key_value_record_t s_keyNoValue = 
{
    iKey    : 0,
    szValue : {'\0'}
};

    int count = m_saValues.GetValues(svValues, iStartIndex, iItemCount, s_keyNoValue);
    if (bQueryIfMissing)
    {
        int iQueryStartIndex = -1;
        for (int i = 0; i < svValues.Size(); ++i)
        {
            if (svValues[i].iKey == 0)
            {
                if (iQueryStartIndex == -1)
                    iQueryStartIndex = i;
            }
            else
            {
                if (iQueryStartIndex != -1)
                {
                    // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                    // Check with current query list to see if it needs to be queried.
                    DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                    QueryValues(iStartIndex + iQueryStartIndex, i - iQueryStartIndex);
                    iQueryStartIndex = -1;
                }
            }
        }
        if (iQueryStartIndex != -1)
        {
            DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svValues.Size() - 1);
            QueryValues(iStartIndex + iQueryStartIndex, svValues.Size() - iQueryStartIndex);
        }
    }
    return count;
}

//! Query values in the given range.
//! If the values are already being queried, then nothing is done.
void
CGeneralQueryResult::QueryValues(int iStartIndex, int iItemCount)
{
    SimpleVector<bool> svQuerying;
    m_saQueryState.GetValues(svQuerying, iStartIndex, iItemCount, false);

    int iQueryStartIndex = -1;
    for (int i = 0; i < svQuerying.Size(); ++i)
    {
        if (!svQuerying[i])
        {
            if (iQueryStartIndex == -1)
                iQueryStartIndex = i;
            svQuerying[i] = true;
        }
        else
        {
            if (iQueryStartIndex != -1)
            {
                // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                // Check with current query list to see if it needs to be queried.
                DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                iQueryStartIndex = -1;
                // Call the FML to query items.
                if (m_iViewID != -1)
                    (m_pIML->*m_pfnResumeQuery)(iStartIndex + iQueryStartIndex, i - iQueryStartIndex, m_iViewID, m_iQueryResultID);
            }
        }
    }
    if (iQueryStartIndex != -1)
    {
        DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svQuerying.Size() - 1);
        // Call the FML to query items.
        if (m_iViewID != -1)
            (m_pIML->*m_pfnResumeQuery)(iStartIndex + iQueryStartIndex, svQuerying.Size() - iQueryStartIndex, m_iViewID, m_iQueryResultID);
    }

    // Add to the list of indices currently being queried.
    m_saQueryState.AddValues(svQuerying, iStartIndex);
}





CMediaQueryResult::CMediaQueryResult(int iQueryResultID, CIML* pIML)
    : CQueryResult(iQueryResultID, pIML)
{
}

CMediaQueryResult::~CMediaQueryResult()
{
    SimpleVector<iml_media_info_t> svValues;
    m_saValues.GetFilledValues(svValues);
    for (int i = 0; i < svValues.Size(); ++i)
    {
        delete [] svValues[i].szMediaTitle;
        delete [] svValues[i].szArtistName;
    }
}

void
CMediaQueryResult::ProcessQueryResults(iml_query_library_info_t* pQueryInfo)
{
    bool bQueryQueued = false;
    // If this is the first result we've received from the FML, then
    // remember to query for all the requested items that have been backlogged.
    if ((m_iViewID == -1) && (pQueryInfo->iViewID != -1))
        bQueryQueued = true;

    m_iViewID = pQueryInfo->iViewID;
    if (pQueryInfo->status == QUERY_SUCCESSFUL)
    {
        m_iTotalItems = pQueryInfo->iTotalItems;
        m_saValues.AddValues(*(pQueryInfo->pRecords), pQueryInfo->iStartIndex);
    }
    delete pQueryInfo->pRecords;
    m_saQueryState.RemoveValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);

    if (bQueryQueued)
    {
        SimpleVector<int> svRanges;
        m_saQueryState.GetFilledRanges(svRanges);
        for (int i = 0; i < svRanges.Size() - 1; i += 2)
            m_pIML->ResumeQueryLibrary(svRanges[i], svRanges[i + 1], m_iViewID, m_iQueryResultID);
    }

    if (m_pfnNewResultsCallback)
        if (m_pfnNewResultsCallback(this, pQueryInfo->status, pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned, m_pUserData))
        {
            if (pQueryInfo->iViewID == -1)
                m_pIML->RequeryMedia(m_iQueryResultID, m_szRequest);
            else
                QueryValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);
        }
}

int
CMediaQueryResult::GetValues(IMLMediaInfoVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false)
{
static iml_media_info_t s_keyNoValue =
{
    iMediaKey       : 0,
    szMediaTitle    : {'\0'},
    szArtistName    : {'\0'},
    iCodecID        : 0
};

    int count = m_saValues.GetValues(svValues, iStartIndex, iItemCount, s_keyNoValue);
    if (bQueryIfMissing)
    {
        int iQueryStartIndex = -1;
        for (int i = 0; i < svValues.Size(); ++i)
        {
            if (svValues[i].iMediaKey == 0)
            {
                if (iQueryStartIndex == -1)
                    iQueryStartIndex = i;
            }
            else
            {
                if (iQueryStartIndex != -1)
                {
                    // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                    // Check with current query list to see if it needs to be queried.
                    DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing media values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                    QueryValues(iStartIndex + iQueryStartIndex, i - iQueryStartIndex);
                    iQueryStartIndex = -1;
                }
            }
        }
        if (iQueryStartIndex != -1)
        {
            DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing media values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svValues.Size() - 1);
            QueryValues(iStartIndex + iQueryStartIndex, svValues.Size() - iQueryStartIndex);
        }
    }
    return count;
}

//! Query values in the given range.
//! If the values are already being queried, then nothing is done.
void
CMediaQueryResult::QueryValues(int iStartIndex, int iItemCount)
{
    SimpleVector<bool> svQuerying;
    m_saQueryState.GetValues(svQuerying, iStartIndex, iItemCount, false);

    int iQueryStartIndex = -1;
    for (int i = 0; i < svQuerying.Size(); ++i)
    {
        if (!svQuerying[i])
        {
            if (iQueryStartIndex == -1)
                iQueryStartIndex = i;
            svQuerying[i] = true;
        }
        else
        {
            if (iQueryStartIndex != -1)
            {
                // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                // Check with current query list to see if it needs to be queried.
                DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying media values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                iQueryStartIndex = -1;
                // Call the FML to query items.
                if (m_iViewID != -1)
                    m_pIML->ResumeQueryLibrary(iStartIndex + iQueryStartIndex, i - iQueryStartIndex, m_iViewID, m_iQueryResultID);
            }
        }
    }
    if (iQueryStartIndex != -1)
    {
        DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying media values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svQuerying.Size() - 1);
        // Call the FML to query items.
        if (m_iViewID != -1)
            m_pIML->ResumeQueryLibrary(iStartIndex + iQueryStartIndex, svQuerying.Size() - iQueryStartIndex, m_iViewID, m_iQueryResultID);
    }

    // Add to the list of indices currently being queried.
    m_saQueryState.AddValues(svQuerying, iStartIndex);
}





CRadioStationQueryResult::CRadioStationQueryResult(int iQueryResultID, CIML* pIML)
    : CQueryResult(iQueryResultID, pIML)
{
}

CRadioStationQueryResult::~CRadioStationQueryResult()
{
    SimpleVector<iml_radio_station_info_t> svValues;
    m_saValues.GetFilledValues(svValues);
    for (int i = 0; i < svValues.Size(); ++i)
    {
        delete [] svValues[i].szStationName;
        delete [] svValues[i].szURL;
    }
}

void
CRadioStationQueryResult::ProcessQueryResults(iml_query_radio_stations_info_t* pQueryInfo)
{
    bool bQueryQueued = false;
    // If this is the first result we've received from the FML, then
    // remember to query for all the requested items that have been backlogged.
    if ((m_iViewID == -1) && (pQueryInfo->iViewID != -1))
        bQueryQueued = true;

    m_iViewID = pQueryInfo->iViewID;
    if (pQueryInfo->status == QUERY_SUCCESSFUL)
    {
        m_iTotalItems = pQueryInfo->iTotalItems;
        m_saValues.AddValues(*(pQueryInfo->pStations), pQueryInfo->iStartIndex);
    }
    delete pQueryInfo->pStations;
    m_saQueryState.RemoveValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);

    if (bQueryQueued)
    {
        SimpleVector<int> svRanges;
        m_saQueryState.GetFilledRanges(svRanges);
        for (int i = 0; i < svRanges.Size() - 1; i += 2)
            m_pIML->ResumeQueryRadioStations(svRanges[i], svRanges[i + 1], m_iViewID, m_iQueryResultID);
    }

    if (m_pfnNewResultsCallback)
        if (m_pfnNewResultsCallback(this, pQueryInfo->status, pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned, m_pUserData))
            if (pQueryInfo->iViewID == -1)
                m_pIML->RequeryRadioStations(m_iQueryResultID, m_szRequest);
            else
                QueryValues(pQueryInfo->iStartIndex, pQueryInfo->iItemsReturned);
}

int
CRadioStationQueryResult::GetValues(IMLRadioStationInfoVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false)
{
static iml_radio_station_info_t s_keyNoValue =
{
    szStationName   : {'\0'},
    szURL           : {'\0'},
    iCodecID        : 0
};

    int count = m_saValues.GetValues(svValues, iStartIndex, iItemCount, s_keyNoValue);
    if (bQueryIfMissing)
    {
        int iQueryStartIndex = -1;
        for (int i = 0; i < svValues.Size(); ++i)
        {
            if (svValues[i].szURL == 0)
            {
                if (iQueryStartIndex == -1)
                    iQueryStartIndex = i;
            }
            else
            {
                if (iQueryStartIndex != -1)
                {
                    // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                    // Check with current query list to see if it needs to be queried.
                    DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing station values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                    QueryValues(iStartIndex + iQueryStartIndex, i - iQueryStartIndex);
                    iQueryStartIndex = -1;
                }
            }
        }
        if (iQueryStartIndex != -1)
        {
            DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Missing station values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svValues.Size() - 1);
            QueryValues(iStartIndex + iQueryStartIndex, svValues.Size() - iQueryStartIndex);
        }
    }
    return count;
}

//! Query values in the given range.
//! If the values are already being queried, then nothing is done.
void
CRadioStationQueryResult::QueryValues(int iStartIndex, int iItemCount)
{
    SimpleVector<bool> svQuerying;
    m_saQueryState.GetValues(svQuerying, iStartIndex, iItemCount, false);

    int iQueryStartIndex = -1;
    for (int i = 0; i < svQuerying.Size(); ++i)
    {
        if (!svQuerying[i])
        {
            if (iQueryStartIndex == -1)
                iQueryStartIndex = i;
            svQuerying[i] = true;
        }
        else
        {
            if (iQueryStartIndex != -1)
            {
                // The range from iQueryStartIndex to iQueryStartIndex + i - 1 is missing.
                // Check with current query list to see if it needs to be queried.
                DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying station values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + i - 1);
                iQueryStartIndex = -1;
                // Call the FML to query items.
                if (m_iViewID != -1)
                    m_pIML->ResumeQueryRadioStations(iStartIndex + iQueryStartIndex, i - iQueryStartIndex, m_iViewID, m_iQueryResultID);
            }
        }
    }
    if (iQueryStartIndex != -1)
    {
        DEBUGP(QUERYRESULT, DBGLEV_INFO, "%w: Querying station values %d to %d\n", m_pIML->GetFriendlyName(), iStartIndex + iQueryStartIndex, iStartIndex + svQuerying.Size() - 1);
        // Call the FML to query items.
        if (m_iViewID != -1)
            m_pIML->ResumeQueryRadioStations(iStartIndex + iQueryStartIndex, svQuerying.Size() - iQueryStartIndex, m_iViewID, m_iQueryResultID);
    }

    // Add to the list of indices currently being queried.
    m_saQueryState.AddValues(svQuerying, iStartIndex);
}
