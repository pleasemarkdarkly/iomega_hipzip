//
// File Name: QueryResult.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef QUERYRESULT_H_
#define QUERYRESULT_H_

#include <content/common/QueryableContentManager.h>
#include <main/djupnp/DJUPnP.h>
#include <main/djupnp/UPnPEvents.h>
#include <main/iml/iml/IML.h>
#include <main/util/datastructures/SparseArray.h>
#include <util/datastructures/SimpleList.h>
#include <util/eventq/EventQueueAPI.h>
#include <util/upnp/api/upnp.h>


#include <stdlib.h>
#include <util/debug/debug.h>

//! The CQueryResult class is a convenience class for querying remote iMLs.
//! It's an abstract base class that remembers a query ID, view ID, total number
//! of results in a query, and a pointer to the iML that's being queried.
//! Derived classes expose functions for querying content and accessing the
//! available results of queries.
class CQueryResult
{
public:

    CQueryResult(int iQueryResultID, CIML* pIML);
    virtual ~CQueryResult() = 0;

    //! Sets the XML text used for making the initial query, in case we need to requery.
    //! This memory should be new[]'ed by the caller.  The query result object owns the memory after this call.
    void SetInitialRequest(char* szRequest);

    //! Returns the query result ID.
    int GetQueryResultID() const
        { return m_iQueryResultID; }

    //! Return the iML associated with this query result.
    CIML* GetIML()
        { return m_pIML; }

    //! Get the view ID assigned by the iML for this query.
    //! If a view ID hasn't yet been assigned, then -1 is returned.
    int GetViewID() const
        { return m_iViewID; }

    //! Returns the total number of items expected in the query.
    //! If the total is not yet known, then -1 is returned.
    int GetTotalItemCount() const
        { return m_iTotalItems; }

    //! Returns the number of items that currently exist in the result object.
    virtual int GetFilledItemCount() const = 0;

    //! Query values in the given range.
    //! If the values are already being queried, then nothing is done.
    virtual void QueryValues(int iStartIndex, int iItemCount) = 0;

    //! Populates an array with the ranges of filled values in the query result.
    //! Each range is represented as a pair of integers: start index and item count.
    //! For example, if values 5 - 10, 15 - 25, and 45 - 100 were filled, then
    //! svRanges would be { 5, 6, 15, 11, 45, 56 }
    virtual void GetFilledRanges(SimpleVector<int>& svRanges) = 0;

//! Function typedef for new results callback.
//! \param pQR The query result object that generated the event.
//! \param iStartIndex The starting index of the new data in the query result.
//! \param iItemCount The number of new items in the query result.
//! \param pUserData A pointer to user-defined data, specified when the callback is set.
//! \retval If true then the query will be retried; if false then nothing will be done.
typedef bool FNNewResultsCallback(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData);

    //! Sets the function that will be called when new query results arrive.
    virtual void SetNewResultsCallback(FNNewResultsCallback* pfnNewResultsCallback, void* pUserData = 0)
        { m_pfnNewResultsCallback = pfnNewResultsCallback; m_pUserData = pUserData; }

protected:

    int     m_iQueryResultID;   // The query ID assigned by the query result manager.
    CIML*   m_pIML;             // The FML that was queried to generate this result.
    int     m_iViewID;          // The view ID assigned by the FML.
    int     m_iTotalItems;      // The total number of results expected.
    FNNewResultsCallback*   m_pfnNewResultsCallback;
                                // Function to call when new results arrive.
    void*   m_pUserData;        // User-defined data to pass to the new results callback.
    char* m_szRequest;          // The XML text used in the initial query.

};

//! This class is used for keeping track of the results of queries for artists, albums,
//! genres, playlists, and radio stations.
class CGeneralQueryResult : public CQueryResult
{
public:

    CGeneralQueryResult(int iQueryResultID, CIML* pIML, PFNResumeQuery pfnResumeQuery);
    ~CGeneralQueryResult();

    //! Returns the number of items that currently exist in the result object.
    int GetFilledItemCount() const
        { return m_saValues.FilledSize(); }

