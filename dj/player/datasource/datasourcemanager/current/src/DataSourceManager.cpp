//........................................................................................
//........................................................................................
//.. File Name: DataSourceManager.cpp													..
//.. Date: 1/1/2001																		..
//.. Author(s): Ed Miller																..
//.. Description of content: contains the definition of the CDataSourceManager class	..
//.. Usage: The CDataSourceManager class keeps a list of the available data stores and	..
//..		provides access to them.
//.. Last Modified By: Ed Miller  edwardm@iobjects.com
//.. Modification date: 1/1/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................

#include <datasource/datasourcemanager/DataSourceManager.h>

#include <datasource/common/DataSource.h>
#include "DataSourceManagerImp.h"

#include <util/debug/debug.h>

DEBUG_MODULE_S(DATASOURCEMANAGER, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE);
DEBUG_USE_MODULE(DATASOURCEMANAGER);

// The global singleton data source manager.
static CDataSourceManager*	s_pDataSourceManager = 0;

// The next free instance ID to give to a data source.
static int s_iNewDataSourceID = 1;

CDataSourceManager::CDataSourceManager()
{
    m_pDataSourceManagerImp = new CDataSourceManagerImp;
    DBASSERT(DATASOURCEMANAGER, m_pDataSourceManagerImp, "Error constructing implementation class");
}

CDataSourceManager::~CDataSourceManager()
{
    delete m_pDataSourceManagerImp;
}

// Returns a pointer to the global data source manager.
CDataSourceManager*
CDataSourceManager::GetInstance()
{
    if (!s_pDataSourceManager)
        s_pDataSourceManager = new CDataSourceManager;
    return s_pDataSourceManager;
}

// Destroy the singleton global data source manager.
void
CDataSourceManager::Destroy()
{
    delete s_pDataSourceManager;
    s_pDataSourceManager = 0;
}

// Adds a data source to the list.
void
CDataSourceManager::AddDataSource(IDataSource* pDataSource)
{
    if (pDataSource)
    {
        m_pDataSourceManagerImp->AddDataSource(pDataSource);
        pDataSource->SetInstanceID(s_iNewDataSourceID++);
    }
}

// Removes a data source from the list.
void
CDataSourceManager::RemoveDataSource(IDataSource* pDataSource)
{
    m_pDataSourceManagerImp->RemoveDataSource(pDataSource);
}

// Returns the number of data sources in the system.
int
CDataSourceManager::GetDataSourceCount()
{
    return m_pDataSourceManagerImp->GetDataSourceCount();
}

// Returns a pointer to a data source with the given ID.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManager::GetDataSourceByID(int iDataSourceID)
{
    return m_pDataSourceManagerImp->GetDataSourceByID(iDataSourceID);
}

// Returns a pointer to a data source at the given index.
// Returns 0 if the index is out-of-bounds.
IDataSource*
CDataSourceManager::GetDataSourceByIndex(int index)
{
    return m_pDataSourceManagerImp->GetDataSourceByIndex(index);
}

// Returns a pointer to the data source of the given type using a zero-based index.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManager::GetDataSourceByClassID(int iDataSourceClassID, int index)
{
    return m_pDataSourceManagerImp->GetDataSourceByClassID(iDataSourceClassID, index);
}

// Asks each data source if the given URL refers to it.
// Returns a pointer to the data source that claims the URL.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManager::GetDataSourceByURL(const char* szURL)
{
    return m_pDataSourceManagerImp->GetDataSourceByURL(szURL);
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for reading.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IInputStream for this file type.
IInputStream*
CDataSourceManager::OpenInputStream(IContentRecord* pRecord)
{
    return m_pDataSourceManagerImp->OpenInputStream(pRecord);
}

// Tells the data source manager to find the data source associated with this URL
// and asks the source to open this record for reading.
// Returns 0 if the URL was unable to be opened, otherwise
// it returns the proper subclass of IInputStream for this file type.
IInputStream*
CDataSourceManager::OpenInputStream(const char* szURL)
{
    return m_pDataSourceManagerImp->OpenInputStream(szURL);
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for writing.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CDataSourceManager::OpenOutputStream(IContentRecord* pRecord)
{
    return m_pDataSourceManagerImp->OpenOutputStream(pRecord);
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for writing.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CDataSourceManager::OpenOutputStream(const char* szURL)
{
    return m_pDataSourceManagerImp->OpenOutputStream(szURL);
}


// Starts a content refresh on the specified data source.
unsigned short
CDataSourceManager::RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    return m_pDataSourceManagerImp->RefreshContent(iDataSourceID, mode, iUpdateChunkSize);
}

// Given a list of content records, this passes them to the data source for metadata retrieval.
void
CDataSourceManager::GetContentMetadata(content_record_update_t* pContentUpdate)
{
    m_pDataSourceManagerImp->GetContentMetadata(pContentUpdate);
}

// Used to send an EVENT_CONTENT_METADATA_UPDATE_END message.
void
CDataSourceManager::NotifyContentMetadataUpdateEnd(int iDataSourceID)
{
    m_pDataSourceManagerImp->NotifyContentMetadataUpdateEnd(iDataSourceID);
}

//! Tells the data source manager to stop getting metadata for files from the given data source.
void
CDataSourceManager::NotifyMediaRemoved(int iDataSourceID)
{
    m_pDataSourceManagerImp->NotifyMediaRemoved(iDataSourceID);
}

