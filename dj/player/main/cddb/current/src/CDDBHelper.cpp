//
// CDDBHelper.cpp
//
// Copyright (c) 1998 - 2002 Fullplay Media Systems (TM). All rights reserved
//

#include <main/cddb/CDDBHelper.h>
#include <main/cddb/CDDBEvents.h>

#include <core/events/SystemEvents.h>

#include <datasource/cddatasource/CDDataSource.h>

#include <extras/cddb/gn_cache.h>
#include <extras/cddb/gn_configmgr.h>
#include <extras/cddb/gn_lookup.h>
#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_system.h>
#include <extras/cddb/gn_translator.h>
#include <extras/cddb/gn_upd.h>
#include <extras/cddb/gn_utils.h>
#include <extras/cddb/toc_util.h>

#include <extras/cdmetadata/CDMetadataEvents.h>

#include <fs/fat/sdapi.h>

#include <main/main/AppSettings.h>

#include <util/debug/debug.h>
#include <util/utils/utils.h>
#include <util/eventq/EventQueueAPI.h>

#include <cyg/kernel/kapi.h>

//#include "lookup.h"
#include "updates.h"


DEBUG_MODULE_S(CDDBHELPER, DBGLEV_DEFAULT | DBGLEV_INFO);
//DEBUG_MODULE_S(CDDBHELPER, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE);
DEBUG_USE_MODULE(CDDBHELPER);

#define CDDB_QUERY_THREAD_PRIORITY      12
#define CDDB_QUERY_THREAD_STACK_SIZE    32 * 1024

//! Local mutex to lock CDDB reading/writing.
static cyg_mutex_t s_mtxCDDB;

//! The global singleton cddb query manager.
CCDDBQueryManager* CCDDBQueryManager::s_pInstance = 0;

//! Returns a pointer to the global cddb query manager.
CCDDBQueryManager*
CCDDBQueryManager::GetInstance()
{
    if (!s_pInstance)
        s_pInstance = new CCDDBQueryManager(CDDB_QUERY_THREAD_PRIORITY, CDDB_QUERY_THREAD_STACK_SIZE);
    return s_pInstance;
}

//! Destroy the singleton global cddb query manager.
void
CCDDBQueryManager::Destroy()
{
    if (s_pInstance)
    {
        delete s_pInstance;
        s_pInstance = 0;
    }
}

#define CDDB_EVENT_QUIT                 1
#define CDDB_EVENT_QUERY_LOCAL          2
#define CDDB_EVENT_QUERY_ONLINE         3
#define CDDB_EVENT_QUERY_LOCAL_ONLINE   4
#define CDDB_EVENT_UPDATE_ONLINE        5


CCDDBQueryManager::CCDDBQueryManager(cyg_addrword_t priority, cyg_ucount32 stacksize)
    : IThreadedObject(priority, "CDDBQueryThread", stacksize),
    m_bInitialized(false),
    m_bCacheDirty(false)
{
    Start();
}

CCDDBQueryManager::~CCDDBQueryManager()
{
    m_bStopped = false;
    
    cyg_mutex_destroy( &s_mtxCDDB );
    PutEvent( CDDB_EVENT_QUIT, 0 );

    // Manually wait here, otherwise the destructor on MessageQueue will be called prior
    //  to the event being handled, and we'll choke
    while( !m_bStopped ) {
        cyg_thread_delay( 1 );
    }
}


gn_error_t
CCDDBQueryManager::InitializeCDDB()
{
    if (m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "CDDB is already initialized\n");
        return SUCCESS;
    }

    cyg_mutex_init(&s_mtxCDDB);

    gnconf_set_str(ONLINE_LOOKUP_URL, "http://ecddb.gracenote.com:8080");
    gnconf_set_str(ONLINE_UPDATE_URL, "http://ecddb.gracenote.com:8080");

    gn_error_t  error = GNERR_NoError;
    int         gEncoding = 0;
    gn_uchar_t* heap = NULL;
    gn_size_t   heap_size = 0;
    int         tries = 0;

//retry:

#ifndef GN_NO_LOGGING
    error = gnlog_init(NULL);
#endif

    error = gnmem_initialize(heap,heap_size);
    if ((error != SUCCESS) && (error != MEMERR_Busy))
        goto cleanup;

    error = gnsys_initialize();

#if !defined(GN_NO_ONLINE)
    /* if ONLINE_LOOKUP_ENCODING is set, set gEncoding for convenience */
    if (error == SUCCESS)
    {
        gn_char_t       online_lookup_encoding[DESCSIZE];

        error = gnconf_get(ONLINE_LOOKUP_ENCODING, online_lookup_encoding);
        if (error == SUCCESS && !strcmp(online_lookup_encoding, "base64"))
            gEncoding = 1;
        else
            gEncoding = 0;
        error = SUCCESS;    /* it's OK if this setting is not in the config file */
    }
#endif /* #if !defined(GN_NO_ONLINE) */

cleanup:
    if (error != SUCCESS)
    {
        gn_str_t    error_message = GN_NULL;

//        bool bDeleteCache = (GNERR_PKG_ID(error) == GNPKG_EmbeddedDBIM) && (GNERR_ERROR_CODE(error) == GNERR_NoBackupCache);
        bool bDeleteCache = false;

        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error 0x%x occurred while initializing eCDDB.\n", error);
        error_message = gnerr_get_error_message(error);
        if (error_message != GN_NULL)
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "%s\n\n",error_message);

        if (tries++ < 5)
        {
#if 0
            ShutdownCDDB();
            error = gnsys_recover();
            if (error != SUCCESS)
                DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error recovering: 0x%x: %s\n", error, gnerr_get_error_message(error));
            else
            {
                DEBUG(CDDBHELPER, DBGLEV_ERROR, "Recovery successful\n");
            }
#endif

            if (bDeleteCache)
            {
                ShutdownCDDB();
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleting cache\n");
                if (!pc_unlink(CDDB_CACHE_FILE))
                {
                    DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unable to delete cache file %s\n", CDDB_CACHE_FILE);
                    error = gnsys_reset_db();
                }
                else
                    DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleted cache file %s\n", CDDB_CACHE_FILE);
                if (!pc_unlink(CDDB_CACHE_BACKUP_FILE))
                    DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unable to delete cache backup file %s\n", CDDB_CACHE_BACKUP_FILE);
                else
                    DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleted cache backup file %s\n", CDDB_CACHE_BACKUP_FILE);
            }
#if 1
            error = RecoverCDDB();
            goto cleanup;
#else
            goto retry;
#endif
        }
        else
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "Giving up on initialization\n");
    }
    else
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "eCDDB is initialized\n\n");
        m_bInitialized = true;
    }

    return error;
}


