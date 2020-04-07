//........................................................................................
//........................................................................................
//.. File Name: DataSourceManagerImp.h                                                  ..
//.. Date: 7/6/2001                                                                     ..
//.. Author(s): Ed Miller                                                               ..
//.. Description of content: contains the definition of the CDataSourceManagerImp class ..
//.. Usage: The CDataSourceManagerImp class hides the implementation details of the     ..
//..        CDataSourceManager class.                                                   ..
//.. Last Modified By: Ed Miller  edwardm@iobjects.com                                  ..
//.. Modification date: 7/6/2001                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.                                   ..
//..     All rights reserved. This code may not be redistributed in source or linkable  ..
//..     object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................

#include "DataSourceManagerImp.h"

#include <content/common/ContentManager.h>
#include <core/events/SystemEvents.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

#include <string.h> /* strnicmp */

DEBUG_USE_MODULE(DATASOURCEMANAGER);

CDataSourceManagerImp::CDataSourceManagerImp()
    : m_usScanID(0)
{
    cyg_mutex_init(&m_mtxMessages);
    cyg_semaphore_init(&m_semMessages, 0);
    CreateRefreshThread();
}

CDataSourceManagerImp::~CDataSourceManagerImp()
{
    DestroyRefreshThread();
    cyg_mutex_destroy(&m_mtxMessages);
    cyg_semaphore_destroy(&m_semMessages);
}

// Adds a data source to the list.
void
CDataSourceManagerImp::AddDataSource(IDataSource* pDataSource)
{
    m_slDataSources.PushBack(pDataSource);
}

// Removes a data source from the list.
void
CDataSourceManagerImp::RemoveDataSource(IDataSource* pDataSource)
{
    for (SimpleListIterator<IDataSource*> itBlah = m_slDataSources.GetHead(); itBlah != m_slDataSources.GetEnd(); ++itBlah)
        if (*itBlah == pDataSource)
        {
            m_slDataSources.Remove(itBlah);
            return;
        }
}

// Returns the number of data sources in the system.
int
CDataSourceManagerImp::GetDataSourceCount()
{
    return m_slDataSources.Size();
}

// Returns a pointer to a data source with the given ID.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManagerImp::GetDataSourceByID(int iDataSourceID)
{
    for (SimpleListIterator<IDataSource*> itBlah = m_slDataSources.GetHead(); itBlah != m_slDataSources.GetEnd(); ++itBlah)
        if ((*itBlah)->GetInstanceID() == iDataSourceID)
            return *itBlah;

    return 0;
}

// Returns a pointer to a data source at the given index.
// Returns 0 if the index is out-of-bounds.
IDataSource*
CDataSourceManagerImp::GetDataSourceByIndex(int index)
{
    if ((index < 0) || (index >= m_slDataSources.Size()))
        return 0;

    SimpleListIterator<IDataSource*> itBlah = m_slDataSources.GetHead();
    for (int i = 0; (i < index) && (itBlah != m_slDataSources.GetEnd()); ++itBlah, ++i)
        ;
    return *itBlah;
}

// Returns a pointer to the data source of the given type using a zero-based index.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManagerImp::GetDataSourceByClassID(int iDataSourceClassID, int index)
{
    SimpleListIterator<IDataSource*> it = m_slDataSources.GetHead();
    for (; it != m_slDataSources.GetEnd(); ++it)
        if ((*it)->GetClassID() == iDataSourceClassID)
            if (index-- <= 0)
                return *it;

    return 0;
}

