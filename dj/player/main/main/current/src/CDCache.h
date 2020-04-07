//
// CDCache.h
//
// Copyright (c) 1998 - 2002 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef CDCACHE_H_
#define CDCACHE_H_

#include <util/datastructures/SimpleVector.h>
#include <util/metakit/mk4.h>
#include <util/metakit/mk4str.h>

// fdecl
typedef struct content_record_update_s content_record_update_t;
class IContentManager;

//! Class used for caching disk information for fast lookup of CD metadata.
class CCDCache
{
public:

    //! \param szDBFilename The metadata database file.
    //! \param iMinTrackCount The minimum number of files a data CD must have to be cached.
    //! \param iURLMatchCount The number of URLs to compare before deciding on a match.
    CCDCache(const char* szDBFilename,
            int iMinTrackCount = 20,
            int iURLMatchCount = 10);
    ~CCDCache();

    //! Called after the initial CD content update.
    //! Checks the cache to find a matching CD.
    //! If a match isn't found, then content updates are sent to the content manager
    //! in the chunk size specified.
    int NotifyContentUpdate(content_record_update_t* pContentUpdate, IContentManager* pContentManager, int iUpdateChunkSize);

    //! Called upon receiving the EVENT_METADATA_UPDATE message.
    //! If the CD being read is new to the cache, then the metadata is added to the cache.
    void NotifyMetadataUpdate(content_record_update_t* pContentUpdate);

    //! Called upon receiving the EVENT_METADATA_UPDATE_END message.
    //! If the CD being read is new to the cache, then the cache is committed to disk.
    void NotifyMetadataUpdateEnd();

    //! Called when the CD is ejected.  Aborts any caching in progress.
    void NotifyCDRemoved();

    //! Saves the database to file.
    bool Commit();

    //! Removes all disk info from the cache.
    void Clear();

private:

    c4_Storage  m_mkCDCache;
    c4_View     m_vCDCache;

    const int   mc_iMinTrackCount;    // Don't cache if there are less than this many tracks on the disk.
    const int   mc_iURLMatchCount;    // Number of URLs to match when comparing disks.

    int         m_iRow;                 // Row in the database for the current CD.
    bool        m_bNew;                 // True if the current CD is new to the cache.
    int         m_iTotalTracks;         // Total number of files on the current CD.
    int         m_iCurrentTrackCount;   // Current number of metadata records processed.
    bool        m_bAbortCache;          // True if the CD has been ejected.


};

#endif // CDCACHE_H_