//! Shut down the CDDB system.
gn_error_t
CCDDBQueryManager::ShutdownCDDB()
{
	gn_error_t	error = GNERR_NoError;
	gn_str_t	error_message = GN_NULL;

	error = gnsys_shutdown();
	if (error != SUCCESS)
	{
		DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error 0x%x occurred while shutting down eCDDB.\n", error);
		error_message = gnerr_get_error_message(error);
        if (error_message != GN_NULL)
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "%s\n\n",error_message);
	}
	error = gnmem_shutdown();
	if (error != SUCCESS)
	{
		DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error 0x%x occurred while shutting down memory.\n", error);
		error_message = gnerr_get_error_message(error);
		if (error_message != GN_NULL)
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "%s\n\n",error_message);
	}
	else
		DEBUG(CDDBHELPER, DBGLEV_INFO, "eCDDB is not initialized\n\n");

#ifndef GN_NO_LOGGING
	gnlog_shutdown();
#endif

    m_bInitialized = false;

    cyg_mutex_destroy(&s_mtxCDDB);

	return error;
}


//! Used for recovery when an system error occurs.
gn_error_t
CCDDBQueryManager::RecoverCDDB()
{
    gn_error_t      error = GNERR_NoError;

    error = gnsys_shutdown();
    if (error == SYSERR_NotInited)
        error = SUCCESS;

    if (error == SUCCESS)
    {
#ifndef GN_NO_LOGGING
        error = gnlog_init(NULL);
#endif

        gn_uchar_t* heap = NULL;
        gn_size_t   heap_size = 0;
        error = gnmem_initialize(heap, heap_size);

        if ((error != SUCCESS) && (error != MEMERR_Busy))
            goto cleanup;

        error = gnsys_recover();
    }

    if (error == SUCCESS)
        error = gnsys_initialize();

#if !defined(GN_NO_UPDATES)
/*
    if (argc)
    {
        gn_upd_init();
        gn_upd_cleanup(argv[0]);
        gn_upd_shutdown();
    }
*/
#endif

cleanup:
    DEBUG(CDDBHELPER, DBGLEV_INFO, "RecoverCDDB returning %x\n", error);
    return error;
}


//! Clear the cache.
gn_error_t
CCDDBQueryManager::ClearCache()
{
    bool bInitialized = m_bInitialized;

    if (bInitialized)
        ShutdownCDDB();

    DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleting cache\n");
    if (!pc_unlink(CDDB_CACHE_FILE))
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unable to delete cache file %s\n", CDDB_CACHE_FILE);
    else
        DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleted cache file %s\n", CDDB_CACHE_FILE);
    if (!pc_unlink(CDDB_CACHE_BACKUP_FILE))
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unable to delete cache backup file %s\n", CDDB_CACHE_BACKUP_FILE);
    else
        DEBUG(CDDBHELPER, DBGLEV_INFO, "Deleted cache backup file %s\n", CDDB_CACHE_BACKUP_FILE);

    if (bInitialized)
        return InitializeCDDB();
    else
        return SUCCESS;
}

//! Update the database with an update package.
//! szUpdateFilename should be only the filename, not the full path.
gn_error_t
CCDDBQueryManager::UpdateLocal(const char* szUpdateFilename)
{
    DEBUG(CDDBHELPER, DBGLEV_TRACE, "Update local: filename: %s\n", szUpdateFilename);

    if (!m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error updating local: CDDB not initialized\n");
        return SYSERR_NotInited;
    }

    cyg_mutex_lock(&s_mtxCDDB);

    gn_error_t error = gn_upd_init();
    if (error == 0)
    {
        error = gn_upd_update(szUpdateFilename);
    }
    
    if (error)
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error updating local from %s: 0x%x: %s\n", szUpdateFilename, error, gnerr_get_error_message(error));

    gn_upd_shutdown();

    cyg_mutex_unlock(&s_mtxCDDB);

    return error;
}


//! Update the database online.
gn_error_t
CCDDBQueryManager::UpdateOnline()
{
    cyg_mutex_lock(&s_mtxCDDB);

    gn_error_t error = do_online_update();

    cyg_mutex_unlock(&s_mtxCDDB);

    return error;
}

//! Update the database online asynchronously.
//! Returns false if CDDB is not initialized.
bool
CCDDBQueryManager::UpdateOnlineAsynch()
{
    if (!m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "CDDB not initialized\n");
        return false;
    }

    PutEvent(CDDB_EVENT_UPDATE_ONLINE, (void*)0);

    return true;
}



//! Resets the db to its original state, undoing all updates.
//! Used for debugging purposes.
gn_error_t
CCDDBQueryManager::Reset()
{
    DEBUG(CDDBHELPER, DBGLEV_TRACE, "Resetting CDDB db\n");

    cyg_mutex_lock(&s_mtxCDDB);

    gn_error_t error = gnsys_reset_db();
    if (error)
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error resetting db: 0x%x: %s\n", error, gnerr_get_error_message(error));

    cyg_mutex_unlock(&s_mtxCDDB);

    return error;
}