// Asks each data source if the given URL refers to it.
// Returns a pointer to the data source that claims the URL.
// Returns 0 if no matching data source was found.
IDataSource*
CDataSourceManagerImp::GetDataSourceByURL(const char* szURL)
{
    SimpleListIterator<IDataSource*> it = m_slDataSources.GetHead();
    for (; it != m_slDataSources.GetEnd(); ++it)
    {
        char szRootURL[256];
        if ((*it)->GetRootURLPrefix(szRootURL, 256))
        {
            if (!strnicmp(szRootURL, szURL, strlen(szRootURL)))
                return *it;
        }
    }

    return 0;
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for reading.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IInputStream for this file type.
IInputStream*
CDataSourceManagerImp::OpenInputStream(IContentRecord* pRecord)
{
    IDataSource* pDataSource = GetDataSourceByID(pRecord->GetDataSourceID());
    if (pDataSource)
        return pDataSource->OpenInputStream(pRecord->GetURL());
    else
        return 0;
}

// Tells the data source manager to find the data source associated with this URL
// and asks the source to open this record for reading.
// Returns 0 if the URL was unable to be opened, otherwise
// it returns the proper subclass of IInputStream for this file type.
IInputStream*
CDataSourceManagerImp::OpenInputStream(const char* szURL)
{
    IDataSource* pDataSource = GetDataSourceByURL(szURL);
    if (pDataSource)
        return pDataSource->OpenInputStream(szURL);
    else
        return 0;
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for writing.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CDataSourceManagerImp::OpenOutputStream(IContentRecord* pRecord)
{
    IDataSource* pDataSource = GetDataSourceByID(pRecord->GetDataSourceID());
    if (pDataSource)
        return pDataSource->OpenOutputStream(pRecord->GetURL());
    else
        return 0;
}

// Tells the data source manager to find the data source associated with this content record
// and asks the source to open this record for writing.
// Returns 0 if the record was unable to be opened, otherwise
// it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CDataSourceManagerImp::OpenOutputStream(const char* szURL)
{
    IDataSource* pDataSource = GetDataSourceByURL(szURL);
    if (pDataSource)
        return pDataSource->OpenOutputStream(szURL);
    else
        return 0;
}


#define RT_EVENT_QUIT                   1
#define RT_EVENT_REFRESH_DS             2
#define RT_EVENT_GET_METADATA           3
#define RT_EVENT_METADATA_UPDATE_END    4

typedef struct rt_ds_refresh_s
{
    int     iDataSourceID;
    IDataSource::RefreshMode    mode;
    bool    bGetMetadata;
    bool    bTwoPass;
    int     iUpdateChunkSize;
    unsigned short  usScanID;

    rt_ds_refresh_s(int theDSID, unsigned short theScanID, IDataSource::RefreshMode theMode, int theUpdateChunkSize)
        : iDataSourceID(theDSID), mode(theMode), iUpdateChunkSize(theUpdateChunkSize), usScanID(theScanID)
        { }

} rt_ds_refresh_t;


// Starts a content refresh on the specified data source.
unsigned short
CDataSourceManagerImp::RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    PutEvent(RT_EVENT_REFRESH_DS, (void*)new rt_ds_refresh_t(iDataSourceID, m_usScanID, mode, iUpdateChunkSize));
    return m_usScanID++;
}

// Given a list of content records, this passes them to the data source for metadata retrieval.
void
CDataSourceManagerImp::GetContentMetadata(content_record_update_t* pContentUpdate)
{
    PutEvent(RT_EVENT_GET_METADATA, (void*)pContentUpdate);
}

// Used to send an EVENT_CONTENT_METADATA_UPDATE_END message.
void
CDataSourceManagerImp::NotifyContentMetadataUpdateEnd(int iDataSourceID)
{
    PutEvent(RT_EVENT_METADATA_UPDATE_END, (void*)iDataSourceID);
}

// Tells the data source manager to stop getting metadata for files from the given data source.
void
CDataSourceManagerImp::NotifyMediaRemoved(int iDataSourceID)
{
    // Get a lock on the message queue so we won't be disturbed as we gut it.
    cyg_mutex_lock(&m_mtxMessages);

    // Remove all events linked to the given data source ID.
    SimpleListIterator<event_t> it = m_slMessages.GetHead();
    while (it != m_slMessages.GetEnd())
    {
        SimpleListIterator<event_t> itNext = it + 1;

        switch ((*it).key)
        {
            case RT_EVENT_REFRESH_DS:
            {
                rt_ds_refresh_t* pRefresh = reinterpret_cast<rt_ds_refresh_t*>((*it).data);
                if (pRefresh->iDataSourceID == iDataSourceID)
                {
                    // Decrease the semaphore count.
                    cyg_semaphore_wait(&m_semMessages);
                    delete pRefresh;
                    m_slMessages.Remove(it);
                }
                break;
            }

            case RT_EVENT_GET_METADATA:
            {
                content_record_update_t* pUpdate = reinterpret_cast<content_record_update_t*>((*it).data);
                if (pUpdate->iDataSourceID == iDataSourceID)
                {
                    // Decrease the semaphore count.
                    cyg_semaphore_wait(&m_semMessages);

                    // Free the content update struct.
                    for (int i = 0; i < pUpdate->media.Size(); ++i)
                    {
                        free(pUpdate->media[i].szURL);
                        delete pUpdate->media[i].pMetadata;
                    }
                    for (int i = 0; i < pUpdate->playlists.Size(); ++i)
                        free(pUpdate->playlists[i].szURL);
                    delete pUpdate;

                    m_slMessages.Remove(it);
                }
                break;
            }

            case RT_EVENT_METADATA_UPDATE_END:
            {
                if (GET_DATA_SOURCE_ID((int)(*it).data) == iDataSourceID)
                {
                    // Decrease the semaphore count.
                    cyg_semaphore_wait(&m_semMessages);
                    m_slMessages.Remove(it);
                }
                break;
            }

            default:
                break;
        }

        it = itNext;

    }
    cyg_mutex_unlock(&m_mtxMessages);

}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DS event queue


void CDataSourceManagerImp::PutEvent( unsigned int key, void* data )
{
    event_t event;
    event.key = key;
    event.data = data;

    cyg_mutex_lock(&m_mtxMessages);
    m_slMessages.PushBack(event);
    cyg_semaphore_post(&m_semMessages);
    cyg_mutex_unlock(&m_mtxMessages);
}

void CDataSourceManagerImp::GetEvent( unsigned int* key, void** data )
{
    event_t event;

    cyg_semaphore_wait(&m_semMessages);
    cyg_mutex_lock(&m_mtxMessages);
    event = m_slMessages.PopFront();
    cyg_mutex_unlock(&m_mtxMessages);
    
    *key = event.key;
    *data = event.data;
}

DEBUG_MODULE_S( RT, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( RT );

void
CDataSourceManagerImp::CreateRefreshThread()
{
    DBEN( RT );

    cyg_thread_create( REFRESH_THREAD_PRIORITY,
                       CDataSourceManagerImp::RefreshThreadEntryPoint,
                       (cyg_addrword_t) this,
                       "Refresh thread",
                       (void*) m_RefreshThreadStack,
                       REFRESH_THREAD_STACK_SIZE,
                       &m_hRefreshThread,
                       &m_RefreshThreadData );

    cyg_thread_resume( m_hRefreshThread );

    DBEX( RT );
}

void
CDataSourceManagerImp::DestroyRefreshThread()
{
    DBEN( RT );

    cyg_thread_suspend( m_hRefreshThread );
    while( !cyg_thread_delete( m_hRefreshThread ) ) {
        // this should never have to execute
        cyg_thread_delay( 1 );
    }

    DBEX( RT );
}

// Thread entry routines
void CDataSourceManagerImp::RefreshThreadEntryPoint( cyg_addrword_t data )
{
    DBEN( RT );

    reinterpret_cast<CDataSourceManagerImp*>(data)->RefreshThread();

    DBEX( RT );
}

void CDataSourceManagerImp::RefreshThread()
{
    DBEN( RT );

    unsigned int key;
    do
    {
        void* data;
        GetEvent(&key, &data);

        switch (key)
        {
            case RT_EVENT_REFRESH_DS:
            {
                rt_ds_refresh_t* pRefresh = reinterpret_cast<rt_ds_refresh_t*>(data);
                DEBUGP( RT, DBGLEV_INFO, "RT message %d: Refresh DS %d Scan ID %d\n", key, pRefresh->iDataSourceID, pRefresh->usScanID);
                if (IDataSource* pDS = GetDataSourceByID( pRefresh->iDataSourceID ))
                {
                    pDS->ListAllEntries(pRefresh->usScanID, pRefresh->mode, pRefresh->iUpdateChunkSize);
                }
                delete pRefresh;
                break;
            }

            case RT_EVENT_GET_METADATA:
            {
                content_record_update_t* pUpdate = reinterpret_cast<content_record_update_t*>(data);
                DEBUGP( RT, DBGLEV_INFO, "RT message %d: Update DS %d\n", key, pUpdate->iDataSourceID);
                pUpdate->bTwoPass = false;
                if (IDataSource* pDS = GetDataSourceByID( pUpdate->iDataSourceID ))
                {
                    pDS->GetContentMetadata(pUpdate);
                }
                break;
            }

            case RT_EVENT_METADATA_UPDATE_END:
            {
                put_event(EVENT_CONTENT_METADATA_UPDATE_END, data);
                break;
            }

            default:
                DEBUGP( RT, DBGLEV_INFO, "RT message: %d\n", key);
                break;
        }

    } while (key != RT_EVENT_QUIT);

    DBEX( RT );
}
