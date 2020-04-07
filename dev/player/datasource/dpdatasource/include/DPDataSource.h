//........................................................................................
//........................................................................................
//.. File Name: DPDataSource.h                                                          ..
//.. Date: 2/5/2001                                                                     ..
//.. Author(s): Ed Miller                                                               ..
//.. Description of content: contains the definition of the CDPDataSource class         ..
//.. Last Modified By: Eric Gibbs   ericg@iobjects.com                                  ..
//.. Modification date: 2/5/2001                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.                                   ..
//..    All rights reserved. This code may not be redistributed in source or linkable  ..
//..    object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................

#ifndef DPDATASOURCE_H_
#define DPDATASOURCE_H_

#include <datasource/common/DataSource.h>
#include <io/storage/blk_dev.h>
#include <util/timer/Timer.h>

class IInputStream;
class IContentManager;
typedef struct media_record_info_s media_record_info_t;
typedef struct playlist_record_s playlist_record_t;

// TODO: rethink this
#define DP_DATA_SOURCE_CLASS_ID  4

#define ASYNCH_READ_PRIORITY   10
#define ASYNCH_READ_STACK_SIZE (8 * 1024)

class CDPDataSource : public IDataSource
{
  public:

    // Attempts to open the drive with the given device name.
    // Returns a pointer to a new CDPDataSource object if successful, 0 otherwise.
    static CDPDataSource* Open(void);

    ~CDPDataSource();

    int GetClassID() const
        { return DP_DATA_SOURCE_CLASS_ID; }
    int GetInstanceID() const
        { return m_iDataSourceID; }

    // Queries the media status to see if a disc has been removed or inserted.
    Cyg_ErrNo GetMediaStatus(bool bSendEvent);

    // Asks the source to create a playlist that contains all of the content
    // it can access.
    // For now this adds entries to a playlist passed in, but in the future it should use events
    // in case the querying takes time.
    void ListAllEntries(IContentManager* pContentManager, bool bGetCodecMetadata);
    void ListAllEntriesAsynch(bool bGetCodecMetadata);

    // Asks the source to open this content record for reading.
    // Returns 0 if the record was unable to be opened, otherwise
    // it returns the proper subclass of IInputStream for this file type.
    IInputStream* OpenContentRecord(IContentRecord* pRecord);

    long Read(long lba, long sectors, void *buffer);

  private:

    CDPDataSource();

    // Called by the data source manager to assign an ID to this data source.
    void SetID(int iDataSourceID)
        { m_iDataSourceID = iDataSourceID; }

    static void StartRefreshContentThreadFunc(cyg_addrword_t data);
    typedef void (*PFNAddMediaRecordCallback)(media_record_info_t& mediaContent, void* pUserData);
    typedef void (*PFNAddPlaylistRecordCallback)(playlist_record_t& mediaContent, void* pUserData);
    void RefreshContentThreadFunc(PFNAddMediaRecordCallback pfnAddMediaRecordCallback, PFNAddPlaylistRecordCallback pfnAddPlaylistRecordCallback, void* pUserData, bool bGetCodecMetadata);
    static void AddMediaRecordToContentManager(media_record_info_t& mediaContent, void* pUserData);
    static void AddMediaRecordToVector(media_record_info_t& mediaContent, void* pUserData);
    static void AddPlaylistRecordToContentManager(playlist_record_t& playlistRecord, void* pUserData);
    static void AddPlaylistRecordToVector(playlist_record_t& playlistRecord, void* pUserData);
    bool    m_bGetCodecMetadata;

    int m_iDataSourceID;

    // Disc/drive description
    char*           m_szDeviceName;
    char*           m_szMountDirectory;
    char*           m_szFilePrefix;
    cyg_io_handle_t	m_hDP;				// Handle to the DP drive.

    char            m_ThreadStack[ASYNCH_READ_STACK_SIZE];
    cyg_thread      m_CygThread;
    cyg_handle_t    m_hThread;

    int DisableReadCache();
    Cyg_ErrNo DoPacket(unsigned char *command, unsigned char *data, int iDataLength, int dir);

    // Traverses the given directory and all subdirectories, adding files with recognized extensions to the playlist.
    void PopulateDPFileList(PFNAddMediaRecordCallback pfnAddMediaRecordCallback, PFNAddPlaylistRecordCallback pfnAddPlaylistRecordCallback, void* pUserData, bool bGetCodecMetadata);

    // Used for checking media status at regular intervals.
    timer_handle_t  m_TimerHandle;
    static void CheckMediaStatusCB(void* arg);
    bool            m_bNoMedia;
};

#endif	// DPDATASOURCE_H_
