//
// DataSourceManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef DATASOURCEMANAGER_H_
#define DATASOURCEMANAGER_H_

#include <datasource/common/DataSource.h>   // RefreshMode

class CDataSourceManagerImp;
class IContentRecord;
class IInputStream;
class IOutputStream;

typedef struct content_record_update_s content_record_update_t;

//! The data source manager keeps a list of all data sources added to the system.
//! It has a worker thread responsible for invoking content and metadata updates
//! on the data sources.
class CDataSourceManager
{
public:

    //! Returns a pointer to the global data source manager.
    static CDataSourceManager* GetInstance();

    //! Destroy the singleton global data source manager.
    static void Destroy();

    //! Adds a data source to the list.
    void AddDataSource(IDataSource* pDataSource);

    //! Removes a data source from the list.
    void RemoveDataSource(IDataSource* pDataSource);

    //! Returns the number of data sources in the system.
    int GetDataSourceCount();

    //! Returns a pointer to a data source with the given instance ID.
    //! Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByID(int iDataSourceID);

    //! Returns a pointer to a data source at the given zero-based index.
    //! Returns 0 if the index is out-of-bounds.
    IDataSource* GetDataSourceByIndex(int index);

    //! Returns a pointer to the data source of the given type using a zero-based index.
    //! Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByClassID(int iDataSourceClassID, int index);

    //! Compares each data source's URL prefix to the given URL.
    //! Returns a pointer to the data source that matches the URL prefix.
    //! Returns 0 if no matching data source was found.
    IDataSource* GetDataSourceByURL(const char* szURL);

    //! Tells the data source manager to find the data source associated with this content record
    //! and asks the source to open this record for reading.
    //! Returns 0 if the record was unable to be opened, otherwise
    //! it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(IContentRecord* pRecord);

    //! Tells the data source manager to find the data source associated with this URL
    //! and asks the source to open this record for reading.
    //! Returns 0 if the URL was unable to be opened, otherwise
    //! it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenInputStream(const char* szURL);

    //! Tells the data source manager to find the data source associated with this content record
    //! and asks the source to open this record for writing.
    //! Returns 0 if the record was unable to be opened, otherwise
    //! it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(IContentRecord* pRecord);

    //! Tells the data source manager to find the data source associated with this content record
    //! and asks the source to open this record for writing.
    //! Returns 0 if the record was unable to be opened, otherwise
    //! it returns the proper subclass of IOutputStream for this file type.
    IOutputStream* OpenOutputStream(const char* szURL);

    //! Starts a content refresh on the specified data source.
    //! \retval The scan ID of the refresh cycle.
    unsigned short RefreshContent(int iDataSourceID, IDataSource::RefreshMode mode, int iUpdateChunkSize);

    //! Given a list of content records, this passes them to the data source for metadata retrieval.
    void GetContentMetadata(content_record_update_t* pContentUpdate);

    //! Used to send an EVENT_CONTENT_METADATA_UPDATE_END message.
    void NotifyContentMetadataUpdateEnd(int iDataSourceID);

    //! Tells the data source manager to stop getting metadata for files from the given data source.
    void NotifyMediaRemoved(int iDataSourceID);

private:

    CDataSourceManager();
    ~CDataSourceManager();

    CDataSourceManagerImp*  m_pDataSourceManagerImp;

};

#endif	// DATASOURCEMANAGER_H_
