//
// QueryResultManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef QUERYRESULTMANAGER_H_
#define QUERYRESULTMANAGER_H_

#include <main/iml/iml/IML.h>
#include <util/datastructures/SimpleList.h>

class CQueryResult;
class CGeneralQueryResult;
class CMediaQueryResult;
class CRadioStationQueryResult;

//! The QueryResult manager keeps a list of all QueryResults added to the system.
class CQueryResultManager
{
public:

    //! Returns a pointer to the global QueryResult manager.
    static CQueryResultManager* GetInstance();

    //! Destroy the singleton global QueryResult manager.
    static void Destroy();

    //! Creates a new query result object and adds it to the list.
    CGeneralQueryResult* CreateGeneralQueryResult(CIML* pIML, PFNResumeQuery pfnResumeQuery);

    //! Creates a new query result object and adds it to the list.
    CMediaQueryResult* CreateMediaQueryResult(CIML* pIML);

    //! Creates a new query result object and adds it to the list.
    CRadioStationQueryResult* CreateRadioStationQueryResult(CIML* pIML);

    //! Removes a query result from the list.
    void RemoveQueryResult(CQueryResult* pQueryResult);

    //! Removes all query results for the given IML from the list.
    void RemoveQueryResultsFromIML(CIML* pIML);

    //! Returns the number of QueryResults in the system.
    int GetQueryResultCount();

    //! Returns a pointer to a QueryResult with the given instance ID.
    //! Returns 0 if no matching QueryResult was found.
    CQueryResult* GetQueryResultByID(int iQueryResultID);

    //! Returns a pointer to a QueryResult at the given zero-based index.
    //! Returns 0 if the index is out-of-bounds.
    CQueryResult* GetQueryResultByIndex(int index);


private:

    CQueryResultManager();
    ~CQueryResultManager();

    static CQueryResultManager* s_pSingleton;   // The global singleton QueryResult manager.
    static int s_iNewQueryResultID;             // The next free instance ID to give to a QueryResult.

    SimpleList<CQueryResult*> m_slQueryResults;         // The list of QueryResults tracked by the manager.

};

#endif	// QUERYRESULTMANAGER_H_