// Converts a TOC from the CD data source to Gracenote's TOC format.
// pCDDBTOC->offsets should be deleted when done.
static void
CDDBPopulateTOC(const cdda_toc_t* pSDKTOC, gn_toc_info_t* pCDDBTOC)
{
    pCDDBTOC->num_offsets = pSDKTOC->entries + 1;
    pCDDBTOC->offsets = new gn_uint32_t[pCDDBTOC->num_offsets];

//    DEBUGP(CDDBHELPER, DBGLEV_INFO, "aTOC: ");
    for (int i = 0; i < pSDKTOC->entries; ++i)
    {
        pCDDBTOC->offsets[i] = pSDKTOC->entry_list[i].lba_startsector + 150;
//        DEBUGP(CDDBHELPER, DBGLEV_INFO, "%d ", pCDDBTOC->offsets[i]);
    }
    pCDDBTOC->offsets[pSDKTOC->entries] = pSDKTOC->entry_list[pSDKTOC->entries - 1].lba_startsector + pSDKTOC->entry_list[pSDKTOC->entries - 1].lba_length + 150;
//    DEBUGP(CDDBHELPER, DBGLEV_INFO, "%d\n", pCDDBTOC->offsets[pSDKTOC->entries]);
}

static void
CDDBDiskInfoToDiskInfo(gn_discinfo_t* disc_info, CCDDBDiskInfo* pDiskInfo)
{
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Revision: %d\n", disc_info->di_revision);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Title: %s\n", disc_info->di_title);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Artist: %s\n", disc_info->di_artist);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Genre: %s\n", disc_info->di_genre);

    pDiskInfo->SetTitle(disc_info->di_title);
    pDiskInfo->SetArtist(disc_info->di_artist ? disc_info->di_artist : "Unknown artist");
    pDiskInfo->SetGenre(disc_info->di_genre ? disc_info->di_genre : "Unknown genre");
    pDiskInfo->SetRevision(disc_info->di_revision);

    for (unsigned int j = 0; j < disc_info->di_numtracks; ++j)
    {
        DEBUGP(CDDBHELPER, DBGLEV_INFO, "Track %d: %s\n", j + 1, disc_info->di_tracktitles[j]);
        CTrackInfo* pTrackInfo = new CTrackInfo;
        pTrackInfo->SetTrackName(disc_info->di_tracktitles[j]);
        pDiskInfo->AddTrack(pTrackInfo);
    }
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "\n");
}

static gn_xlt_error_t
CDDBInfoToDiskInfo(const cdda_toc_t* pTOC, void *info, gn_size_t info_size, DiskInfoVector& svDisks)
{
    gn_discinfo_t*  disc_info = NULL;
    gn_xlt_error_t xlate_err = gnxlt_convert_alloc(info, info_size, &disc_info);

    if (xlate_err == XLTERR_NoError)
    {
        CCDDBDiskInfo* pDiskInfo = new CCDDBDiskInfo(pTOC, info, info_size);
        CDDBDiskInfoToDiskInfo(disc_info, pDiskInfo);
        svDisks.PushBack(pDiskInfo);

        // Release these
        gnxlt_release(disc_info);
        gnmem_free(disc_info);
    }
    else
    {
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error translating data: 0x%x: %s\n", xlate_err, gnerr_get_error_message(xlate_err));
    }

    return xlate_err;
}

static void
CDDBResultsToDiskInfo(const cdda_toc_t* pTOC, gn_tlu_result_t **results, gn_uint32_t nresults, DiskInfoVector& svDisks)
{
    for (unsigned int i = 0; i < nresults; ++i)
    {
        DEBUGP(CDDBHELPER, DBGLEV_INFO, "Match %d:\n", i + 1);
        void*       info = NULL;
        gn_size_t   info_size;
        gn_error_t lookup_err = gntlu_result_info_alloc(results[i], &info, &info_size);

        if (lookup_err == SUCCESS)
        {
#if DEBUG_LEVEL != 0
            XMLTagRef xml = 0;
            ParseBufToXMLTag((const char*)info, info_size, &xml);
            gn_str_t raw_xml = RenderXMLTagToStrEx(xml, GN_FALSE, GN_TRUE, GN_TRUE);
            if (raw_xml)
            {
                DEBUG(CDDBHELPER, DBGLEV_TRACE, "Raw XML:\n");
                DEBUGP(CDDBHELPER, DBGLEV_TRACE, "%s\n", raw_xml);
                DEBUG(CDDBHELPER, DBGLEV_TRACE, "End XML\n");
                gnmem_free(raw_xml);
            }
            /* release the XML text */
            DisposeXMLTag(xml);
#endif  // DEBUG_LEVEL

            CDDBInfoToDiskInfo(pTOC, info, info_size, svDisks);

            /* and release these */
            gnmem_free(info);     // Store in CCDDBDiskInfo
        }
        else
        {
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "Results to info conversion error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
        }
    }
}

/* convert gn_toc_info_t structure into gn_toc_full_t for cache lookups */
static int
convert_toc_info_to_full(const gn_toc_info_t* toc, gn_toc_full_t* toc_full)
{
	toc_full->num_offsets = toc->num_offsets;
	gnmem_memcpy(&toc_full->offsets[0], toc->offsets, toc->num_offsets * sizeof(toc_full->offsets[0]));
	return 0;
}


