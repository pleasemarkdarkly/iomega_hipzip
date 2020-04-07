//
// IMLManager.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef IMLMANAGER_H_
#define IMLMANAGER_H_

#include <content/common/QueryableContentManager.h>
#include <main/djupnp/UPnPEvents.h>
#include <util/datastructures/SimpleList.h>
#include <cyg/kernel/kapi.h>

class CIML;
class CNetDataSource;
class IContentManager;

//! The IML manager keeps a list of all IMLs added to the system.
class CIMLManager
{
public:

    //! Returns a pointer to the global IML manager.
    static CIMLManager* GetInstance();

    //! Destroy the singleton global IML manager.
    static void Destroy();

    //! Sets the net data source that will be used for generating media content records.
    void SetNetDataSource(CNetDataSource* pNetDS)
        { m_pNetDS = pNetDS; }

    //! Sets the net data source that will be used for generating media content records.
    void SetContentManager(IContentManager* pContentManager)
        { m_pContentManager = pContentManager; }

    //! Populates a media content record list based on the list of media IDs passed in.
    void CreateMediaRecords(MediaRecordList& records, const IMLMediaInfoVector& mediaKeyValues, const char* szBaseURL);
    
    //! Creates a media content record for the given media item.
    IMediaContentRecord* CreateMediaRecord(const iml_media_info_t& mediaInfo, const char* szBaseURL);
    
    //! Creates a media content record for the given radio stream.
    IMediaContentRecord* CreateMediaRecord(const iml_radio_station_info_t& radioInfo);

    //! Clears all IML media records from the content manager.
    void ClearMediaRecords();
    
    //! Adds a IML to the list.
    void AddIML(CIML* pIML);

    //! Removes a IML from the list.
    void RemoveIML(CIML* pIML);

    //! Mark an IML as unavailable but do not remove it completely
    void SetIMLUnavailable(CIML* pIML);
    
    //! Returns the number of IMLs in the system.
    int GetIMLCount();

    //! Called when an IML is done caching its top-level queries.
    //! The manager will check the list to see if there are any other IMLs that need to be cached.
    //! If so, it will tell one IML to begin caching.
    void NotifyIMLCached(CIML* pIML);

    //! Returns a pointer to a IML with the given device number.
    //! Returns 0 if no matching IML was found.
    CIML* GetIMLByDeviceNumber(int iDeviceNumber);

    //! Returns a pointer to a IML with the given instance ID.
    //! Returns 0 if no matching IML was found.
    CIML* GetIMLByID(int iIMLID);

    //! Returns a pointer to a IML at the given zero-based index.
    //! Returns 0 if the index is out-of-bounds.
    CIML* GetIMLByIndex(int index);

    //! Returns the number of IMLs in the system with fully cached top-level queries.
    int GetCachedIMLCount();

    //! Returns a pointer to a cached IML at the given zero-based index.
    //! Returns 0 if the index is out-of-bounds.
    CIML* GetCachedIMLByIndex(int index);

    //! Returns whether an IML is alive on the network.  May return a false positive.
    bool IsAvailable(CIML* pIML);
    
private:

    CIMLManager();
    ~CIMLManager();

    //! Scans the list of IMLs and starts caching, if a spot is available.
    void UpdateCaching();

    static CIMLManager* s_pSingleton;   // The global singleton IML manager.
    static int s_iNewIMLID;             // The next free instance ID to give to a IML.

    SimpleList<CIML*> m_slIMLs;         // The list of IMLs tracked by the manager.

    static void  PollUnavailIMLsEntry(cyg_addrword_t Data);
    void         PollUnavailIMLs();
    char         m_ThreadStack[4096];
    cyg_handle_t m_hThread;
    cyg_thread   m_ThreadData;
    
    CNetDataSource*  m_pNetDS;  // Used for generating media content records.
    IContentManager* m_pContentManager;  // Used for generating media content records.

    static const int sc_iMaxCaching;    // The maximum number of IMLs to allow to be caching at once.
    int m_iCaching;                     // The number of IMLs currently caching their queries.
};

#endif	// IMLMANAGER_H_
