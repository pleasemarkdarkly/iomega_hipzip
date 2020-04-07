#include <main/buffering/BufferDebug.h>
#include <main/buffering/DocOrderAuthority.h>
#include <playlist/common/Playlist.h>
#include <core/playmanager/PlayManager.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_BUF_DOCORDER_AUTH, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_BUF_COMMON );
DEBUG_USE_MODULE(DBG_BUF_DOCORDER_AUTH);  // debugging prefix : (11) doa

CDocOrderAuthority::CDocOrderAuthority()
{
}

CDocOrderAuthority::~CDocOrderAuthority()
{

}

char* GenUrlFromEntry(IPlaylistEntry* pEntry)
{
    if (!pEntry)
        return 0;
    IMediaContentRecord* pCR = pEntry->GetContentRecord();
    const char* szUrl = pCR->GetURL();
    int nLen = strlen(szUrl)+1;
    char* szNewUrl = new char[nLen];
    if (!szNewUrl)
    {
        DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_INFO, "DOA:failed to alloc space for url!\n"); 
        return 0;
    }
    strcpy(szNewUrl,szUrl);
    return szNewUrl;
}

// sync with the dj playlist for ordering info
void CDocOrderAuthority::GetDJOrdering(StringList* plstUrls, int nBehind, int nAhead, IMediaContentRecord* mcr)
{
    DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "doa:GetDJOrdering\n"); 
    int nId = mcr->GetID();
    
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    DBASSERT( DBG_BUF_DOCORDER_AUTH, ( pPL->GetSize()>0 ) , "doa:empty playlist\n"); 
    IPlaylistEntry* pCurrent = pPL->GetCurrentEntry();
    int nCrntId = pCurrent->GetContentRecord()->GetID();
    IPlaylistEntry* pOrigin = NULL;
    if (nId == nCrntId)
    {
         DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "doa:orderCrntBased\n"); 
         pOrigin = pCurrent;
    }
    else
    {
        IPlaylistEntry* pNext = pPL->GetNextEntry(pCurrent,CPlayManager::GetInstance()->GetPlaylistMode());
        if (!pNext)
        {   
            pNext = pPL->GetEntry(0,CPlayManager::GetInstance()->GetPlaylistMode());
            DBASSERT( DBG_BUF_DOCORDER_AUTH, pNext!=NULL, "DOA:not crnt based, but no next!\n"); 
        }
        if (pNext && pNext->GetContentRecord()->GetID() == nId)
        {
            DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "doa:orderNextBased\n"); 
            pOrigin = pNext;
            
        }
        else
            DBASSERT( DBG_BUF_DOCORDER_AUTH, ( 1<0 ) , "doa:unknown id %d requested as ordering origin\n",nId);
    }

    DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "doa:<-%d,%d->,%d pl\n",nBehind,nAhead,pPL->GetSize()); 
    plstUrls->PushBack(GenUrlFromEntry(pOrigin));
    int cOrdered = 1;
    int nEntries = pPL->GetSize();
    // how many documents are requested?
    int nDocs = nBehind + nAhead + 1;
    DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "%d avail docs\n",nDocs); 
    // normalize down to playlist size
    if (nEntries < nDocs)
    {
        DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "pl.size<#docs\n"); 
        if ((nDocs - nEntries) > nBehind)
        {
            nBehind = 0; 
            DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "noBehind\n"); 
            nAhead = nEntries-1;
            DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "->Only %d\n",nAhead); 
        }
        else
        {
            nBehind -= (nDocs - nEntries);
            DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "<-Only %d\n",nBehind); 
        }
        nDocs = nEntries;
    }
    // walk backwards from the current entry
    IPlaylistEntry* pEntry = pOrigin;
    for (int i = 0; i < nBehind; ++i)
    {
        DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "%d behind\n",i+1); 
        if (cOrdered >= nEntries)
            break;
        pEntry = pPL->GetPreviousEntry(pEntry,CPlayManager::GetInstance()->GetPlaylistMode());
        // loop under if necessary
        if (!pEntry)
        {
            DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "loopUnder\n"); 
            pEntry = pPL->GetEntry(pPL->GetSize()-1,CPlayManager::GetInstance()->GetPlaylistMode());
        }
        char* url = GenUrlFromEntry(pEntry);
        plstUrls->PushFront(url);
        ++cOrdered;
    }
    // walk forward from the current entry
    pEntry = pOrigin;
    for (int i = 0; i < nAhead; ++i)
    {
        DEBUGP( DBG_BUF_DOCORDER_AUTH, DBGLEV_LOW_LEVEL_TRACE, "%d ahead\n",i+1); 
        if (cOrdered >= nEntries)
            break;
        pEntry = pPL->GetNextEntry(pEntry,CPlayManager::GetInstance()->GetPlaylistMode());
        // loop over if necessary
        if (!pEntry)
        {
            pEntry = pPL->GetEntry(0,CPlayManager::GetInstance()->GetPlaylistMode());
        }
        DBASSERT( DBG_BUF_DOCORDER_AUTH, ( pEntry ) , "DOA:asked for entry 0, got NULL!\n"); 

        plstUrls->PushBack(GenUrlFromEntry(pEntry));
        ++cOrdered;
    }
}

void CDocOrderAuthority::GetOrdering(StringList* plstUrls, int nBehind, int nAhead,IMediaContentRecord* mcr)
{
    plstUrls->Clear();
    GetDJOrdering(plstUrls, nBehind, nAhead,mcr);
}
