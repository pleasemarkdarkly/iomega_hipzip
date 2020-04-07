//
// FatDataSourceImp.cpp
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include "FatDataSourceImp.h"

#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */
#include <ctype.h>      /* isspace */
#include <errno.h>

#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>
#include <content/common/ContentManager.h>
#include <content/common/Metadata.h>
#include <core/events/SystemEvents.h>  // event types
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/debug/debug.h>
#include <util/eventq/EventQueueAPI.h>

#include <core/playmanager/PlayManager.h>

#include <fs/fat/sdapi.h>

// For media change
#include <cyg/io/io.h>
#include <io/storage/blk_dev.h>
#include <io/storage/drives.h>

#include <main/util/filenamestore/FileNameStore.h>
#include <main/main/AppSettings.h>
#include <main/main/FatHelper.h>
#include <main/main/EventTypes.h>

DEBUG_USE_MODULE(FATDATASOURCE);

CFatDataSourceImp::CFatDataSourceImp(int iDriveNumber, cyg_io_handle_t hDrive, bool bMediaInitialized)
    : IDataSource(FAT_DATA_SOURCE_CLASS_ID),
      m_iDataSourceID(0),
      m_iDriveNumber(iDriveNumber),
      m_hDrive(hDrive),
      m_bMediaInitialized(bMediaInitialized),
      m_eRefreshMode(DSR_ONE_PASS_WITH_METADATA)
{
    GetMediaGeometry();
    register_timer(CheckMediaStatusCB, (void*)this, TIMER_MILLISECONDS(1000), -1, &m_TimerHandle);
    resume_timer(m_TimerHandle);
}

CFatDataSourceImp::~CFatDataSourceImp()
{
    pc_system_close(m_iDriveNumber);
}

//! Sets the default refresh mode of the data source.
//! If DSR_DEFAULT is passed in, then the original default refresh setting for the data source should be used.
void
CFatDataSourceImp::SetDefaultRefreshMode(IDataSource::RefreshMode mode)
{
    if (mode == DSR_DEFAULT)
        m_eRefreshMode = DSR_ONE_PASS_WITH_METADATA;
    else
        m_eRefreshMode = mode;
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CFatDataSourceImp::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    sprintf(szRootURLPrefix, "file://%c:", 'A' + m_iDriveNumber);
    return true;
}

ERESULT
CFatDataSourceImp::ListAllEntries(IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    if( !m_bMediaInitialized ) {
        if( !pc_system_init( m_iDriveNumber ) ) {
            put_event(EVENT_CONTENT_UPDATE_ERROR, (void*)GetInstanceID());
            return MAKE_ERESULT(SEVERITY_FAILED, 0, 0);
        }
        m_bMediaInitialized = true;
    }

    put_event(EVENT_CONTENT_UPDATE_BEGIN, (void*)GetInstanceID());

    RefreshMode eUseMode = mode == DSR_DEFAULT ? m_eRefreshMode : mode;
    RefreshContentThreadFunc(eUseMode, iUpdateChunkSize == DS_DEFAULT_CHUNK_SIZE ? m_iUpdateChunkSize : iUpdateChunkSize);

//    put_event(EVENT_CONTENT_UPDATE_END, (void*)GetInstanceID());

    if (eUseMode == DSR_TWO_PASS)
        put_event(EVENT_CONTENT_METADATA_UPDATE_BEGIN, (void*)GetInstanceID());

    return MAKE_ERESULT(SEVERITY_SUCCESS, 0, 0);
}

void
CFatDataSourceImp::AddMediaRecordToVector(media_record_info_t& mediaContent, content_record_update_t** ppContentUpdate)
{
    (*ppContentUpdate)->media.PushBack(mediaContent);
    /*
    if (iUpdateChunkSize &&
        (((*ppContentUpdate)->media.Size() + (*ppContentUpdate)->playlists.Size()) >= iUpdateChunkSize))
    {
        content_record_update_t* pContentUpdate = new content_record_update_t;
        pContentUpdate->iDataSourceID = GetInstanceID();
        pContentUpdate->bTwoPass = (*ppContentUpdate)->bTwoPass;
        put_event(EVENT_CONTENT_UPDATE, (void*)*ppContentUpdate);
        *ppContentUpdate = pContentUpdate;
    }
    */
}

