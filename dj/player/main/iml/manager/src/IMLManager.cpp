//
// IMLManager.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/iml/manager/IMLManager.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <main/iml/iml/IML.h>
#include <network.h>
#include <arpa/inet.h>
#include <io/net/Net.h>

#include <util/debug/debug.h>

DEBUG_MODULE_S(IMLMANAGER, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(IMLMANAGER);

// The global singleton IML manager.
CIMLManager* CIMLManager::s_pSingleton = 0;

// The next free instance ID to give to a IML.
int CIMLManager::s_iNewIMLID = 1;

// The maximum number of IMLs to allow to be caching at once.
const int CIMLManager::sc_iMaxCaching = 1;

// Artist name to use for radio stations.
static const TCHAR sc_szRadioStationArtist[] = {'I','n','t','e','r','n','e','t',' ','R','a','d','i','o',0};

// Returns a pointer to the global IML manager.
CIMLManager*
CIMLManager::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CIMLManager;
    return s_pSingleton;
}

// Destroy the singleton global IML manager.
void
CIMLManager::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

CIMLManager::CIMLManager() :
    m_pNetDS(0),
    m_pContentManager(0),
    m_iCaching(0)
{
    // Create thread to poll IMLs that become unavailable
    cyg_thread_create(20,
        CIMLManager::PollUnavailIMLsEntry,
        (cyg_addrword_t)this,
        "Unavail IML Polling",
        (void*)m_ThreadStack,
        sizeof(m_ThreadStack),
        &m_hThread,
        &m_ThreadData);
    cyg_thread_resume(m_hThread);
}

CIMLManager::~CIMLManager()
{
    while( !cyg_thread_delete( m_hThread ) ) {
        cyg_thread_delay( 1 );
    }

    while (!m_slIMLs.IsEmpty())
        delete m_slIMLs.PopFront();
}

//! Scans the list of IMLs and starts caching, if a spot is available.
void
CIMLManager::UpdateCaching()
{
    DEBUG(IMLMANAGER, DBGLEV_TRACE, "caching: %d max: %d\n", m_iCaching, sc_iMaxCaching);
    if (m_iCaching < sc_iMaxCaching)
    {
        for (SimpleListIterator<CIML*> it = m_slIMLs.GetHead(); it != m_slIMLs.GetEnd(); ++it)
        {
            DEBUG(IMLMANAGER, DBGLEV_TRACE, "IML: %w Caching: %d\n", (*it)->GetFriendlyName(), (int)(*it)->GetCachedStatus());
            if ((*it)->GetCachedStatus() == CIML::NOT_CACHED)
            {
                ++m_iCaching;
                (*it)->StartQueryCaching();
                return;
            }
        }
    }
}

//! Populates a media content record list based on the list of media IDs passed in.
void
CIMLManager::CreateMediaRecords(MediaRecordList& records, const IMLMediaInfoVector& mediaKeyValues, const char* szBaseURL)
{
    if (m_pNetDS && m_pContentManager)
    {
        content_record_update_t contentUpdate;
        contentUpdate.bTwoPass = false;
        contentUpdate.iDataSourceID = m_pNetDS->GetInstanceID();
        for (int i = 0; i < mediaKeyValues.Size(); ++i)
        {
            if (mediaKeyValues[i].iMediaKey)
            {
                // Create a content record.
                media_record_info_t mri;
                mri.szURL = (char*)malloc(128);
                sprintf(mri.szURL, "%s=%d", szBaseURL, mediaKeyValues[i].iMediaKey);
                mri.bVerified = true;
                mri.iCodecID = mediaKeyValues[i].iCodecID;
                mri.iDataSourceID = contentUpdate.iDataSourceID;
                mri.pMetadata = m_pContentManager->CreateMetadataRecord();

                // Store the title and artist in the metadata.
                if (mri.pMetadata)
                {
                    mri.pMetadata->SetAttribute(MDA_TITLE, (void*)mediaKeyValues[i].szMediaTitle);
                    mri.pMetadata->SetAttribute(MDA_ARTIST, (void*)mediaKeyValues[i].szArtistName);
                }

                contentUpdate.media.PushBack(mri);
            }
        }
        m_pContentManager->AddContentRecords(&contentUpdate, &records);
    }
}

