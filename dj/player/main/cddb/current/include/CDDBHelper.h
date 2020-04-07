//
// CDDBHelper.h
//  Helper functions for accessing CDDB.
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems (TM). All rights reserved
//
//

#ifndef CDDBHELPER_H_
#define CDDBHELPER_H_

#include <extras/cddb/gn_defines.h>
#include <extras/cddb/gn_errors.h>
#include <extras/cddb/gn_tocinfo.h>
#include <extras/cddb/microxml.h>
#include <extras/cdmetadata/DiskInfo.h>

#include <util/datastructures/MessageQueue.h>
#include <util/thread/ThreadedObject.h>

#define CDDB_QUERY_ONLINE
#define CDDB_CACHE_RESULTS


typedef struct cdda_toc_s cdda_toc_t;
class CCDDBDiskInfo;

//! The CCDDBQueryManager is used to query CDDB asynchronously.
class CCDDBQueryManager : private IThreadedObject
{
public:

    //! Returns a pointer to the global cddb query manager.
    static CCDDBQueryManager* GetInstance();

    //! Destroy the singleton global cddb query manager.
    static void Destroy();

    //! Has CDDB been initialized?
    bool IsCDDBInitialized() const
        { return m_bInitialized; }

    //! Initialize the CDDB system.
    gn_error_t InitializeCDDB();

    //! Shut down the CDDB system.
    gn_error_t ShutdownCDDB();

    //! Used for recovery when a major error occurs.
    gn_error_t RecoverCDDB();

    //! Clear the cache.
    gn_error_t ClearCache();

    //! Update the database with an update package.
    //! szUpdateFilename should be only the filename, not the full path.
    gn_error_t UpdateLocal(const char* szUpdateFilename);

    //! Update the database online.
    gn_error_t UpdateOnline();

    //! Update the database online asynchronously.
    //! Returns false if CDDB is not initialized.
    bool UpdateOnlineAsynch();

    //! Resets the db to its original state, undoing all updates.
    //! Used for debugging purposed.
    gn_error_t Reset();

    //! Get disk info for the given TOC from the local cache.
    bool GetDiskInfoCache(const cdda_toc_t* pTOC, DiskInfoVector& svDisks);

    //! Get disk info for the given TOC from the local database.
    //! Returns false if CDDB is not initialized.
    //! \param uiID An identifier that will be passed back with the results.
    bool GetDiskInfoLocalAsynch(const cdda_toc_t* pTOC, unsigned int uiID);

    //! Get disk info for the given TOC from the online database.
    //! Returns false if CDDB is not initialized.
    //! \param uiID An identifier that will be passed back with the results.
    bool GetDiskInfoOnlineAsynch(const cdda_toc_t* pTOC, unsigned int uiID);

    //! Get disk info for the given TOC from the local and online databases.
    //! Returns false if CDDB is not initialized.
    //! \param uiID An identifier that will be passed back with the results.
    bool GetDiskInfoLocalOnlineAsynch(const cdda_toc_t* pTOC, unsigned int uiID);

    //! Adds a disk to the local CDDB cache.
    bool AddDiskToCache(CCDDBDiskInfo* pDiskInfo);

    //! Saves the cache if it is has been updated.
    void SaveCache();

private:

    CCDDBQueryManager(cyg_addrword_t priority, cyg_ucount32 stacksize);
    ~CCDDBQueryManager();

    //! The global singleton cddb query manager.
    static CCDDBQueryManager*   s_pInstance;

    bool    m_bInitialized;     // Has CDDB been initialized?
    bool    m_bCacheDirty;      // True if the cache needs to be saved.

	void ThreadBody();

    // CDDB event queue
    void PutEvent(unsigned int key, void* pData);
    void GetEvent(unsigned int* pKey, void** ppData);

    // Internal event representation
    typedef struct event_s 
    {
        unsigned int key;
        void* data;
    } event_t;

    MessageQueue<event_t> m_messageQueue;

};

//! Get disk info for the given TOC from the local database.
bool CDDBGetDiskInfoLocal(const cdda_toc_t* pTOC, DiskInfoVector& svDisks);

//! Get disk info for the given TOC from the online database.
gn_error_t CDDBGetDiskInfoOnline(const cdda_toc_t* pTOC, DiskInfoVector& svDisks);

class CCDDBDiskInfo : public IDiskInfo
{
public:

	CCDDBDiskInfo(const cdda_toc_t* pTOC, void *info, gn_size_t info_size);
	CCDDBDiskInfo(const cdda_toc_t* pTOC, XMLTag* pXML);
	CCDDBDiskInfo(const cdda_toc_t* pTOC);
	virtual ~CCDDBDiskInfo();

    gn_toc_full_t* GetTOC()
        { return &m_toc; }
    gn_uint32_t GetTOCID() const
        { return m_tocID; }

    void* GetInfo()
        { return m_pInfo; }
    gn_size_t GetInfoSize() const
        { return m_infoSize; }

    void SetRevision(gn_uint32_t revision)
        { m_revision = revision; }
    gn_uint32_t GetRevision() const
        { return m_revision; }

    virtual bool IsPartial() const
        { return false; }

    //! Takes the title, artist, genre, and track metadata from the IDiskInfo base class
    //! and converts it into a cddb info blob.
    //! Used for populating unknown CDs.
    void ConvertMetadataIntoInfo();

protected:

    void ProcessXML(XMLTag* pXML);

    gn_toc_full_t   m_toc;
    gn_uint32_t     m_tocID;
    void*           m_pInfo;
    gn_size_t       m_infoSize;
    gn_uint32_t     m_revision;

};

class CCDDBPartialDiskInfo : public CCDDBDiskInfo
{
public:

	CCDDBPartialDiskInfo(const cdda_toc_t* pTOC, const char* muid, const char* mediaid);
	~CCDDBPartialDiskInfo();

    bool IsPartial() const
        { return m_bPartial; }

    bool RetrieveDiskInfo();

private:

    const char*     m_muid;
    const char*     m_mediaid;
    bool            m_bPartial;

};

//! Gets the latest update level and data revision for CDDB.
gn_error_t CDDBGetUpdateLevel(unsigned short& usUpdateLevel, unsigned short& usDataRevision);



#endif // CDDBHELPER_H_