void
CFatDataSourceImp::AddPlaylistRecordToVector(playlist_record_t& playlistRecord, content_record_update_t** ppContentUpdate)
{
    (*ppContentUpdate)->playlists.PushBack(playlistRecord);
}

void
CFatDataSourceImp::CheckMediaStatusCB(void* arg)
{
    //  DEBUGP(FATDATASOURCE, DBGLEV_INFO, "Checking media status\n");
    ((CFatDataSourceImp*)arg)->GetMediaStatus(true);
}

// Queries the media status to see if a disc has been removed or inserted.
Cyg_ErrNo
CFatDataSourceImp::GetMediaStatus(bool bSendEvent)
{
    Cyg_ErrNo err = ENOERR;
    cyg_uint32 len = sizeof(cyg_uint8);
	
    err = cyg_io_get_config(m_hDrive, IO_BLK_GET_CONFIG_MEDIA_STATUS, &len, &len);
    if (err == -ENOMED)
    {
        if (m_bMediaInitialized && bSendEvent)
        {
            DEBUGP(FATDATASOURCE, DBGLEV_INFO, "&&& No media &&&\n");
            m_bMediaInitialized = false;  // Only send one media removed message.
            // Send a message to the UI.
            put_event(EVENT_MEDIA_REMOVED, (void*)GetInstanceID());
        }
    }
    else if (err == -EMEDCHG)
    {
        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "&&& Media changed &&&\n");
        GetMediaGeometry();
        if (bSendEvent)
        {
            m_bMediaInitialized = true;
            // Send a message to the UI.
            put_event(EVENT_MEDIA_INSERTED, (void*)GetInstanceID());
        }
    }
    else if (err == -ESLEEP)
    {
        /* TODO Drive sleep not yet supported on Dar */
    }
    return err;
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream*
CFatDataSourceImp::OpenInputStream(const char* szURL)
{
    DBASSERT(FATDATASOURCE, szURL, "Opening null URL");
    DEBUGP(FATDATASOURCE, DBGLEV_INFO, "OpenInputStream: %s\n", szURL);

    CFatFileInputStream* pFFIS = new CFatFileInputStream();
    if (FAILED(pFFIS->Open(szURL + 7)))
    {
        delete pFFIS;
        return 0;
    }
    else
        return pFFIS;
}

//! Asks the source to open this URL for writing.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IOutputStream for this file type.
IOutputStream*
CFatDataSourceImp::OpenOutputStream(const char* szURL)
{
    CFatFileOutputStream* pFFOS = new CFatFileOutputStream();
    if (FAILED(pFFOS->Open(szURL + 7)))
    {
        delete pFFOS;
        return 0;
    }
    else
        return pFFOS;
}

//! Asks the source the length of the media serial number, if available.
//! This function actually isn't const since we cache the serial number.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CFatDataSourceImp::GetSerialNumberLength() const
{
    // fail if no media
    if( !m_bMediaInitialized ) return -1;

    return m_dgGeometry.serial_len;
}

//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CFatDataSourceImp::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    // fail if no media
    if( !m_bMediaInitialized ) return -1;
    
    // this implicitly forces a geometry refresh
    if( iBufferLen < GetSerialNumberLength() ) {
        return -1;
    }

    // copy the serial number if available
    if( m_dgGeometry.serial_len ) {
        memcpy( (void*) pBuffer, m_dgGeometry.serial_num, m_dgGeometry.serial_len );
    }
    
    return m_dgGeometry.serial_len;
}

