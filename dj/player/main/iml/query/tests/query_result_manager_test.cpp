#include <pkgconf/kernel.h>
#include <cyg/kernel/kapi.h>            /* All the kernel specific stuff */
#include <cyg/hal/hal_arch.h>           /* CYGNUM_HAL_STACK_SIZE_TYPICAL */
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_edb7xxx.h>
#include <cyg/hal/drv_api.h>

#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#include <main/iml/query/QueryResultManager.h>
#include <main/iml/query/QueryResult.h>

#include <util/debug/debug.h>

#include <stdio.h>  /* sprintf */
#include <stdlib.h>

DEBUG_MODULE_S(MAIN, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(MAIN);

/* DEFINES */

#define NTHREADS 1
#define STACKSIZE (CYGNUM_HAL_STACK_SIZE_TYPICAL + (16 * 4096))

/* STATICS */

static cyg_handle_t _ThreadH[NTHREADS];
static cyg_thread _Thread[NTHREADS];
static char _ThreadStack[NTHREADS][STACKSIZE];

static void
mem_stats(void)
{
	struct mallinfo mem_info;		    
	mem_info = mallinfo();
	diag_printf("Memory system: Total=0x%08x Used = 0x%08x Free=0x%08x Max=0x%08x\n",
		    mem_info.arena, mem_info.arena - mem_info.fordblks, mem_info.fordblks,
		    mem_info.maxfree);
}

void
PrintArtists(CGeneralQueryResult* pQR, int iStartIndex, int iCount, bool bQueryIfMissing = false)
{
    ContentKeyValueVector svArtists;
    int filled = pQR->GetValues(svArtists, iStartIndex, iCount, bQueryIfMissing);

    diag_printf("Items %d to %d (%d filled)\n", iStartIndex, iStartIndex + iCount - 1, filled);
    for (int i = 0; i < iCount; ++i)
    {
        if (svArtists[i].iKey)
        {
            char szValue[16];
            diag_printf("    %d: Key: %d Value: %s\n", iStartIndex + i, svArtists[i].iKey, TcharToChar(szValue, svArtists[i].szValue));
        }
        else
            diag_printf("    %d: No value\n", iStartIndex + i);
    }
    diag_printf("\n");
}

void
AddArtists(iml_query_info_t& queryInfo, int iStartIndex, int iCount)
{
    queryInfo.iStartIndex = iStartIndex;
    queryInfo.iTotalItems = iCount;
    ContentKeyValueVector* psvArtists = new ContentKeyValueVector;
    for (int i = 0; i < iCount; ++i)
    {
        cm_key_value_record_t ak;
        ak.iKey = iStartIndex + i + 1;
        ak.szValue = (TCHAR*)malloc(sizeof(TCHAR) * 16);
        char szValue[16];
        sprintf(szValue, "Artist %d", iStartIndex + i);
        CharToTchar(const_cast<TCHAR*>(ak.szValue), szValue);
        psvArtists->PushBack(ak);
    }
    queryInfo.pKeyValues = psvArtists;
}

void
NewArtistResultsCallback(CQueryResult* pQR, int iStartIndex, int iItemCount, void* pUserData)
{
    diag_printf("New artists:\n");
    PrintArtists((CGeneralQueryResult*)pQR, iStartIndex, iItemCount, false);
}

void
PrintMediaItems(CMediaQueryResult* pQR, int iStartIndex, int iCount, bool bQueryIfMissing = false)
{
    IMLMediaInfoVector svMedia;
    int filled = pQR->GetValues(svMedia, iStartIndex, iCount, bQueryIfMissing);

    diag_printf("Media items %d to %d (%d filled)\n", iStartIndex, iStartIndex + iCount - 1, filled);
    for (int i = 0; i < iCount; ++i)
    {
        if (svMedia[i].iMediaKey)
        {
            char szValue[16];
            diag_printf("    %d: Key: %d Title: %s\n", iStartIndex + i, svMedia[i].iMediaKey, TcharToChar(szValue, svMedia[i].szMediaTitle));
        }
        else
            diag_printf("    %d: No value\n", iStartIndex + i);
    }
    diag_printf("\n");
}

void
AddMediaItems(iml_query_library_info_t& queryInfo, int iStartIndex, int iCount)
{
    queryInfo.iStartIndex = iStartIndex;
    queryInfo.iTotalItems = iCount;
    IMLMediaInfoVector* psvMedia = new IMLMediaInfoVector;
    for (int i = 0; i < iCount; ++i)
    {
        iml_media_info_t mk;
        mk.iMediaKey = iStartIndex + i + 1;
        mk.szMediaTitle = (TCHAR*)malloc(sizeof(TCHAR) * 16);
        char szValue[16];
        sprintf(szValue, "Title %d", iStartIndex + i);
        CharToTchar(const_cast<TCHAR*>(mk.szMediaTitle), szValue);
        psvMedia->PushBack(mk);
    }
    queryInfo.pRecords = psvMedia;
}

void
NewMediaItemsResultsCallback(CQueryResult* pQR, int iStartIndex, int iItemCount, void* pUserData)
{
    diag_printf("New media items:\n");
    PrintMediaItems((CMediaQueryResult*)pQR, iStartIndex, iItemCount, false);
}

static void
_MainThread(CYG_ADDRESS Data)
{       
    DBEN(MAIN);

    mem_stats();

    {
        CIML* pIML = new CIML(1, "Mr. Friendly");
        CIMLManager* pIM = CIMLManager::GetInstance();
        pIM->AddIML(pIML);

        CQueryResultManager* pQRM = CQueryResultManager::GetInstance();

        CGeneralQueryResult* pQR = pQRM->CreateGeneralQueryResult(pIML, &CIML::ResumeQueryArtists);
        pQR->SetNewResultsCallback(NewArtistResultsCallback);

        PrintArtists(pQR, 0, 20, true);

        iml_query_info_t queryInfo;
        queryInfo.iDeviceNumber = 1;
        queryInfo.iViewID = 1;
        queryInfo.status = QUERY_SUCCESSFUL;

        AddArtists(queryInfo, 0, 10);
        pQR->ProcessQueryResults(&queryInfo);
        delete queryInfo.pKeyValues;
        PrintArtists(pQR, 0, 20, true);

        AddArtists(queryInfo, 13, 4);
        pQR->ProcessQueryResults(&queryInfo);
        delete queryInfo.pKeyValues;
        PrintArtists(pQR, 0, 20, true);

        pQRM->RemoveQueryResult(pQR);
        delete pQR;
        CQueryResultManager::Destroy();

        pIM->RemoveIML(pIML);
        delete pIML;
        CIMLManager::Destroy();
    }

    mem_stats();

    {
        CIML* pIML = new CIML(2, "Mr. Yuck");
        CIMLManager* pIM = CIMLManager::GetInstance();
        pIM->AddIML(pIML);

        CQueryResultManager* pQRM = CQueryResultManager::GetInstance();

        CMediaQueryResult* pQR = pQRM->CreateMediaQueryResult(pIML);
        pQR->SetNewResultsCallback(NewMediaItemsResultsCallback);

        PrintMediaItems(pQR, 0, 20, true);

        iml_query_library_info_t queryInfo;
        queryInfo.iDeviceNumber = 2;
        queryInfo.iViewID = 4;
        queryInfo.status = QUERY_SUCCESSFUL;

        AddMediaItems(queryInfo, 0, 10);
        pQR->ProcessQueryResults(&queryInfo);
        delete queryInfo.pRecords;
        PrintMediaItems(pQR, 0, 20, true);

        AddMediaItems(queryInfo, 13, 4);
        pQR->ProcessQueryResults(&queryInfo);
        delete queryInfo.pRecords;
        PrintMediaItems(pQR, 0, 20, true);

        pQRM->RemoveQueryResult(pQR);
        delete pQR;
        CQueryResultManager::Destroy();

        pIM->RemoveIML(pIML);
        delete pIML;
        CIMLManager::Destroy();
    }

    mem_stats();

    DBEX(MAIN);
}

extern "C" {

void
cyg_user_start(void)
{
    DBEN(MAIN);
    
    cyg_thread_create(10, _MainThread, (cyg_addrword_t) 0, "MainThread",
		      (void *)_ThreadStack[0], STACKSIZE, &_ThreadH[0], &_Thread[0]);
    cyg_thread_resume(_ThreadH[0]);

    DBEX(MAIN);
}

};
