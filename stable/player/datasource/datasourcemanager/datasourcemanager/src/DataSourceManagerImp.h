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

#ifndef DATASOURCEMANAGERIMP_H_
#define DATASOURCEMANAGERIMP_H_

#include <datasource/common/DataSource.h>   // RefreshMode
#include <util/datastructures/SimpleList.h>
#include <cyg/kernel/kapi.h>

class IContentRecord;
class IInputStream;
class IOutputStream;

typedef struct content_record_update_s content_record_update_t;

class CDataSourceManagerImp
{
public:

    CDataSourceManagerImp();
    ~CDataSourceManagerImp();

    // Adds a data source to the list.
    void AddDataSource(IDataSource* pDataSource);

    // Removes a data source from the list.
    void RemoveDataSource(IDataSource* pDataSource);

    // Returns the number of data sources in the system.
    int GetDataSourceCount();

    // Returns a pointer to a data source with the given ID.
    // Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByID(int iDataSourceID);

    // Returns a pointer to a data source at the given index.
    // Returns 0 if the index is out-of-bounds.
    IDataSource* GetDataSourceByIndex(int index);

    // Returns a pointer to the data source of the given type using a zero-based index.
    // Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByClassID(int iDataSourceClassID, int index);

    // Asks each data source if the given URL refers to it.
    // Returns a pointer to the data source that claims the URL.
    // Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByURL(const char* szURL);

    // Tells the data source manager to find the data source associated with this content record
    // and asks the source to open this record for reading.
    // Returns 0 if the record was unable to be opened, otherwise
    // it returns the proper subclass of CInputStream for this file type.
    IInputStream* OpenContentRecord(IContentRecord* pRecord);

    // Tells the data source manager to find the data source associated with this content record
    // and asks the source to open this record for reading.
    // Returns 0 if the record was unable to be opened, otherwise
    // it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(IContentRecord* pRecord);

    // Tells the data source manager to find the data source associated with this URL
    // and asks the source to open this record for reading.
    // Returns 0 if the URL was unable to be opened, otherwise
    // it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(const char* szURL);

    // Tells the data source manager to find the data source associated with this content record
    // and asks the source to open this record for writing.
    // Returns 0 if the record was unable to be opened, otherwise
    // it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(IContentRecord* pRecord);

    // Tells the data source manager to find the data source associated with this content record
    // and asks the source to open this record for writing.
    // Returns 0 if the record was unable to be opened, otherwise
    // it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(const char* szURL);

    // Starts a content refresh on the specified data source.
    // \retval The scan ID of the refresh cycle.
    unsigned short RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize);

    // Given a list of content records, this passes them to the data source for metadata retrieval.
    void GetContentMetadata(content_record_update_t* pContentUpdate);

    // Used to send an EVENT_CONTENT_METADATA_UPDATE_END message.
    void NotifyContentMetadataUpdateEnd(int iDataSourceID);

    // Tells the data source manager to stop getting metadata for files from the given data source.
    void NotifyMediaRemoved(int iDataSourceID);

private:

    typedef SimpleList<IDataSource*> DataSourceList;
    DataSourceList				m_slDataSources;	// The list of currently available data sources.

    // The scan ID to use for the next refresh cycle.
    unsigned short  m_usScanID;

    // DS event queue
    void PutEvent( unsigned int, void* );
    void GetEvent( unsigned int*, void** );

    // internal event representation
    typedef struct event_s 
    {
        unsigned int key;
        void* data;
    } event_t;

    SimpleList<event_t> m_slMessages;
    cyg_sem_t   m_semMessages;
    cyg_mutex_t m_mtxMessages;

    // table of event structures
    event_t* m_pEventTable;

#define REFRESH_THREAD_STACK_SIZE   8*1024
#define REFRESH_THREAD_PRIORITY     14

    // Thread handle, data, and stack
    cyg_handle_t m_hRefreshThread;
    cyg_thread   m_RefreshThreadData;
    char         m_RefreshThreadStack[ REFRESH_THREAD_STACK_SIZE ] __attribute__((aligned(16)));

    void CreateRefreshThread();
    void DestroyRefreshThread();

    // Thread entry routines
    static void RefreshThreadEntryPoint( cyg_addrword_t data );
    void RefreshThread();


};

#endif	// DATASOURCEMANAGER_H_