// Get disk info for the given TOC from the local cache.
bool
CCDDBQueryManager::GetDiskInfoCache(const cdda_toc_t* pTOC, DiskInfoVector& svDisks)
{
    cyg_mutex_lock(&s_mtxCDDB);

    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);

    // First, check the cache.
    // Convert toc_info into cache storage format
    gn_toc_full_t       toc_full;
    gn_error_t          lookup_err;
    gn_cache_data_t     cookie = NULL;

    convert_toc_info_to_full(&toc, &toc_full);

    lookup_err = gncache_lookup_toc((gn_toc_hdr_t*)&toc_full, &cookie);
    if (lookup_err == SUCCESS)
    {
        gn_cache_rec_type_t type;
		lookup_err = gncache_data_type(cookie, &type);

        if (lookup_err == SUCCESS)
        {
            if (type == CACHE_TYPE_TOC)
            {
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Record found in cache, TOC-only\n");
            }
            else if (type == CACHE_TYPE_TOC_ID)
            {
                gn_tlu_match_t      match = 0;
                gn_tlu_result_t     **results = NULL;
                gn_uint32_t         nresults = 0;

                gn_uint32_t toc_id = 0;
                gncache_data_toc_id(cookie, &toc_id);
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Record found in cache, toc_id == %d\n", toc_id);

                lookup_err = gntlu_lookup(&toc, toc_id, TLOPT_Default, &match, &results, &nresults);
                if (lookup_err == TLERR_NoError)
                {
                    CDDBResultsToDiskInfo(pTOC, results, nresults, svDisks);

                    /* free the search results now that we have the metadata */
                    gntlu_release(results, nresults);
                }
                else
                {
                    DEBUG(CDDBHELPER, DBGLEV_ERROR, "Unexpected result looking up toc ID: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
                }
            }
            else if (type == CACHE_TYPE_DATA)
            {
                void*       info = NULL;
                gn_size_t   info_size = 0;

                lookup_err = gncache_retrieve_data_alloc(cookie, &info, &info_size);

                if (lookup_err == XLTERR_NoError)
                {
                    CDDBInfoToDiskInfo(pTOC, info, info_size, svDisks);
                    gnmem_free(info);     // Store in CCDDBDiskInfo
                }
                else
                {
                    DEBUG(CDDBHELPER, DBGLEV_ERROR, "Results to info conversion error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
                }
            }
            else
            {
                DEBUG(CDDBHELPER, DBGLEV_WARNING, "Record found in cache, unknown type: %d\n", type);
            }
        }
        gncache_release_data(cookie);
    }
    else
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "TOC not found in local cache\n");
    }

    cyg_mutex_unlock(&s_mtxCDDB);

    return true;
}

//! Creates a copy of the given SDK TOC.
//! The copy should be deleted by first deleting [] the entry list, then deleting the TOC itself.
static cdda_toc_t*
CopySdkToc(const cdda_toc_t* pTOC)
{
    cdda_toc_t* pCopy = new cdda_toc_t;
    DBASSERT(CDDBHELPER, pCopy, "Unable to copy TOC\n");

    pCopy->entries = pTOC->entries;
    pCopy->entry_list = new cdda_toc_entry_t[pCopy->entries];
    DBASSERT(CDDBHELPER, pCopy->entry_list, "Unable to copy TOC\n");

    memcpy((void*)pCopy->entry_list, (void*)pTOC->entry_list, pCopy->entries * sizeof(cdda_toc_entry_t));

    return pCopy;
}

static void
DeleteSdkToc(cdda_toc_t* pTOC)
{
    delete [] pTOC->entry_list;
    delete pTOC;
}


// Get disk info for the given TOC from the local database.
bool
CDDBGetDiskInfoLocal(const cdda_toc_t* pTOC, DiskInfoVector& svDisks)
{
    cyg_mutex_lock(&s_mtxCDDB);

    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);

	/* perform the lookup */
    gn_tlu_result_t     **results;
    gn_tlu_match_t      match;
    gn_uint32_t         nresults = 0;
//    gn_tlu_options_t    lu_options = TLOPT_Default | TLOPT_AutoFuzzy;
    gn_tlu_options_t    lu_options = TLOPT_Default;

    // Check the local database.
    gn_error_t lookup_err = gntlu_lookup(&toc, 0, lu_options, &match, &results, &nresults);

	if (lookup_err == TLERR_NoError && nresults > 0)
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "%d results found\n", nresults);

        CDDBResultsToDiskInfo(pTOC, results, nresults, svDisks);

		/* free the search results now that we have the metadata */
		gntlu_release(results, nresults);

        /* eliminate duplicates */
        for (int i = 0; i + 1 < svDisks.Size(); ++i)
        {
            int j = i + 1;
            while (j < svDisks.Size())
            {
                if (!tstrcmp(svDisks[i]->GetTitle(), svDisks[j]->GetTitle()) &&
                    !tstrcmp(svDisks[i]->GetArtist(), svDisks[j]->GetArtist()))
                {
                    if (((CCDDBDiskInfo*)svDisks[i])->GetRevision() > ((CCDDBDiskInfo*)svDisks[j])->GetRevision())
                    {
                        delete svDisks[j];
                    }
                    else
                    {
                        delete svDisks[i];
                        svDisks[i] = svDisks[j];
                    }
                    svDisks.Remove(j);
                }
                else
                    ++j;
            }
        }
    }
    else if (lookup_err)
    {
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Local lookup error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
    }
    else
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "No match found for TOC locally\n");
    }

    delete [] toc.offsets;

    cyg_mutex_unlock(&s_mtxCDDB);

    return true;
}

static void
CDDBSDiskInfoToDiskInfo(gn_sdiscinfo_t* disc_info, CCDDBDiskInfo* pDiskInfo)
{
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Revision: %d\n", disc_info->di_revision);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Title: %s\n", disc_info->di_title);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Artist: %s\n", disc_info->di_artist);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "Genre: %s\n", disc_info->di_genre);
    DEBUGP(CDDBHELPER, DBGLEV_INFO, "TOC ID: %x\n", disc_info->di_toc_id);

    pDiskInfo->SetTitle(disc_info->di_title);
    pDiskInfo->SetArtist(disc_info->di_artist ? disc_info->di_artist : "Unknown artist");
    pDiskInfo->SetGenre(disc_info->di_genre ? disc_info->di_genre : "Unknown genre");
    pDiskInfo->SetRevision(disc_info->di_revision);

    DEBUGP(CDDBHELPER, DBGLEV_INFO, "\n");
}


#define			FUZZY_CHOICES		50

