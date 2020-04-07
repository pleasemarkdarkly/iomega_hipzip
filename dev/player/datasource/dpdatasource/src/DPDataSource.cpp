//........................................................................................
//........................................................................................
//.. File Name: DPDataSource.cpp                                                        ..
//.. Date: 2/5/2001                                                                     ..
//.. Author(s): Ed Miller                                                               ..
//.. Description of content: contains the definition of the CDPDataSource class         ..
//.. Usage: The CDPDataSource class implements the CDPDataSource class to               ..
//..        represent the DP player.                                                    ..
//.. Last Modified By: Ed Miller  edwardm@iobjects.com                                  ..
//.. Modification date: 2/5/2001                                                        ..
//........................................................................................
//.. Copyright:(c) 1995-2001 Interactive Objects Inc.                                   ..
//..    All rights reserved. This code may not be redistributed in source or linkable  ..
//..    object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com                                              ..
//........................................................................................
//........................................................................................

#include <datasource/dpdatasource/DPDataSource.h>
#include <datastream/dataplayfile/DPInputStream.h>
#include <util/debug/debug.h>
#include <fs/dataplay/dp_hw.h>
#include <fs/dataplay/dfs.h>
#include <fs/dataplay/dp.h>

#include <string.h>     /* memset */
#include <stdlib.h>     /* calloc */
#include <stdio.h>      /* sprintf */
#include <dirent.h>

#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>
#include <core/events/SystemEvents.h>   // event types
#include <cyg/fileio/fileio.h>
#include <playlist/common/ContentManager.h>
#include <playlist/common/Metadata.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

#include <core/playmanager/PlayManager.h>

DEBUG_MODULE(DPDATASOURCE);
DEBUG_USE_MODULE(DPDATASOURCE);

CDPDataSource*
CDPDataSource::Open()
{
    DEBUGP(DPDATASOURCE, DBGLEV_INFO, "Opening device DP\n");

	// Mount the drive.
	DEBUGP(DPDATASOURCE, DBGLEV_INFO,"data play init\n");
    int dp_ret = dp_init();
	DEBUGP(DPDATASOURCE, DBGLEV_INFO,"dp returned %d on init\n",dp_ret);
	return new CDPDataSource();
}

CDPDataSource::CDPDataSource()
	: IDataSource(DP_DATA_SOURCE_TYPE_ID)
{
    m_szFilePrefix = (char*)malloc(strlen(szDeviceName) + 5 /* "dfs:/" */ + 1 /* "\0" */);
    DBASSERT(CDDATASOURCE, m_szFilePrefix, "Unable to allocate file prefix string");
    strcpy(m_szFilePrefix, "dfs:/");
}

CDPDataSource::~CDPDataSource()
{
}

void
CDPDataSource::CheckMediaStatusCB(void* arg)
{
}

// (epg,8/10/2001): todo create a disc-status query fn.

void
CDPDataSource::ListAllEntries(IContentManager* pContentManager, bool bGetCodecMetadata)
{
    PopulateDPFileList(AddMediaRecordToContentManager, AddPlaylistRecordToContentManager, (void*)pContentManager, bGetCodecMetadata);
}

void
CDPDataSource::ListAllEntriesAsynch(bool bGetCodecMetadata)
{
    m_bGetCodecMetadata = bGetCodecMetadata;

	// Start worker thread
    cyg_thread_create(ASYNCH_READ_PRIORITY,
            CDPDataSource::StartRefreshContentThreadFunc,
            (cyg_addrword_t)this,
            "DP asynch reader",
            (void*)m_ThreadStack,
            ASYNCH_READ_STACK_SIZE,
            &m_hThread,
            &m_CygThread);
    cyg_thread_resume(m_hThread);
}

