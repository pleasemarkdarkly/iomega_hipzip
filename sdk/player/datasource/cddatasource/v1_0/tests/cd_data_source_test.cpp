#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <io/storage/drives.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <playlist/simpleplaylist/SimplePlaylist.h>

/* DEFINES */

#define DEBUG(s...) diag_printf(##s)
//#define DEBUG(s...) /**/

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
_MainThread(CYG_ADDRESS Data)
{       
    DEBUG("+%s\n", __FUNCTION__);

    CDataSourceManager* pDSM = CDataSourceManager::GetDataSourceManager();

//    CCDDataSource* pCDDataSource = CCDDataSource::Open("/dev/hda/");
    CCDDataSource* pCDDataSource = CCDDataSource::Open(block_drive_names[1]);

    pDSM->AddDataSource(pCDDataSource);

    int iDataSourceNum = pDSM->GetDataSourceCount(), i;
    DEBUG("Data source count: %d\n", iDataSourceNum);
    for (i = 0; i < iDataSourceNum; ++i)
    {
        IDataSource* pDS = pDSM->GetDataSourceByIndex(i);
        DEBUG("Data source %d ID: %d Type: %d\n", i, pDS->GetID(), pDS->GetType());
        IPlaylist* pPlaylist = pDS->ListAllEntries();
        int iPlaylistEntryCount = pPlaylist->GetSize();
        DEBUG("Playlist size: %d\n", iPlaylistEntryCount);
#if 0
        for (int j = 0; j < iPlaylistEntryCount; ++j)
        {
            IPlaylistEntry* pEntry = pPlaylist->GetEntry(j);
            if (pEntry)
            {
                IContentRecord* pRecord = pEntry->GetContentRecord();
                if (pRecord)
                {
                    DEBUG("Entry %d: %s %s %s %s %s\n", j, pRecord->GetURL(), pRecord->GetTitle(), pRecord->GetArtist(), pRecord->GetAlbum(), pRecord->GetGenre());
                    pDS->OpenPlaylistEntry(pEntry);
                }
            }
            else
                DEBUG("Entry %d: :(\n", j);
        }
#else
        IPlaylistEntry* pEntry = pPlaylist->GetCurrentEntry();
        while (pEntry)
        {
            IContentRecord* pRecord = pEntry->GetContentRecord();
            if (pRecord)
            {
                DEBUG("Entry %d: %s %s %s %s %s\n", pEntry->GetIndex(), pRecord->GetURL(), pRecord->GetTitle(), pRecord->GetArtist(), pRecord->GetAlbum(), pRecord->GetGenre());
                pDS->OpenPlaylistEntry(pEntry);
            }
            pEntry = pPlaylist->SetNextEntry();
        }
        pPlaylist->SetCurrentEntry(pPlaylist->GetEntry(iPlaylistEntryCount - 1));
        pEntry = pPlaylist->GetCurrentEntry();
        while (pEntry)
        {
            IContentRecord* pRecord = pEntry->GetContentRecord();
            if (pRecord)
            {
                DEBUG("Entry %d: %s %s %s %s %s\n", pEntry->GetIndex(), pRecord->GetURL(), pRecord->GetTitle(), pRecord->GetArtist(), pRecord->GetAlbum(), pRecord->GetGenre());
                pDS->OpenPlaylistEntry(pEntry);
            }
            pEntry = pPlaylist->SetPreviousEntry();
        }
#endif
        delete pPlaylist;
    }

    pDSM->RemoveDataSource(pCDDataSource);   

    iDataSourceNum = pDSM->GetDataSourceCount();
    DEBUG("Data source count: %d\n", iDataSourceNum);
    for (i = 0; i < iDataSourceNum; ++i)
        DEBUG("Data source %d ID: %d Type: %d\n", i, pDSM->GetDataSourceByIndex(i)->GetID(), pDSM->GetDataSourceByIndex(i)->GetType());

    CDataSourceManager::Destroy();

    DEBUG("-%s\n", __FUNCTION__);
}

extern "C" {

void
cyg_user_start(void)
{
    DEBUG("+%s\n", __FUNCTION__);
    
    cyg_thread_create(10, _MainThread, (cyg_addrword_t) 0, "MainThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    diag_printf("-%s\n", __FUNCTION__);
}

};