static gn_error_t
CDDBMultiXMLToDiskInfo(const cdda_toc_t* pTOC, XMLTagRef xml, DiskInfoVector& svDisks)
{
    /* multiple matches are containing in an ALBS element */
    if (strcmp(GetXMLTagName(xml), GN_STR_ALBUMS))
        return kXMLInvalidParamError;

    /* first extract MATCHES attribute from ALBS tag */
    gn_cstr_t match_str = GetXMLTagAttrFromStr(xml, GN_STR_MATCHES);
    if (match_str == NULL)
        return kXMLInvalidParamError;

    gn_uint32_t matchcnt = parse_digits_to_uint32(match_str, strlen(match_str), GN_FALSE);
    if (matchcnt <= 0)
        return kXMLInvalidParamError;

    /* get the subtags for the ALB elements */
    XMLTagRef   alb_tags[FUZZY_CHOICES];
    for (unsigned int n = 0; n < matchcnt && n < FUZZY_CHOICES; n++) {
        alb_tags[n] = GetXMLSubTag(xml, n);
        if (alb_tags[n] == NULL) {
            if (n == 0)
                return kXMLInvalidParamError;
            matchcnt = n;
            break;
        }
    }

    gn_sdiscinfo_t  *matches[FUZZY_CHOICES];
    for (int n = 0; n < FUZZY_CHOICES; n++)
        matches[n] = NULL;

    /* convert the ALB elements to the sdiscinfo structs */
    gn_error_t  xlate_err = SUCCESS;
    unsigned int n;
    for (n = 0; n < matchcnt && n < FUZZY_CHOICES; n++)
    {
        xlate_err = gnxlt_sconvert_xml_alloc(alb_tags[n], &matches[n]);
        if (xlate_err)
            break;
        else
        {
            if (strcmp(GetXMLTagName(alb_tags[n]), GN_STR_ALBUM))
                continue;

            XMLTagRef discid_tag = GetXMLSubTagFromStr(alb_tags[n], GN_STR_DISCID);

            /* extract toc_id(MUID) and media_id(MEDIAID) from XML */
            if (discid_tag)
            {
                XMLTagRef muid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MUID);
                XMLTagRef mediaid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MEDIAID);
                if (muid_tag == 0 || mediaid_tag == 0)
                    continue;

                CCDDBPartialDiskInfo* pDiskInfo = new CCDDBPartialDiskInfo(pTOC, GetXMLTagData(muid_tag), GetXMLTagData(mediaid_tag));
                CDDBSDiskInfoToDiskInfo(matches[n], pDiskInfo);
                svDisks.PushBack(pDiskInfo);
            }
        }
    }

    /* clean up on error */
    if (xlate_err != SUCCESS) {
        /* free the short info structs */
        if (n > 0) {
            while (n-- >= 0)
                gnxlt_srelease_free(matches[n]);
        }

        return xlate_err;
    }

#if 0
    /* let the user choose */
    choice = get_selection(&matches[0], n);

    /* set the appropriate selection */
    if (choice == 0)
        *xml_choice = NULL;
    else
        *xml_choice = alb_tags[choice - 1];
#endif

    /* free intermediate selection data */
    for (n = 0; n < matchcnt && n < FUZZY_CHOICES; n++) {
        gnxlt_srelease_free(matches[n]);
    }
    return SUCCESS;
}


// Get disk info for the given TOC from the online database.
gn_error_t
CDDBGetDiskInfoOnline(const cdda_toc_t* pTOC, DiskInfoVector& svDisks)
{
    cyg_mutex_lock(&s_mtxCDDB);

    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);

    XMLTagRef           xml = 0;
    gn_tlu_match_t      match_code = TLMATCH_None;
    gn_tlu_options_t    lu_options = TLOPT_Default;

	gn_error_t lookup_err = gntlu_lookup_online_ex(&toc, lu_options, 0, &xml, &match_code);

    if (lookup_err == 0 && xml != 0)
    {
#if DEBUG_LEVEL != 0
        gn_str_t raw_xml = RenderXMLTagToStrEx(xml, GN_FALSE, GN_TRUE, GN_TRUE);
        if (raw_xml)
        {
            DEBUG(CDDBHELPER, DBGLEV_TRACE, "Raw XML:\n");
            DEBUGP(CDDBHELPER, DBGLEV_TRACE, "%s\n", raw_xml);
            DEBUG(CDDBHELPER, DBGLEV_TRACE, "End XML\n");
            gnmem_free(raw_xml);
        }
#endif  // DEBUG_LEVEL

        if (match_code == TLMATCH_Fuzzy) // Multiple matches
        {
            CDDBMultiXMLToDiskInfo(pTOC, xml, svDisks);
        }
        else    // Single match
        {
            /* convert XML data to disc info structure */
            gn_discinfo_t*  disc_info = 0;
            lookup_err = gnxlt_convert_xml_alloc(xml, &disc_info);
            if (lookup_err == SUCCESS)
            {
                CCDDBDiskInfo* pDiskInfo = new CCDDBDiskInfo(pTOC, xml);
                CDDBDiskInfoToDiskInfo(disc_info, pDiskInfo);
                svDisks.PushBack(pDiskInfo);

			    gnxlt_release(disc_info);
			    gnmem_free((void*)disc_info);
            }
            else
                DEBUG(CDDBHELPER, DBGLEV_ERROR, "XML result translation error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
        }

        /* release the XML text */
        DisposeXMLTag(xml);
    }
    else if (lookup_err)
    {
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Online lookup error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
    }
    else if (match_code == TLMATCH_None)
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "No match found for TOC online\n");
    }

    delete [] toc.offsets;

    cyg_mutex_unlock(&s_mtxCDDB);

    return lookup_err;
}

extern "C"
{
extern int dump_cache(int verbose);
}