//! Creates a media content record for the media IDs passed in.
IMediaContentRecord*
CIMLManager::CreateMediaRecord(const iml_media_info_t& mediaInfo, const char* szBaseURL)
{
    char szURL[128];
    sprintf(szURL, "%s=%d", szBaseURL, mediaInfo.iMediaKey);
    // Create a content record and store the title in the metadata.
    // TODO: artist, album, etc.
    if (IMediaContentRecord* pRecord = m_pNetDS->GenerateEntry(CPlayManager::GetInstance()->GetContentManager(), szURL, mediaInfo.iCodecID))
    {
        pRecord->SetAttribute(MDA_TITLE, (void*)mediaInfo.szMediaTitle);
        pRecord->SetAttribute(MDA_ARTIST, (void*)mediaInfo.szArtistName);
        return pRecord;
    }
    return 0;
}

//! Creates a media content record for the given radio stream.
IMediaContentRecord*
CIMLManager::CreateMediaRecord(const iml_radio_station_info_t& radioInfo)
{
    // Create a content record and store the title in the metadata.
    // TODO: artist, album, etc.
    if (IMediaContentRecord* pRecord = m_pNetDS->GenerateEntry(CPlayManager::GetInstance()->GetContentManager(), radioInfo.szURL, radioInfo.iCodecID))
    {
        pRecord->SetAttribute(MDA_TITLE, (void*)radioInfo.szStationName);
        pRecord->SetAttribute(MDA_ARTIST, (void*)sc_szRadioStationArtist);
        return pRecord;
    }
    return 0;
}

//! Clears all IML media records from the content manager.
void
CIMLManager::ClearMediaRecords()
{
    // Assume that all records in the content manager are from the IML.
    // This is true in the case of the simpler content manager.
    m_pContentManager->Clear();
}

// Adds a IML to the list.
void
CIMLManager::AddIML(CIML* pIML)
{
    if (pIML)
    {
        m_slIMLs.PushBack(pIML);
        pIML->SetInstanceID(s_iNewIMLID++);

        UpdateCaching();
    }
}

// Removes a IML from the list.
void
CIMLManager::RemoveIML(CIML* pIML)
{
    for (SimpleListIterator<CIML*> it = m_slIMLs.GetHead(); it != m_slIMLs.GetEnd(); ++it)
        if (*it == pIML)
        {
            if ((*it)->GetCachedStatus() == CIML::CACHING)
            {
                --m_iCaching;
                UpdateCaching();
            }
            m_slIMLs.Remove(it);
            return;
        }
}

// Mark an IML as unavailable but do not remove it completely
void
CIMLManager::SetIMLUnavailable(CIML* pIML)
{
    DEBUGP(IMLMANAGER, DBGLEV_WARNING, "Setting %s Unavailable\n", pIML->GetMediaBaseURL());
    DJUPnPControlPointRemoveDevice(pIML->GetDeviceNumber());
}

// Returns the number of IMLs in the system.
int
CIMLManager::GetIMLCount()
{
    return m_slIMLs.Size();
}

//! Called when an IML is done caching its top-level queries.
//! The manager will check the list to see if there are any other IMLs that need to be cached.
//! If so, it will tell one IML to begin caching.
void
CIMLManager::NotifyIMLCached(CIML* pIML)
{
    if (pIML->GetCachedStatus() == CIML::CACHING)
    {
        --m_iCaching;
        pIML->SetCachedStatus(CIML::CACHED);
        UpdateCaching();
    }
}

//! Returns a pointer to a IML with the given device number.
//! Returns 0 if no matching IML was found.
CIML*
CIMLManager::GetIMLByDeviceNumber(int iDeviceNumber)
{
    for (SimpleListIterator<CIML*> it = m_slIMLs.GetHead(); it != m_slIMLs.GetEnd(); ++it)
        if ((*it)->GetDeviceNumber() == iDeviceNumber)
            return *it;

    return 0;
}

// Returns a pointer to a IML with the given ID.
// Returns 0 if no matching IML was found.
CIML*
CIMLManager::GetIMLByID(int iIMLID)
{
    for (SimpleListIterator<CIML*> it = m_slIMLs.GetHead(); it != m_slIMLs.GetEnd(); ++it)
        if ((*it)->GetInstanceID() == iIMLID)
            return *it;

    return 0;
}

// Returns a pointer to a IML at the given index.
// Returns 0 if the index is out-of-bounds.
CIML*
CIMLManager::GetIMLByIndex(int index)
{
    if ((index < 0) || (index >= m_slIMLs.Size()))
        return 0;

    SimpleListIterator<CIML*> it = m_slIMLs.GetHead();
    for (int i = 0; (i < index) && (it != m_slIMLs.GetEnd()); ++it, ++i)
        ;
    return *it;
}