//! Cache the media geometry from the block device
int
CFatDataSourceImp::GetMediaGeometry() 
{
    cyg_uint32 len = sizeof( m_dgGeometry );
    Cyg_ErrNo err = cyg_io_get_config( m_hDrive, IO_BLK_GET_CONFIG_GEOMETRY, (void*) &m_dgGeometry, &len );
    if( err < 0 ) {
        return -1;
    }
    return 0;
}

//
// Static members
//

#if 0
static char*
MakeFullFilename(DSTAT& statobj)
{
    char* full_filename = 0;
#if defined(__DHARMA)
	make_string( full_filename, 7 /* file:// */ + strlen(statobj.path) + 1 /* \\ */+
		 strlen(statobj.fname) + 1 /* . */ + strlen(statobj.fext) + 1);
	sprintf( full_filename, "file://%s\\%s.%s", statobj.path, statobj.fname, statobj.fext );
#else /* __DHARMA */
	make_string( full_filename, strlen(statobj.path) + 1 /* \\ */+
		 strlen(statobj.fname) + 1 /* . */ + strlen(statobj.fext) + 1);
	sprintf( full_filename, "%s\\%s.%s", statobj.path, statobj.fname, statobj.fext );
#endif /* __DHARMA */

    return full_filename;
}

static char*
strextend(char* str1, char* str2)
{
    char* strout = (char*)malloc(strlen(str1) + strlen(str2) + 1);
    strcpy(strout, str1);
    strcat(strout, str2);
    return strout;
}

static char*
StatobjToFullFilename(const char* szRoot, DSTAT& statobj)
{
    char* szFullname = (char*)malloc(strlen(szRoot) + 1);
    strcpy(szFullname, szRoot);
    if (strlen(statobj.path) >= 3)
    {
        char szBlah[EMAXPATH];
        strcpy(szBlah, statobj.path);
        char* pch = szBlah + 3;
        while (char* pchSlash = strchr(pch, '\\'))
        {
            *pchSlash = '\0';
            DSTAT statobjparent;
            if (pc_gfirst(&statobjparent, szBlah))
            {
                diag_printf("Parent LFN: %s\n", statobjparent.longFileName);
                char* szTemp = GenerateURL(szFullname, statobjparent);
                free(szFullname);
                szFullname = szTemp;
                *pchSlash++ = '\\';
                pch = pchSlash;
                pc_gdone(&statobjparent);
            }
            else
                break;
        }
        DSTAT statobjparent;
        if (pc_gfirst(&statobjparent, szBlah))
        {
            diag_printf("Parent LFN: %s\n", statobjparent.longFileName);
            char* szTemp = GenerateURL(szFullname, statobjparent);
            free(szFullname);
            szFullname = szTemp;
            pc_gdone(&statobjparent);
        }
    }

    char* szTemp = GenerateURL(szFullname, statobj);
    free(szFullname);
    szFullname = szTemp;

    free(szFullname);
    return 0;
}
#endif // 0