// Adds a disk to the local CDDB cache.
bool
CCDDBQueryManager::AddDiskToCache(CCDDBDiskInfo* pDiskInfo)
{
    DEBUG(CDDBHELPER, DBGLEV_INFO, "Adding disk to CDDB cache\n");
    gn_error_t our_err = SUCCESS;

    cyg_mutex_lock(&s_mtxCDDB);

    if (!pDiskInfo->IsPartial())
    {
        our_err = gncache_add_toc_data((gn_toc_hdr_t*)pDiskInfo->GetTOC(), pDiskInfo->GetTOCID(), pDiskInfo->GetInfo(), pDiskInfo->GetInfoSize());
        if (our_err)
        {
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "gncache_add_toc_data error: 0x%x: %s\n", our_err, gnerr_get_error_message(our_err));
        }
        else
        {
            // Mark the cache as dirty so it can be saved later.
            m_bCacheDirty = true;
        }

//        dump_cache(1);
    }
    else
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unable to add partial disk info to CDDB cache\n");

    cyg_mutex_unlock(&s_mtxCDDB);

    return our_err == SUCCESS;
}

//! Saves the cache if it is has been updated.
void
CCDDBQueryManager::SaveCache()
{
    if (m_bCacheDirty)
    {
        DEBUG(CDDBHELPER, DBGLEV_INFO, "Saving the CDDB cache\n");

        cyg_mutex_lock(&s_mtxCDDB);

        // Save the cache by shutting it down and restarting.
        gncache_shutdown();
        gncache_initialize(CACHE_OPT_Default);

        // No more dirt.
        m_bCacheDirty = false;

        cyg_mutex_unlock(&s_mtxCDDB);

    }
}


typedef struct cddb_query_request_info_s
{
    cdda_toc_t*     pTOC;
    unsigned int    uiID;

    cddb_query_request_info_s(cdda_toc_t* theTOC, unsigned int theID)
        : pTOC(theTOC), uiID(theID)
        { }

} cddb_query_request_info_t;



//! Get disk info for the given TOC from the local database.
//! Returns false if CDDB is not initialized.
bool
CCDDBQueryManager::GetDiskInfoLocalAsynch(const cdda_toc_t* pTOC, unsigned int uiID)
{
    if (!m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "CDDB not initialized\n");
        return false;
    }

    cdda_toc_t* pTocCopy = CopySdkToc(pTOC);
    PutEvent(CDDB_EVENT_QUERY_LOCAL, (void*)new cddb_query_request_info_t(pTocCopy, uiID));

    return true;
}


//! Get disk info for the given TOC from the online database.
//! Returns false if CDDB is not initialized.
bool
CCDDBQueryManager::GetDiskInfoOnlineAsynch(const cdda_toc_t* pTOC, unsigned int uiID)
{
    if (!m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "CDDB not initialized\n");
        return false;
    }

    cdda_toc_t* pTocCopy = CopySdkToc(pTOC);
    PutEvent(CDDB_EVENT_QUERY_ONLINE, (void*)new cddb_query_request_info_t(pTocCopy, uiID));

    return true;
}

//! Get disk info for the given TOC from the local and online databases.
//! Returns false if CDDB is not initialized.
bool
CCDDBQueryManager::GetDiskInfoLocalOnlineAsynch(const cdda_toc_t* pTOC, unsigned int uiID)
{
    if (!m_bInitialized)
    {
        DEBUG(CDDBHELPER, DBGLEV_WARNING, "CDDB not initialized\n");
        return false;
    }

    cdda_toc_t* pTocCopy = CopySdkToc(pTOC);
    PutEvent(CDDB_EVENT_QUERY_LOCAL_ONLINE, (void*)new cddb_query_request_info_t(pTocCopy, uiID));

    return true;
}


void
CCDDBQueryManager::ThreadBody()
{
    DEBUG(CDDBHELPER, DBGLEV_INFO, "Worker thread starting\n");

    unsigned int key;
    do
    {
        void* data;
        GetEvent(&key, &data);

        switch (key)
        {
            case CDDB_EVENT_QUIT:
                DEBUG(CDDBHELPER, DBGLEV_WARNING, "Quitting\n");
                break;

            case CDDB_EVENT_QUERY_LOCAL:
            {
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Local query\n");
                cddb_query_request_info_t* pQueryRequest = (cddb_query_request_info_t*)data;
                DiskInfoVector svDisks;
                CDDBGetDiskInfoLocal(pQueryRequest->pTOC, svDisks);
                if (svDisks.IsEmpty())
                {
                    // Send an event if there are no hits for this CD's freedb ID.
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)pQueryRequest->uiID);
                }
                else
                {
                    // Send an event if there are multiple hits for this CD's freedb ID.
                    cd_multiple_hit_event_t* pEvent = new cd_multiple_hit_event_t;
                    pEvent->iDataSourceID = 0;
                    pEvent->uiDiskID = pQueryRequest->uiID;
                    for (int i = 0; i < svDisks.Size(); ++i)
                        pEvent->svDisks.PushBack(svDisks[i]);
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_MULTIPLE_HITS, (void*)pEvent);
                }
                DeleteSdkToc(pQueryRequest->pTOC);
                delete pQueryRequest;
                break;
            }

            case CDDB_EVENT_QUERY_ONLINE:
            {
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Online query\n");
                cddb_query_request_info_t* pQueryRequest = (cddb_query_request_info_t*)data;
                DiskInfoVector svDisks;
                gn_error_t lookup_err = CDDBGetDiskInfoOnline(pQueryRequest->pTOC, svDisks);
                if (svDisks.IsEmpty())
                {
                    // Send an event if there are no hits for this CD's freedb ID.
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)pQueryRequest->uiID);
                }
                else
                {
                    // Send an event if there are multiple hits for this CD's freedb ID.
                    cd_multiple_hit_event_t* pEvent = new cd_multiple_hit_event_t;
                    pEvent->iDataSourceID = 0;
                    pEvent->uiDiskID = pQueryRequest->uiID;
                    for (int i = 0; i < svDisks.Size(); ++i)
                        pEvent->svDisks.PushBack(svDisks[i]);
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_MULTIPLE_HITS, (void*)pEvent);
                }
                // Inform the interface if there was an error.
                if (lookup_err)
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_ONLINE_LOOKUP_ERROR, (void*)lookup_err);

                DeleteSdkToc(pQueryRequest->pTOC);
                delete pQueryRequest;
                break;
            }

            case CDDB_EVENT_QUERY_LOCAL_ONLINE:
            {
                DEBUG(CDDBHELPER, DBGLEV_INFO, "Local and online query\n");
                cddb_query_request_info_t* pQueryRequest = (cddb_query_request_info_t*)data;
                DiskInfoVector svDisks;
                CDDBGetDiskInfoLocal(pQueryRequest->pTOC, svDisks);
                CDDBGetDiskInfoOnline(pQueryRequest->pTOC, svDisks);
                if (svDisks.IsEmpty())
                {
                    // Send an event if there are no hits for this CD's freedb ID.
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_NO_HITS, (void*)pQueryRequest->uiID);
                }
                else
                {
                    // Send an event if there are multiple hits for this CD's freedb ID.
                    cd_multiple_hit_event_t* pEvent = new cd_multiple_hit_event_t;
                    pEvent->iDataSourceID = 0;
                    pEvent->uiDiskID = pQueryRequest->uiID;
                    for (int i = 0; i < svDisks.Size(); ++i)
                        pEvent->svDisks.PushBack(svDisks[i]);
                    CEventQueue::GetInstance()->PutEvent(EVENT_CD_METADATA_MULTIPLE_HITS, (void*)pEvent);
                }
                DeleteSdkToc(pQueryRequest->pTOC);
                delete pQueryRequest;
                break;
            }

            case CDDB_EVENT_UPDATE_ONLINE:
            {
                CEventQueue::GetInstance()->PutEvent(EVENT_CDDB_ONLINE_UPDATE_BEGIN, (void*)0);
                gn_error_t update_err = UpdateOnline();
                if (update_err)
                    CEventQueue::GetInstance()->PutEvent(EVENT_CDDB_ONLINE_UPDATE_ERROR, (void*)update_err);
                else
                    CEventQueue::GetInstance()->PutEvent(EVENT_CDDB_ONLINE_UPDATE_END, (void*)0);
                break;
            }

            default:
                DEBUG(CDDBHELPER, DBGLEV_WARNING, "Unknown event: %d\n", key);
                break;
        }

    } while (key != CDDB_EVENT_QUIT);
}