void
CDPDataSource::StartRefreshContentThreadFunc(cyg_addrword_t data)
{
    content_record_update_t* pContentUpdate = new content_record_update_t;

    reinterpret_cast<CDPDataSource*>(data)->PopulateDPFileList(AddMediaRecordToVector, AddPlaylistRecordToVector, (void*)pContentUpdate, reinterpret_cast<CDPDataSource*>(data)->m_bGetCodecMetadata);

    CEventQueue::GetInstance()->PutEvent(EVENT_CONTENT_UPDATE, pContentUpdate);
}

void
CDPDataSource::AddMediaRecordToContentManager(media_record_info_t& mediaContent, void* pUserData)
{
    ((IContentManager*)pUserData)->AddMediaRecord(mediaContent);
}

void
CDPDataSource::AddMediaRecordToVector(media_record_info_t& mediaContent, void* pUserData)
{
    ((content_record_update_t*)pUserData)->media.PushBack(mediaContent);
}

void
CDPDataSource::AddPlaylistRecordToContentManager(playlist_record_t& playlistRecord, void* pUserData)
{
    ((IContentManager*)pUserData)->AddPlaylistRecord(playlistRecord);
}

void
CDPDataSource::AddPlaylistRecordToVector(playlist_record_t& playlistRecord, void* pUserData)
{
    ((content_record_update_t*)pUserData)->playlists.PushBack(playlistRecord);
}


IInputStream* 
CDPDataSource::OpenContentRecord(IContentRecord* pRecord)
{
    DBASSERT(DPDATASOURCE, pRecord, "Opening null content record");
    DEBUGP(DPDATASOURCE, DBGLEV_INFO, "OpenContentRecord: %s\n", pRecord->GetURL());

    // Verify that the track came from this data source.
    if (strncmp(pRecord->GetURL(), m_szFilePrefix, strlen(m_szFilePrefix)))
        return 0;

	CDPInputStream* ret = new CDPInputStream;
	if (pStream->Open(pRecord->GetURL()) < 0)
	{
		delete pStream;

		DEBUGP(CDDATASOURCE, DBGLEV_ERROR, "Unable to open dataplay track %s\n", pRecord->GetURL());
		return 0;
	}

	return (IInputStream*) pStream;
}

//==========================================================================

char* FindFileExtension(char* filename)
{
	char* ret = filename+strlen(filename);
	while (*--ret != '.');
	return ++ret;	// want to return first char of extension, not period.
}

#define make_string(var,len) ( var = (char*) malloc(sizeof(char)* (len)) )
#define make_wide_string(var,len) ( var = (unsigned short*) malloc(sizeof(unsigned short)* (len)) )
#define free_string(adr) free( adr )
#define shift_upper(a) (a >= 'a' && a <= 'z' ? a - ('a' - 'A') : a )