void
CFatDataSourceImp::RefreshContentThreadFunc(IDataSource::RefreshMode mode, int iUpdateChunkSize)
{
    suspend_timer(m_TimerHandle);

    SimpleList<CStoreFileNameRef*> dqueue;
    char szName[EMAXPATH];
    char mask[EMAXPATH];
    char* path = 0;
    DSTAT statobj;

    char root[6];
    sprintf( root, "%c:", 'A' + m_iDriveNumber );
    char longroot [11];
    sprintf( longroot, "file://%c:", 'A' + m_iDriveNumber );
    
    // create a new store to contain the current filesystem information
    CFileNameStore* store = new CFileNameStore ( root, longroot, this->GetInstanceID() );

    strcpy (mask,root);
    strcat (mask, "\\*.*");
    
    dqueue.PushBack(store->GetRoot());
   
    while (!dqueue.IsEmpty())
    {
        CStoreFileNameRef *refDir(dqueue.PopFront());
        // grab the full path.  the only way for this to happen is through dynamic allocation (currently), so remember to delete it later.
        path = refDir->Path();
        strcpy(mask, path);
        strcat (mask, "\\*.*");

        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "%s: mask: %s\n", __FUNCTION__, mask);
        if( pc_gfirst( &statobj, mask ) )
        {
            do
            {
                if( statobj.fname[0] == '.' )
		            continue;
                else if (statobj.fattribute & (AHIDDEN | AVOLUME))
                    continue;
                else 
                {
                    // trim out any trailing spaces in the file name + extension fields
                    TrimStatNames(statobj);
                    // skip the file if its path is too long
                    if (!ValidateShortPathLength(statobj))
                        continue;
                    // prep a short name, in the routine work string
                    MakeShortFilename(statobj,szName);
                    // prep a dynamic long name, to be freed after creating the node
                    char* szLongName = MakeLongFilename(statobj);
                    // handle directories
                    if( statobj.fattribute & ADIRENT )
                    {
                        // create a new directory node
                        CDirFileNameNode* dir = new CDirFileNameNode(szName, szLongName);
                        (**refDir)->AddChild(dir);
                        // push a reference onto the stack to be processed later
                        dqueue.PushBack((CStoreFileNameRef*)dir->GetRef());
                    }
                    // handle normal files
                    else
                    {
                        DEBUGP(FATDATASOURCE, DBGLEV_INFO, "File %s\n", szName);
                        // create a file node to represent the file we found
                        CFileFileNameNode* file = new CFileFileNameNode(szName, szLongName);
                        // insert the file into the store
                        (**refDir)->AddChild(file);
		            }
                    // free up the long name
                    free (szLongName);
                }
            } while(pc_gnext(&statobj));
            if (refDir->DynamicAlloc())
                delete [] path;
    	    pc_gdone( &statobj );
	    }
        delete refDir;
    }
    DEBUGP( FATDATASOURCE, DBGLEV_TRACE, "FDS:After full walk, we found the following files\n"); 
    store->PrintStructureReport();
    put_event(EVENT_FILESYSTEM_UPDATE, (void*) store); 
    resume_timer(m_TimerHandle);
}

void CFatDataSourceImp::CreateMediaContentRecord(IFileNameRef* file, content_record_update_t* pContentUpdate)
{
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CPlaylistFormatManager* pPFM = CPlaylistFormatManager::GetInstance();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
    IFileNameNode* pNode = **((CStoreFileNameRef*)file);
    
    char* extension = ExtensionFromFilename(pNode->Name());
    if (extension == pNode->Name())
        return;
    if (RegKey codecID = pCodecMgr->FindCodecID(extension))
    {
        media_record_info_t mediaContent;
        mediaContent.bVerified = true;
        mediaContent.iDataSourceID = GetInstanceID();
        mediaContent.iCodecID = codecID;
        mediaContent.pFilename = file;
        if ((mediaContent.pMetadata = pCM->CreateMetadataRecord()))
        {
            if (mediaContent.pMetadata->UsesAttribute(MDA_FILE_NAME))
            {
                TCHAR tszScratch[256];
                if (pNode->LongName() != 0)
                    mediaContent.pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch,pNode->LongName(),256));
                else
                    mediaContent.pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch,pNode->Name(),256));
            }
            if (ICodec* pCodec = pCodecMgr->FindCodec(mediaContent.iCodecID,(void*)m_pCodecWorkspace, CODEC_WORKSPACE_SIZE))
            {
                CFatFileInputStream FFIS;
                char* path = mediaContent.pFilename->Path();
                if (SUCCEEDED(FFIS.Open(path)))
                {
                    pCodec->GetMetadata(this, mediaContent.pMetadata, &FFIS);
                    mediaContent.pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)FFIS.Length());
                    FFIS.Close();
                }
                if (mediaContent.pFilename->DynamicAlloc())
                    delete [] path;
                pCodec->~ICodec();
                pCodec = NULL;
            }
        }
        else
            mediaContent.pMetadata = NULL;
        AddMediaRecordToVector(mediaContent, &pContentUpdate);
    }
    else if (unsigned int iPlaylistFormat = pPFM->FindPlaylistFormat(extension))
    {
        playlist_record_t playlistRecord;
        playlistRecord.bVerified = true;
        playlistRecord.iDataSourceID = GetInstanceID();
        playlistRecord.iPlaylistFormatID = iPlaylistFormat;
        playlistRecord.pFilename = file;
        AddPlaylistRecordToVector(playlistRecord, &pContentUpdate);
    }
    else
        delete file;

}