void
CCDDBQueryManager::PutEvent( unsigned int key, void* pData )
{
    event_t event;
    event.key = key;
    event.data = pData;

    m_messageQueue.Put(event);
}

void
CCDDBQueryManager::GetEvent(unsigned int* pKey, void** ppData )
{
    event_t event;

    m_messageQueue.Get(&event);
    
    *pKey = event.key;
    *ppData = event.data;
}
















CCDDBDiskInfo::CCDDBDiskInfo(const cdda_toc_t* pTOC, void *info, gn_size_t info_size)
    : m_tocID(2),
    m_pInfo(info),
    m_infoSize(info_size)
{
    // Convert the SDK TOC into a CDDB TOC.
    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);
    convert_toc_info_to_full(&toc, &m_toc);
    delete [] toc.offsets;

//diag_printf("#### Info:\n%s\n\nSize: %d Strlen: %d\n#####\n", (char*)info, info_size, strlen((char*)info));

    // Reduce the amount of data being stored in the cache
    gn_error_t xlate_err = gnxlt_reduce_raw(info, info_size, &m_pInfo, &m_infoSize, 0);
    if (xlate_err != SUCCESS)
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "XML result translation error: 0x%x: %s\n", xlate_err, gnerr_get_error_message(xlate_err));

//diag_printf("#### Raw:\n%s\n\nSize: %d Strlen: %d\n#####\n", (char*)m_pInfo, m_infoSize, strlen((char*)m_pInfo));
}


CCDDBDiskInfo::CCDDBDiskInfo(const cdda_toc_t* pTOC, XMLTag* pXML)
    : m_pInfo(0),
    m_infoSize(0)
{
    // Convert the SDK TOC into a CDDB TOC.
    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);
    convert_toc_info_to_full(&toc, &m_toc);
    delete [] toc.offsets;

    ProcessXML(pXML);
}


CCDDBDiskInfo::CCDDBDiskInfo(const cdda_toc_t* pTOC)
    : m_tocID(2),
    m_pInfo(0),
    m_infoSize(0)

{
    // Convert the SDK TOC into a CDDB TOC.
    gn_toc_info_t   toc;
    CDDBPopulateTOC(pTOC, &toc);
    convert_toc_info_to_full(&toc, &m_toc);
    delete [] toc.offsets;
}

CCDDBDiskInfo::~CCDDBDiskInfo()
{
    gnmem_free(m_pInfo);
}

//#define CDDB_INFO_HEADER    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ALB LNG=\"1\"><REV>1</REV><TIT>%s</TIT><ART>%s</ART><GEN ID=\"%d\">%s</GEN><TRKS CNT=\"%d\">"
#define CDDB_INFO_HEADER    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ALB LNG=\"1\"><REV>1</REV><TIT>%s</TIT><ART>%s</ART><GEN>%s</GEN><TRKS CNT=\"%d\">"
//#define CDDB_INFO_HEADER    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ALB LNG=\"1\"><REV>1</REV><TIT>%s</TIT><TRKS CNT=\"%d\">"
//#define CDDB_INFO_HEADER    "<?xml version=\"1.0\" encoding=\"UTF-8\" ?><ALB LNG=\"1\"><REV>1</REV><TIT>Unknown Album %d</TIT><TRKS CNT=\"%d\">"
//#define CDDB_INFO_TRACK     "<TRK N=\"%d\"><TIT>%s</TIT><ART>%s</ART><GENID=\"%d\">%s</GEN></TRK>"
#define CDDB_INFO_TRACK     "<TRK N=\"%d\"><TIT>%s</TIT></TRK>"
//#define CDDB_INFO_TRACK     "<TRK N=\"%d\"><TIT>Track %d</TIT></TRK>"
#define CDDB_INFO_FOOTER    "</TRKS></ALB>"

