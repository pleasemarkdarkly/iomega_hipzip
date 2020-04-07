//
// QueryResultManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/iml/query/QueryResultManager.h>
#include <main/iml/query/QueryResult.h>

#include <util/debug/debug.h>

DEBUG_USE_MODULE(QUERYRESULTMANAGER);

// The global singleton QueryResult manager.
CQueryResultManager* CQueryResultManager::s_pSingleton = 0;

// The next free instance ID to give to a QueryResult.
int CQueryResultManager::s_iNewQueryResultID = 1;

// Returns a pointer to the global QueryResult manager.
CQueryResultManager*
CQueryResultManager::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CQueryResultManager;
    return s_pSingleton;
}

// Destroy the singleton global QueryResult manager.
void
CQueryResultManager::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

CQueryResultManager::CQueryResultManager()
{
}

CQueryResultManager::~CQueryResultManager()
{
    while (!m_slQueryResults.IsEmpty())
        delete m_slQueryResults.PopFront();
}

//! Creates a new query result object and adds it to the list.
CGeneralQueryResult*
CQueryResultManager::CreateGeneralQueryResult(CIML* pIML, PFNResumeQuery pfnResumeQuery)
{
    CGeneralQueryResult* pQueryResult = new CGeneralQueryResult(s_iNewQueryResultID++, pIML, pfnResumeQuery);
    m_slQueryResults.PushBack(pQueryResult);
    return pQueryResult;
}

//! Creates a new query result object and adds it to the list.
CMediaQueryResult*
CQueryResultManager::CreateMediaQueryResult(CIML* pIML)
{
    CMediaQueryResult* pQueryResult = new CMediaQueryResult(s_iNewQueryResultID++, pIML);
    m_slQueryResults.PushBack(pQueryResult);
    return pQueryResult;
}

//! Creates a new query result object and adds it to the list.
CRadioStationQueryResult*
CQueryResultManager::CreateRadioStationQueryResult(CIML* pIML)
{
    CRadioStationQueryResult* pQueryResult = new CRadioStationQueryResult(s_iNewQueryResultID++, pIML);
    m_slQueryResults.PushBack(pQueryResult);
    return pQueryResult;
}

// Removes a QueryResult from the list.
void
CQueryResultManager::RemoveQueryResult(CQueryResult* pQueryResult)
{
    for (SimpleListIterator<CQueryResult*> itBlah = m_slQueryResults.GetHead(); itBlah != m_slQueryResults.GetEnd(); ++itBlah)
        if (*itBlah == pQueryResult)
        {
            // Release the query through the iml.
            // If the iml deletes the query, then remove it from the list.
            // Otherwise, keep it around in case pending query results arrive.
            if (pQueryResult->GetIML()->ReleaseQuery(pQueryResult))
                m_slQueryResults.Remove(itBlah);
            return;
        }
}

//! Removes all query results for the given IML from the list.
void
CQueryResultManager::RemoveQueryResultsFromIML(CIML* pIML)
{
    SimpleListIterator<CQueryResult*> it = m_slQueryResults.GetHead();
    while (it != m_slQueryResults.GetEnd())
    {
        if ((*it)->GetIML() == pIML)
        {
            // Release the query through the iml.
            (*it)->GetIML()->ReleaseQuery(*it);
            SimpleListIterator<CQueryResult*> itDel = it;
            ++it;
            m_slQueryResults.Remove(itDel);
        }
        else
            ++it;
    }
}

// Returns the number of QueryResults in the system.
int
CQueryResultManager::GetQueryResultCount()
{
    return m_slQueryResults.Size();
}

// Returns a pointer to a QueryResult with the given ID.
// Returns 0 if no matching QueryResult was found.
CQueryResult*
CQueryResultManager::GetQueryResultByID(int iQueryResultID)
{
    for (SimpleListIterator<CQueryResult*> itBlah = m_slQueryResults.GetHead(); itBlah != m_slQueryResults.GetEnd(); ++itBlah)
        if ((*itBlah)->GetQueryResultID() == iQueryResultID)
            return *itBlah;

    return 0;
}

// Returns a pointer to a QueryResult at the given index.
// Returns 0 if the index is out-of-bounds.
CQueryResult*
CQueryResultManager::GetQueryResultByIndex(int index)
{
    if ((index < 0) || (index >= m_slQueryResults.Size()))
        return 0;

    SimpleListIterator<CQueryResult*> itBlah = m_slQueryResults.GetHead();
    for (int i = 0; (i < index) && (itBlah != m_slQueryResults.GetEnd()); ++itBlah, ++i)
        ;
    return *itBlah;
}