// Returns the number of IMLs in the system with fully cached top-level queries.
int
CIMLManager::GetCachedIMLCount()
{
    int count = 0;
    for (SimpleListIterator<CIML*> it = m_slIMLs.GetHead(); it != m_slIMLs.GetEnd(); ++it)
        if ((*it)->GetCachedStatus() == CIML::CACHED)
            ++count;

    return count;
}

// Returns a pointer to a cached IML at the given zero-based index.
// Returns 0 if the index is out-of-bounds.
CIML*
CIMLManager::GetCachedIMLByIndex(int index)
{
    if ((index < 0) || (index >= m_slIMLs.Size()))
        return 0;

    SimpleListIterator<CIML*> it = m_slIMLs.GetHead();
    for (int i = -1; it != m_slIMLs.GetEnd(); ++it)
        if ((*it)->GetCachedStatus() == CIML::CACHED)
            if (++i == index)
                return *it;

    return 0;
}

// Returns whether an IML is alive on the network.  May return a false positive.
bool
CIMLManager::IsAvailable(CIML* pIML)
{
#if 0
    // The timeout on transferHTTP is 60s, so try another method
    // Must #include <util/upnp/genlib/http_client.h> to use this section of code
    
    // Request the desc.xml document.  This should be reasonably quick for an fml.    
    
    // Add the 5 just to be safe, cMediaBaseURL should already have the port in it
    int iMediaBaseURLSize = strlen(pIML->GetMediaBaseURL());
    char* cURL = new char[iMediaBaseURLSize + 5 /* 2900/ */ + 8 /* desc.xml */ + 1];
    strcpy(cURL, pIML->GetMediaBaseURL());
    
    char* cPort = strstr(cURL, ":"); // http:
    cPort       = strstr(cPort + 1, ":"); // http://xxx.xxx.xxx.xxx:
    ++cPort;
    *cPort = 0;
    
    strcat(cURL, "2900/desc.xml");

    char* cResponse = 0;
    int   iStatus = transferHTTP("GET", "\r\n", 2, &cResponse, cURL);

    // Don't care about the response
    if (cResponse)
        free(cResponse);

    return (iStatus == HTTP_SUCCESS);

#else
    // This just pings the host, the fml could have died on its own, so this may cause a false
    // positive, which is preferable in this case, since we will catch serious errors later on SetSong.

    if (!pIML)
    {
        return false;
    }
    else
    {
        DBASSERT(IMLMANAGER, (strncmp(pIML->GetMediaBaseURL(), "http://", 7) == 0), "MediaBaseURL not http://");
        char* cAddressStart = (char*)(pIML->GetMediaBaseURL() + 7);
        
        char* cAddressEnd = strchr(cAddressStart, ':' );
        if (!cAddressEnd)
            cAddressEnd = strchr(cAddressStart, '/' );
        DBASSERT(IMLMANAGER, cAddressEnd, "Parsing error on URL %s", pIML->GetMediaBaseURL());
        
        int   iAddressSize = cAddressEnd - cAddressStart;
        char* cAddress     = new char[iAddressSize + 1];
        memcpy(cAddress, cAddressStart, iAddressSize);
        cAddress[iAddressSize] = 0;
        DEBUG(IMLMANAGER, DBGLEV_TRACE, "cAddress %s\n", cAddress);
        
        bool bStatus = CheckRemoteHost(inet_addr(cAddress));
        DEBUG(IMLMANAGER, DBGLEV_TRACE, "bStatus %d\n", bStatus);
        
        delete [] cAddress;
        return bStatus;
    }
#endif    
}

void
CIMLManager::PollUnavailIMLsEntry(cyg_addrword_t Data)
{
    reinterpret_cast<CIMLManager*>(Data)->PollUnavailIMLs();
}

void
CIMLManager::PollUnavailIMLs()
{
    for (;;)
    {
        // 100 ticks/second * 60 seconds/minute * 1 minute = Sleep for 1 minute before checking again
        cyg_thread_delay(100*60*3);

        if( m_pNetDS && m_pNetDS->IsInitialized() ) {
            DEBUGP(IMLMANAGER, DBGLEV_TRACE, "Polling IMLs\n");
            DJUPnPControlPointRefresh(false);
        }
        else {
            DEBUGP(IMLMANAGER, DBGLEV_TRACE, "IML poll thread awoke, but network not available\n");
        }
    }
}