    //! Called from the UI event loop.
    //! Adds the new results to the list.
    void ProcessQueryResults(iml_query_info_t* pQueryInfo);

    //! Fills a vector with query results.
    //! \param svValues A vector to hold the results.  Values with no result will have a iKey value of 0.
    //! \param iStartIndex The zero-based starting index in the query view to get values for.
    //! \param iItemCount The number of items to get.
    //! \param bQueryIfMissing If true, then missing items will be queried by the FML.
    //! \retval The number of items in the result vector that have an actual value.
    int GetValues(ContentKeyValueVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false);

    void GetFilledRanges(SimpleVector<int>& svRanges)
        { m_saValues.GetFilledRanges(svRanges); }
    
    //! Query values in the given range.
    //! If the values are already being queried, then nothing is done.
    void QueryValues(int iStartIndex, int iItemCount);

private:

    PFNResumeQuery  m_pfnResumeQuery;   // Used to request more data from the FML.
    SparseArray<cm_key_value_record_t>  m_saValues;
    SparseArray<bool>   m_saQueryState; // Used to keep track of which items are currently being queried.
};


//! This class is used for keeping track of the results of queries for media items.
class CMediaQueryResult : public CQueryResult
{
public:

    CMediaQueryResult(int iQueryResultID, CIML* pIML);
    ~CMediaQueryResult();

    //! Returns the number of items that currently exist in the result object.
    int GetFilledItemCount() const
        { return m_saValues.FilledSize(); }

    //! Called from the UI event loop.
    //! Adds the new results to the list.
    void ProcessQueryResults(iml_query_library_info_t* pQueryInfo);

    //! Fills a vector with query results.
    //! \param svValues A vector to hold the results.  Values with no result will have a iMediaKey value of 0.
    //! \param iStartIndex The zero-based starting index in the query view to get values for.
    //! \param iItemCount The number of items to get.
    //! \param bQueryIfMissing If true, then missing items will be queried by the FML.
    //! \retval The number of items in the result vector that have an actual value.
    int GetValues(IMLMediaInfoVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false);

    //! Query values in the given range.
    //! If the values are already being queried, then nothing is done.
    void QueryValues(int iStartIndex, int iItemCount);

    void GetFilledRanges(SimpleVector<int>& svRanges)
        { m_saValues.GetFilledRanges(svRanges); }
    
private:

    SparseArray<iml_media_info_t>   m_saValues;
    SparseArray<bool>   m_saQueryState; // Used to keep track of which items are currently being queried.
};


//! This class is used for keeping track of the results of queries for radio stations.
class CRadioStationQueryResult : public CQueryResult
{
public:

    CRadioStationQueryResult(int iQueryResultID, CIML* pIML);
    ~CRadioStationQueryResult();

    //! Returns the number of items that currently exist in the result object.
    int GetFilledItemCount() const
        { return m_saValues.FilledSize(); }

    //! Called from the UI event loop.
    //! Adds the new results to the list.
    void ProcessQueryResults(iml_query_radio_stations_info_t* pQueryInfo);

    //! Fills a vector with query results.
    //! \param svValues A vector to hold the results.  Values with no result will have a iMediaKey value of 0.
    //! \param iStartIndex The zero-based starting index in the query view to get values for.
    //! \param iItemCount The number of items to get.
    //! \param bQueryIfMissing If true, then missing items will be queried by the FML.
    //! \retval The number of items in the result vector that have an actual value.
    int GetValues(IMLRadioStationInfoVector& svValues, int iStartIndex, int iItemCount, bool bQueryIfMissing = false);

    //! Query values in the given range.
    //! If the values are already being queried, then nothing is done.
    void QueryValues(int iStartIndex, int iItemCount);

    void GetFilledRanges(SimpleVector<int>& svRanges)
        { m_saValues.GetFilledRanges(svRanges); }
    
private:

    SparseArray<iml_radio_station_info_t>   m_saValues;
    SparseArray<bool>   m_saQueryState; // Used to keep track of which items are currently being queried.

};


#endif	// QUERYRESULT_H_