//! Takes the title, artist, genre, and track metadata from the IDiskInfo base class
//! and converts it into a cddb info blob.
//! Used for populating unknown CDs.
void
CCDDBDiskInfo::ConvertMetadataIntoInfo()
{
    // Create an XML info string.
    m_infoSize = strlen(CDDB_INFO_HEADER) + tstrlen(GetTitle()) + tstrlen(GetArtist()) + tstrlen(GetGenre()) + strlen(CDDB_INFO_FOOTER);
    int iMaxTrackTagLen = 0;
    for (int i = 0; i < m_svTracks.Size(); ++i)
    {
        int iTrackTagLen = strlen(CDDB_INFO_TRACK) + tstrlen(m_svTracks[i]->GetTrackName());
        if (iTrackTagLen > iMaxTrackTagLen)
            iMaxTrackTagLen = iTrackTagLen;
        m_infoSize += strlen(CDDB_INFO_TRACK) + tstrlen(m_svTracks[i]->GetTrackName());
    }
    m_pInfo = gnmem_malloc(m_infoSize);

    char szTitle[256], szArtist[256], szGenre[256];
    sprintf((char*)m_pInfo, CDDB_INFO_HEADER,
        TcharToCharN(szTitle, GetTitle(), 255),
        TcharToCharN(szArtist, GetArtist(), 255),
        TcharToCharN(szGenre, GetGenre(), 255),
        m_svTracks.Size());
    char* szTrack = new char[iMaxTrackTagLen + 1];
    for (int i = 0; i < m_svTracks.Size(); ++i)
    {
        sprintf(szTrack, CDDB_INFO_TRACK, i + 1, TcharToCharN(szTitle, m_svTracks[i]->GetTrackName(), 255));
        strcat((char*)m_pInfo, szTrack);
    }
    delete [] szTrack;
    strcat((char*)m_pInfo, CDDB_INFO_FOOTER);

    m_infoSize = strlen((char*)m_pInfo) + 1;

//diag_printf("#### Raw:\n%s\n\nSize: %d Strlen: %d\n#####\n", (char*)m_pInfo, m_infoSize, strlen((char*)m_pInfo));
}

void
CCDDBDiskInfo::ProcessXML(XMLTag* pXML)
{
    /* for online lookup, data will be in structured XML */
    /* we must render it to a buffer before saving */
    gnmem_free(m_pInfo);
    m_pInfo = RenderXMLTagToStrEx(pXML, GN_FALSE, GN_FALSE, GN_FALSE);

    /* we also need the toc_id to keep this TOC from being updated */
    XMLTagRef discid_tag = GetXMLSubTagFromStr(pXML, GN_STR_DISCID);

    /* extract toc_id(MUID) from XML */
    if (discid_tag)
    {
        XMLTagRef muid_tag = GetXMLSubTagFromStr(discid_tag, GN_STR_MUID);
        if (muid_tag)
        {
            gn_cstr_t muid_str = GetXMLTagData(muid_tag);
            if (muid_str)
                m_tocID = parse_buf_to_int32(muid_str, strlen(muid_str));
        }
    }

    /* just in case we couldn't get it, set to non-zero */
    if (m_tocID == 0)
        m_tocID = 2;

    m_infoSize = strlen((char*)m_pInfo) + 1;
}


CCDDBPartialDiskInfo::CCDDBPartialDiskInfo(const cdda_toc_t* pTOC, const char* muid, const char* mediaid)
    : CCDDBDiskInfo(pTOC)
{
    m_muid = strdup_new(muid);
    m_mediaid = strdup_new(mediaid);
}

CCDDBPartialDiskInfo::~CCDDBPartialDiskInfo()
{
    delete [] m_muid;
    delete [] m_mediaid;
}

bool
CCDDBPartialDiskInfo::RetrieveDiskInfo()
{
    DEBUG(CDDBHELPER, DBGLEV_INFO, "Retrieving partial disk info\n");

    XMLTag* pXML;
    gn_error_t lookup_err = gntlu_lookup_id_online(m_muid, m_mediaid, &pXML);

    if (lookup_err == SUCCESS)
    {
        if (pXML)
        {
            /* convert XML data to disc info structure */
            gn_discinfo_t*  disc_info = 0;
            gn_error_t xlate_err = gnxlt_convert_xml_alloc(pXML, &disc_info);
            if (xlate_err == SUCCESS)
            {
                CDDBDiskInfoToDiskInfo(disc_info, this);

			    gnxlt_release(disc_info);
			    gnmem_free((void*)disc_info);
            }
            else
                DEBUG(CDDBHELPER, DBGLEV_ERROR, "XML result translation error: 0x%x: %s\n", xlate_err, gnerr_get_error_message(xlate_err));

            ProcessXML(pXML);
    		DisposeXMLTag(pXML);
        }
        else
        {
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "Lookup succeeded but no data found\n");
            return false;
        }
    }
    else
    {
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Online lookup error: 0x%x: %s\n", lookup_err, gnerr_get_error_message(lookup_err));
        return false;
    }

    m_bPartial = false;
    return true;
}


//! Gets the latest update level for CDDB.
gn_error_t
CDDBGetUpdateLevel(unsigned short& usUpdateLevel, unsigned short& usDataRevision)
{
    gn_upd_error_t              error = UPDERR_NoError;
    gn_upd_install_profile_t    profile = {0};

    error = gn_upd_init();
    if (error != UPDERR_NoError)
        DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error intializing update: 0x%x: %s\n", error, gnerr_get_error_message(error));
    else
    {
        error = gn_upd_load_install_profile(&profile);
        if (error != UPDERR_NoError)
            DEBUG(CDDBHELPER, DBGLEV_ERROR, "Error getting update profile: 0x%x: %s\n", error, gnerr_get_error_message(error));
        else
        {
            usUpdateLevel = 0;
            for (int i = 0; i < profile.update_level_element_count; ++i)
                if (profile.update_level_list[i] > usUpdateLevel)
                    usUpdateLevel = profile.update_level_list[i];
            usDataRevision = profile.data_revision;

            gn_upd_cleanup_profile(&profile);
        }
        gn_upd_shutdown();
    }
    return error;
}