// Retrieves metadata for each media content record in the passed-in list.
void
CFatDataSourceImp::GetContentMetadata(content_record_update_t* pContentUpdate)
{
#if 0
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    CCodecManager* pCodecMgr = CCodecManager::GetInstance();
    for (int i = 0; i < pContentUpdate->media.Size(); ++i)
    {
        DSTAT statobj;
        if ( pc_gfirst( &statobj, pContentUpdate->media[i].pFilename + 7))
        {
            if ( (pContentUpdate->media[i].pMetadata = pCM->CreateMetadataRecord()) )
            {
                if (ICodec* pCodec = pCodecMgr->FindCodec(pContentUpdate->media[i].iCodecID,(void*)m_pCodecWorkspace, CODEC_WORKSPACE_SIZE))
                {
                    pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_SIZE, (void*)statobj.fsize);
                    CFatFileInputStream FFIS;
                    if (SUCCEEDED(FFIS.Open(pContentUpdate->media[i].pFilename + 7)))
                    {
                        pCodec->GetMetadata(this, pContentUpdate->media[i].pMetadata, &FFIS);
                        FFIS.Close();
                    }

                    if (pContentUpdate->media[i].pMetadata->UsesAttribute(MDA_FILE_NAME))
                    {
                        TCHAR tszScratch[256];
                        if (statobj.longFileName[0] != '\0')
                            pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, statobj.longFileName, 256));
                        else
                        {
                            char szFilename[14];
                            szFilename[0] = '\0';
                            AddStrippedString(szFilename, statobj.fname, 8);
                            strcat(szFilename, ".");
                            AddStrippedString(szFilename, statobj.fext, 3);
                            pContentUpdate->media[i].pMetadata->SetAttribute(MDA_FILE_NAME, (void*)CharToTcharN(tszScratch, szFilename, 14));
                        }
                    }
                    pCodec->~ICodec();
                    pCodec = NULL;
                }
            }
    	    pc_gdone( &statobj );
        }
    }
    put_event(EVENT_CONTENT_METADATA_UPDATE, (void*)pContentUpdate);
#endif
}

void 
CFatDataSourceImp::CreateMediaRecordsForNewFiles(FileRefList* lstNewFiles)
{
    content_record_update_t* pContentUpdate = new content_record_update_t;
    pContentUpdate->iDataSourceID = GetInstanceID();
    pContentUpdate->bTwoPass = false; //mode == DSR_TWO_PASS;
    // iterate over the list of files
    for (FileRefListItr itrFile = lstNewFiles->GetHead(); itrFile != lstNewFiles->GetEnd(); ++itrFile)
        // create media content records when appropriate
        CreateMediaContentRecord( *itrFile, pContentUpdate);
    // get rid of the list
    delete lstNewFiles;
    // update if any of the files were either media or playlist.
    if (pContentUpdate->media.Size() || pContentUpdate->playlists.Size())
    {
        put_event(EVENT_CONTENT_UPDATE, (void*)pContentUpdate);
        put_event(EVENT_CONTENT_UPDATE_END, (void*)GetInstanceID());
    }
    else
    {
        put_event(EVENT_CONTENT_UPDATE_END, (void*)GetInstanceID());
        delete pContentUpdate;
    }
}