void
CDPDataSource::PopulateDPFileList(PFNAddMediaRecordCallback pfnAddMediaRecordCallback,
    PFNAddPlaylistRecordCallback pfnAddPlaylistRecordCallback,
    void* pUserData,
    bool bGetCodecMetadata)
{
    SimpleList<int> hdir_queue;	// track handles to directories
    SimpleList<char*> path_queue;	// track pathnames to directories
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
	DFSHANDLE hDir = -1;
	char* path = 0;			// path of directory currently being processed
	char* newdir = 0;		// path of subdirectory to be processed later
    int err;
	dfs_dirstate_t dirstate;
	memset ( (void*) &dirstate, 0, sizeof(dfs_dirstate_t));
	dp_dir_entry_t direntry;
	memset ( (void*) &direntry, 0, sizeof(dp_dir_entry_t));
	
	dfs_media_info_t media_info;

   	DEBUGP(DPDATASOURCE, DBGLEV_INFO, "get media info\n");
    if( (err = dfs_get_media_info( &media_info ) ) ) {
		DEBUGP(DPDATASOURCE, DBGLEV_INFO,  "Error reading dfs media info: %s\n", dp_get_error( err ) );
		//    return ;
    }
	// init parent to root?
	hDir = media_info.handle;

	make_string(path,strlen("dfs:\\")+1);
	strcpy(path,"dfs:\\");
    hdir_queue.PushBack( hDir );
    path_queue.PushBack( path );

    while (!hdir_queue.IsEmpty())
    {
        hDir = hdir_queue.PopFront();
        path = path_queue.PopFront();
        DEBUGP(DPDATASOURCE, DBGLEV_INFO, "%s: hDir: %s\n", __FUNCTION__, hDir);
        if( dfs_gfirst( hDir, &dirstate, &direntry, 0 ) )
        {
            do
            {
				diag_printf("file retrieved has characteristics: type %d\n",direntry.attrib.att_type);
                if( direntry.name[0] == '.' )
                {
					diag_printf(">>>>>>period directories are used in dataplayland!\n");
		            continue;
		        }
                else if( !direntry.attrib.att_type )
                {
                    int len = strlen( path ) + strlen( direntry.name ) + 1 /* \ */  + 1 /* \0 */ ;
                    // dc - make sure we dont push a dir that we can't possible reference files from
                    if( (len + 12) < METADATA_STRING_SIZE )
                    {
                        make_string( newdir, len );
                        sprintf( newdir, "%s%s\\", path, direntry.name );

                        DEBUGP(DPDATASOURCE, DBGLEV_INFO, "Directory Entry Information***\n");
                        DEBUGP(DPDATASOURCE, DBGLEV_INFO, "parent [%s]\n", path);
                        DEBUGP(DPDATASOURCE, DBGLEV_INFO, "direntry.name [%s]\n", direntry.name);
                        DEBUGP(DPDATASOURCE, DBGLEV_INFO, "subdir path [%s]\n", newdir);

                        hdir_queue.PushBack(direntry.handle);
						path_queue.PushBack(newdir);
                    }
                }
                else
                {
                    DEBUGP(DPDATASOURCE, DBGLEV_INFO, "%s: File %s\n", __FUNCTION__, direntry.name);
					char* extension = FindFileExtension(direntry.name);
                    if (RegKey codecID = pCodecMgr->FindCodecID(extension))
                    {
                        media_record_info_t mediaContent;
                        mediaContent.pMetadata = pCM->CreateMetadataRecord();
                        mediaContent.iDataSourceID = GetID();
                        mediaContent.iCodecID = codecID;
                        mediaContent.szURL = (char*)malloc(strlen(path) + strlen(direntry.name) + 1);
						strcpy(mediaContent.szURL, path);
						strcat(mediaContent.szURL, direntry.name);

						if (bGetCodecMetadata)
						{
                            if (ICodec* pCodec = pCodecMgr->FindCodec(mediaContent.iCodecID))
                            {
							    CDPInputStream DPIS;
							    if (DPIS.Open(mediaContent.szURL))
							    {
								    pCodec->GetMetaData(mediaContent.pMetadata, &DPIS);
							    }
                                delete pCodec;
                            }
						}
						(*pfnAddMediaRecordCallback)(mediaContent, pUserData);
            		}
                    else if (unsigned int iPlaylistFormat = pPFM->FindPlaylistFormat(extension))
                    {
                        playlist_record_t playlistRecord;
                        playlistRecord.iDataSourceID = GetID();
                        playlistRecord.iPlaylistFormatID = iPlaylistFormat;
                        playlistRecord.szURL = (char*)malloc(strlen(path) + strlen(direntry.name) + 1);
						strcpy(playlistRecord.szURL, path);
						strcat(playlistRecord.szURL, direntry.name);
						(*pfnAddPlaylistRecordCallback)(playlistRecord, pUserData);
                    }
		        }
            } while(dfs_gnext(&dirstate, &direntry));

    	    dfs_gdone( &dirstate );
			free_string(path);	// we've exhausted this path, so free it and move on.
	    }
		else
			free_string(path);	// no entries in this directory, so just free the path and move on.
    }
}
//==========================================================================
