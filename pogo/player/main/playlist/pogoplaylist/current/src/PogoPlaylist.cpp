#include <main/playlist/pogoplaylist/PogoPlaylist.h>
#include <stdlib.h> // rand
#include <content/common/QueryableContentManager.h>
#include <core/playmanager/PlayManager.h>
#include <main/ui/PlayerScreen.h>
#include <main/main/AppSettings.h>
#include <main/util/filenamestore/FileNameStore.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_POGOPLAYLIST, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_POGOPLAYLIST);

struct tEntryMetadataMapping {
    CPogoPlaylistEntry* entry;
    TCHAR* string;
    int numeric;
};

//////////////////////////////////////////////////////////////////////////////////////////
//	CPogoPlaylist
//////////////////////////////////////////////////////////////////////////////////////////

CPogoPlaylist::CPogoPlaylist(const char* szPlaylistName)
    : m_ePogoPlaylistMode(NORMAL), m_pSortHead(0), m_nAlbumDurationAlbumKey(-1), m_nRandomNumberSeed(47), m_bPragRandShortCircuit(false)
{
    m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
}

CPogoPlaylist::~CPogoPlaylist()
{
    Clear();
}

// Returns the playlist's name.
const char*
CPogoPlaylist::GetName() const
{
    return 0;
}

// Clears the playlist and frees up its memory.
void
CPogoPlaylist::Clear()
{
    while (!m_slPlaylistEntries.IsEmpty())
        delete m_slPlaylistEntries.PopFront();
    m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
    m_pSortHead = 0;
}

// Returns the number of entries in the playlist.
int
CPogoPlaylist::GetSize()
{
    return m_slPlaylistEntries.Size();
}

// Returns true if the playlist is empty, false otherwise.
bool
CPogoPlaylist::IsEmpty() const
{
    return m_slPlaylistEntries.IsEmpty();
}

// Adds an entry to the end of the playlist.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CPogoPlaylist::AddEntry(IMediaContentRecord* pContentRecord)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:AddEnt\n");
    CPogoPlaylistEntry* pEntry = new CPogoPlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.PushBack(pEntry);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();
    return pEntry;
}

// Inserts an entry to the playlist at the specified zero-based index.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CPogoPlaylist::InsertEntry(IMediaContentRecord* pContentRecord, int index)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:InsEnt\n");
    CPogoPlaylistEntry* pEntry = new CPogoPlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.Insert(pEntry, index);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    return pEntry;
}

// Adds all entries from the content record list to the end of the playlist.
// (epg,10/17/2001): TODO: this will resort every time entries are added, so if it is used
// in an additive way, it will resort the same elements, which wouldn't be very efficient.
// Currently in Pogo this method isn't used that way anywhere, but if that should change,
// then the sorting should be separated out into its own explicit event.
void
CPogoPlaylist::AddEntries(MediaRecordList& records)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:AddEntries\n");
    for (MediaRecordIterator it = records.GetHead(); it != records.GetEnd(); ++it)
        AddEntry(*it);
    ResortSortEntries();
}


// Removes an entry from the playlist.
void
CPogoPlaylist::DeleteEntry(IPlaylistEntry* pEntry)
{
    DeleteEntry(FindEntryByEntry(pEntry));
}

// Removes all entries from the given data source from the playlist.
void
CPogoPlaylist::DeleteEntriesFromDataSource(int iDataSourceID)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:DelEntriesFromDS\n");
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    while (it != m_slPlaylistEntries.GetEnd())
    {
        PlaylistIterator next = it + 1;
        if ((*it)->GetContentRecord()->GetDataSourceID() == iDataSourceID)
            DeleteEntry(it);
        it = next;
    }
    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetTail();
}

// use merge sort to sort the playlist entries according to the comparison function lt.
void CPogoPlaylist::MergeSortEntries(PlaylistEntryLessThanFunction lt)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:MergeSort\n");
    CPogoPlaylistEntry *p, *q, *e, *tail;
    int insize, nmerges, psize, qsize, i;

    insize = 1;

    while (1)
    {
        p = m_pSortHead;
        m_pSortHead = NULL;
        tail = NULL;
        
        nmerges = 0;  /* count number of merges we do in this pass */
        while (p)
        {
            nmerges++;   /* there exists a merge to be done */
            /* step `insize' places along from p */
            q = p;
            psize = 0;
            for (i = 0; i < insize; i++) 
            {
                psize++;
                q = q->m_pSortNext;
                if (!q) break;
            }
            /* if q hasn't fallen off end, we have two lists to merge */
            qsize = insize;
            /* now we have two lists; merge them */
            while (psize > 0 || (qsize > 0 && q)) 
            {

                /* decide whether next element of merge comes from p or q */
                if (psize == 0) {
                    /* p is empty; e must come from q. */
                    e = q; q = q->m_pSortNext; qsize--;
                } else if (qsize == 0 || !q) {
                    /* q is empty; e must come from p. */
                    e = p; p = p->m_pSortNext; psize--;
                } else if (lt(p,q)) {
                /* First element of p is lower (or same);
                    * e must come from p. */
                    e = p; p = p->m_pSortNext; psize--;
                } else {
                    /* First element of q is lower; e must come from q. */
                    e = q; q = q->m_pSortNext; qsize--;
                }
                
                /* add the next element to the merged list */
                if (tail) {
                    tail->m_pSortNext = e;
                } else {
                    m_pSortHead = e;
                }
                e->m_pSortPrev = tail;
                tail = e;
            }
            p = q;
        }
        tail->m_pSortNext = NULL;
        m_pSortHead->m_pSortPrev = NULL;
        
        /* If we have done only one merge, we're finished. */
        if (nmerges <= 1)   /* allow for nmerges==0, the empty list case */
            return;
        
        /* Otherwise repeat, merging lists twice the size */
        insize *= 2;
    }
}

// sort the entries with the lessthan function supplied.
void CPogoPlaylist::SortEntries(PlaylistEntryLessThanFunction lt)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:Sort\n");
    MergeSortEntries(lt);
}

// given two playlist entries, and a string property id, return whether left is less than right in that attribute.
inline bool StringPropLessThan(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right, int iAttributeID)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:StringLT\n");
    void *pDataLeft = 0;
    void *pDataRight = 0;
    left->GetContentRecord()->GetAttribute(iAttributeID, &pDataLeft);
    right->GetContentRecord()->GetAttribute(iAttributeID, &pDataRight);
    if (pDataLeft && pDataRight)
        // (epg,10/16/2001): TODO: write tstricmp, replace 
        return (tstrcmp((TCHAR*)pDataLeft,(TCHAR*)pDataRight) <= 0);
    else if (pDataLeft)
        return true;
    return false;
}

// given two playlist entries, and an integer property id, return whether left is less than right in that attribute.
inline bool IntPropLessThan(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right, int iAttributeID)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:IntLT\n");
    void *pDataLeft = 0;
    void *pDataRight = 0;
    if (!SUCCEEDED(left->GetContentRecord()->GetAttribute(iAttributeID, &pDataLeft)))
        pDataLeft = 0;
    if (!SUCCEEDED(right->GetContentRecord()->GetAttribute(iAttributeID, &pDataRight)))
        pDataRight = 0;
    if (pDataLeft && pDataRight)
        return ((int)pDataLeft <= (int)pDataRight);  // (epg,10/16/2001): TODO: hard to tell if the data will be in *pdata or maybe just pdata.  TEST ME!
    else if (pDataRight)
        return false;
    return true;
}

// given two pogo playlist entries, determine if the left entry's 
// track name is less than the right entry's track name.
bool TrackNameLessThan(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:TrackNameLT\n");
    return StringPropLessThan(left,right,MDA_TITLE);
}

// given two pogo playlist entries, determine if the left entry's 
// album position is less than the right entry's album position.
bool AlbumPositionLessThan(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:AlbumPosLT\n");
    return IntPropLessThan(left,right,MDA_ALBUM_TRACK_NUMBER);
}

// given two pogo playlist entries, determine if the left entry's 
// album name is less than the right entry's album name.
bool AlbumNameLessThan(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:AlbumNmLT\n");
    return StringPropLessThan(left,right,MDA_ALBUM);
}

// place the sort links in playlist order so the links represent a correct linked list.
void CPogoPlaylist::SortToPlaylistOrder()
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:SortPLOrder\n");
    if (m_slPlaylistEntries.Size() == 0)
        return;
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    (*it)->m_pSortPrev = NULL;
    m_pSortHead = (*it);
    PlaylistIterator next;
    for (int i = 0; it != NULL; ++i, ++it)
    {
        next = it + 1;
        if (next != NULL) {
            (*it)->m_pSortNext = (*next);
            (*next)->m_pSortPrev = (*it);
        } else
        {
            (*it)->m_pSortNext = NULL;
        }
    }
    m_pSortHead->m_pSortPrev = NULL;
}

// print out a string describing the entry passed in.  
// the track number is the position it holds in the playlist
void PrintPlaylistEntryAttributes(int iTrackNum, IPlaylistEntry* pEntry)
{
    IMediaContentRecord* cr = pEntry->GetContentRecord();
    char temp[256];
    void* pdata;

    // print the track number
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "%d ", iTrackNum);

    // and album
    if (SUCCEEDED(cr->GetAttribute(MDA_ALBUM, &pdata)))
        TcharToChar(temp,(TCHAR*)pdata);
    else
        strcpy (temp, "<?>");
     DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, " album '%s'", temp);

    // and album position number
    if (!SUCCEEDED(cr->GetAttribute(MDA_ALBUM_TRACK_NUMBER, &pdata)))
        pdata = (void*)-1;
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, ", albumpos %d", (int)pdata);
    
    // and title
    if (SUCCEEDED(cr->GetAttribute(MDA_TITLE, &pdata)))
        TcharToChar(temp,(TCHAR*)pdata);
    else
        strcpy (temp, "<?>");
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, ", title '%s'\n", temp);
}

// print out each entry's attributes, in the current sorted order.
void CPogoPlaylist::PrintSortedPlaylistEntryAttributes(const char* szCaption)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "Sorted %s\n", szCaption);
    int pos = 1;
    for (CPogoPlaylistEntry* p = m_pSortHead; p != NULL; p = p->m_pSortNext)
    {
        PrintPlaylistEntryAttributes(pos,p);
        ++pos;
    }
}

void CPogoPlaylist::RandomizeOrderOfAlbums()
{
    // count the albums
    void* pData;
    TCHAR tszAlbum[PLAYLIST_STRING_SIZE];
    tszAlbum[0] = 0;
    TCHAR tszNoAlbum[1];
    tszNoAlbum[0] = 0;
    int cAlbums = 0;
    CPogoPlaylistEntry* walker = m_pSortHead;
    while (walker != NULL)
    {
        walker->GetContentRecord()->GetAttribute(MDA_ALBUM, &pData);
        if (!pData)
            pData = (void*)tszNoAlbum;
        if (tstrcmp((TCHAR*) pData,tszAlbum))
        {
            ++cAlbums;
            tstrcpy(tszAlbum,(TCHAR*)pData);
        }
        walker = walker->m_pSortNext;
    }
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "%d albums found\n",cAlbums); 
    // iteratively select albums at random from the pool, and move them into a new ordering.
    CPogoPlaylistEntry* pOldHead = m_pSortHead;
    m_pSortHead = NULL;
    CPogoPlaylistEntry* pNewTail = NULL;
    while (pOldHead != NULL)
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "%d albums left\n",cAlbums); 
        // select an album from the remaining albums to go next
        int nToSkip = rand() % cAlbums;
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "skipping %d albums\n",nToSkip); 
        // skip over the intervening albums to the desired next album
        walker = pOldHead;
        walker->GetContentRecord()->GetAttribute(MDA_ALBUM, &pData);
        if (!pData)
            pData = (void*)tszNoAlbum;
        tstrcpy(tszAlbum,(TCHAR*) pData);
        int nSkipped = 0;
        int nSkippedInAlbum = 0;
        while (nToSkip > 0)
        {
            ++nSkipped; ++nSkippedInAlbum;
            walker = walker->m_pSortNext;
            walker->GetContentRecord()->GetAttribute(MDA_ALBUM, &pData);
            if (!pData)
                pData = (void*)tszNoAlbum;
            if (tstrcmp((TCHAR*) pData, tszAlbum))
            {
                DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "album was %d\n",nSkippedInAlbum); 
                nSkippedInAlbum = 0;
                --nToSkip;
                tstrcpy(tszAlbum,(TCHAR*) pData);
            }
        }
        // find the end of the desired album
        CPogoPlaylistEntry* pLastInAlbum = walker;

        if (pLastInAlbum->m_pSortNext)
            pLastInAlbum->m_pSortNext->GetContentRecord()->GetAttribute(MDA_ALBUM,&pData);
        if (!pData)
            pData = (void*)tszNoAlbum;
        while (pLastInAlbum->m_pSortNext && !tstrcmp(tszAlbum,(TCHAR*) pData))
        {
            pLastInAlbum = pLastInAlbum->m_pSortNext;
            if (pLastInAlbum->m_pSortNext)
                pLastInAlbum->m_pSortNext->GetContentRecord()->GetAttribute(MDA_ALBUM,&pData);
            if (!pData)
                pData = (void*)tszNoAlbum;
        }
        // move the album from the old list to the new.
        if (walker->m_pSortPrev)
            walker->m_pSortPrev->m_pSortNext = pLastInAlbum->m_pSortNext;
        if (pLastInAlbum->m_pSortNext)
            pLastInAlbum->m_pSortNext->m_pSortPrev = walker->m_pSortPrev;
        // if this is the first album in the old list, then update the old head pointer.
        if (walker == pOldHead)
            pOldHead = pLastInAlbum->m_pSortNext;
        if (m_pSortHead == NULL)
        {
            m_pSortHead = walker;
            walker->m_pSortPrev = NULL;
        }
        else
        {
            pNewTail->m_pSortNext = walker;
            walker->m_pSortPrev = pNewTail;
        }
        pLastInAlbum->m_pSortNext = NULL;
        pNewTail = pLastInAlbum;
        // count down the number of albums to move
        --cAlbums;
    }
}

// order the sort links by random album.  ie, albums are selected at random, and then the 
// whole album is played in album order.
void CPogoPlaylist::SortEntriesByRandomAlbumOrder()
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:sort album\n");
    // start with a standard album ordering
    SortEntriesByAlbumOrder();
    // randomize the order of the albums
    RandomizeOrderOfAlbums();
}

void CPogoPlaylist::InitMapToNativeOrdering(tEntryMetadataMapping* map,int nMappings)
{
    int i = 0;
    for (PlaylistIterator entry = m_slPlaylistEntries.GetHead(); entry != m_slPlaylistEntries.GetEnd(); ++entry, ++i)
    {
        map[i].entry = *entry;
        map[i].string = NULL;
        map[i].numeric = -1;
    }
}

const static TCHAR s_szEmptyString[1] = { 0 };

// if possible, load the data with the album position.  if not, then fall back on title.
void CPogoPlaylist::SetMapToAlbumPositionAndTitle(tEntryMetadataMapping* map, int nMappings)
{
    for (int i = 0; i < nMappings; ++i)
    {
        if (!SUCCEEDED(map[i].entry->GetContentRecord()->GetAttribute(MDA_ALBUM_TRACK_NUMBER, (void**)&(map[i].numeric))))
            map[i].numeric = -1;
        map[i].entry->GetContentRecord()->GetAttribute(MDA_TITLE, (void**)&(map[i].string));
    }
}

void CPogoPlaylist::SetMapToAlbumName(tEntryMetadataMapping* map, int nMappings)
{
    for (int i = 0; i < nMappings; ++i)
        if (!SUCCEEDED(map[i].entry->GetContentRecord()->GetAttribute(MDA_ALBUM, (void**)&(map[i].string))))
            map[i].string = (TCHAR*)s_szEmptyString;
}

int CompareMappingsByString(const void* a, const void* b)
{
    return tstrcmp(((tEntryMetadataMapping*)a)->string, ((tEntryMetadataMapping*)b)->string);
}

int CompareMappingsByInteger(const void* a, const void* b)
{
    // if both a & b have integer data, then compare the numbers.
    if (((tEntryMetadataMapping*)a)->numeric > -1 && ((tEntryMetadataMapping*)b)->numeric > -1)
    {
        if (((tEntryMetadataMapping*)a)->numeric <  ((tEntryMetadataMapping*)b)->numeric)
            return -1;
        // if the numbers are the same, fall back on string order.
        else if (((tEntryMetadataMapping*)a)->numeric ==  ((tEntryMetadataMapping*)b)->numeric)
            return CompareMappingsByString(a,b);
        return 1;
    }
    // if only a has a numeric datum, then it comes first
    if (((tEntryMetadataMapping*)a)->numeric > -1)
        return -1;
    // if only b has a numeric datum, then it coems first.
    if (((tEntryMetadataMapping*)b)->numeric > -1)
        return 1;
    // if there is only fallback string data, then call string compare.
    return CompareMappingsByString(a,b);
}

void CPogoPlaylist::SortMapByString(tEntryMetadataMapping* map, int nMappings)
{
    if (nMappings > 1)
        qsort( (void*) map, nMappings, sizeof (tEntryMetadataMapping), CompareMappingsByString);
}

void CPogoPlaylist::SortMapByInteger(tEntryMetadataMapping* map, int nMappings)
{
    if (nMappings > 1)
        qsort( (void*) map, nMappings, sizeof (tEntryMetadataMapping), CompareMappingsByInteger);
}

// order the sort links in the entries to reflect an "album" ordering.  
// albums should be grouped and arranged alphabetically.  tracks should
// be ordered by album-position when available, and alphabetically otherwise.
// this works out to a sort by name, then album position, then album name.
void CPogoPlaylist::SortEntriesByAlbumOrder()
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:sort album\n");
    int nEntries = m_slPlaylistEntries.Size();
    if (nEntries == 0)
        return;
    // construct an array to sort with 
    tEntryMetadataMapping* map = new tEntryMetadataMapping[nEntries+1];
    // init the map to the native playlist ordering
    InitMapToNativeOrdering(map,nEntries);
    
    // sort by album name
    SetMapToAlbumName(map,nEntries);
    SortMapByString(map,nEntries);

    // iterate through the albums, sorting each internally to album position
    TCHAR tszAlbum[PLAYLIST_STRING_SIZE];
    tstrcpy(tszAlbum,map[0].string);
    int iAlbumFrom = 0;
    int i;
    for (i = 1; i < nEntries; ++i)
    {
        if (tstrcmp(tszAlbum,map[i].string))
        {
            // sort the previous album
            SetMapToAlbumPositionAndTitle(&map[iAlbumFrom],i-iAlbumFrom);
            SortMapByInteger(&map[iAlbumFrom],i-iAlbumFrom);
            // start tracking a new album
            tstrcpy(tszAlbum,map[i].string);
            iAlbumFrom = i;
        }
    }
    // sort the last album
    if (iAlbumFrom != i)
    {
        SetMapToAlbumPositionAndTitle(&map[iAlbumFrom],i-iAlbumFrom);
        SortMapByInteger(&map[iAlbumFrom],i-iAlbumFrom);
    }
    
    // use the sorted array to construct the sort linked list
    m_pSortHead = map[0].entry;
    m_pSortHead->m_pSortPrev = NULL;
    for (int i = 0; i < nEntries; ++i)
    {
        if (i == nEntries - 1)
            map[i].entry->m_pSortNext = NULL;
        else
        {
            map[i].entry->m_pSortNext = map[i+1].entry;
            map[i+1].entry->m_pSortPrev = map[i].entry;
        }
    }
    PrintSortedPlaylistEntryAttributes("Album Order");

    delete [] map;
}



// Reshuffles the random links in the playlist.  The current entry will be set as the first
// random entry.
void 
CPogoPlaylist::ReshuffleRandomEntries() 
{
    int nEntries = m_slPlaylistEntries.Size();

    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:reshuf\n"); 
    // remember the current entry
    CPogoPlaylistEntry* pCurrent = (CPogoPlaylistEntry*)GetCurrentEntry();
    
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:array to native\n"); 
    // create an array of all the entries, in native order
    // (epg,1/29/2002): space req'd to shuffle all of filernew: 40k
    CPogoPlaylistEntry** entries = new CPogoPlaylistEntry*[m_slPlaylistEntries.Size()];
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; i < nEntries; ++i, ++it)
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:init ary[%d] to %x\n",i,(int)(*it)); 
        entries[i] = (*it);
    }
    
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:shuf\n"); 
    // randomize them.
    int iSwap;
    CPogoPlaylistEntry* temp;
    for (int iTarget = m_slPlaylistEntries.Size() - 1; iTarget > 0; --iTarget)
    {
        // select one from the iTarget
        iSwap = rand() % iTarget;
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:settings %d to %d\n",iTarget,iSwap); 
        
        // and swap it with the top entry
        temp = entries[iTarget];
        entries[iTarget] = entries[iSwap];
        entries[iSwap] = temp;
    }

    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:put crnt first\n"); 
    // place the current entry first
    if (pCurrent != NULL)
    {
        int iCurrent = -1;
        while (entries[++iCurrent] != pCurrent);
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:crnt had idx %d\n",iCurrent); 
        entries[iCurrent] = entries[0];
        entries[0] = pCurrent;
    }

    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:re list\n"); 
    // reconstruct the list links from the array
    // handle the head
    m_pSortHead = entries[0];
    m_pSortHead->m_pSortPrev = NULL;
    // handle the body
    for (int i = 1; i < m_slPlaylistEntries.Size() - 1; ++i)
    {
        entries[i]->m_pSortPrev = entries[i-1];
        entries[i]->m_pSortNext = entries[i+1];
        entries[i-1]->m_pSortNext = entries[i];
    }
    // handle the tail
    entries[nEntries-1]->m_pSortNext = NULL;
    if (nEntries > 1)
        entries[nEntries-1]->m_pSortPrev = entries[nEntries-2];

    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:list structure:\n"); 
    for (int i = 0; i < m_slPlaylistEntries.Size(); ++i) {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:%d: prev %x, this %x, next %x\n",i,(int)entries[i]->m_pSortPrev,(int)entries[i],(int)entries[i]->m_pSortNext);
    }

    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:del ary %x\n",(int)entries); 
    delete [] entries;

    //PrintSortedPlaylistEntryAttributes("Random");

#if 0
    // (epg,1/28/2002): slow on longer lists, but operates in-place
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:Reshuf\n");
    CPogoPlaylistEntry* pCurrent = (CPogoPlaylistEntry*)GetCurrentEntry();
    m_pSortHead = pCurrent;
    m_pSortHead->m_pSortNext = m_pSortHead->m_pSortPrev = 0;
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; it != m_slPlaylistEntries.GetEnd(); ++i, ++it)
    {
        if (*it == pCurrent)
        {
            --i;
            continue;
        }
        int iSortIndex = rand() % (i + 1);
        CPogoPlaylistEntry* pSortNext = m_pSortHead;
        CPogoPlaylistEntry* pSortPrev = 0;
        while (iSortIndex--)
        {
            pSortPrev = pSortNext;
            pSortNext = pSortNext->m_pSortNext;
        }
        (*it)->m_pSortNext = pSortNext;
        (*it)->m_pSortPrev = pSortPrev;
        if (pSortNext)
            pSortNext->m_pSortPrev = *it;
        if (pSortPrev)
            pSortPrev->m_pSortNext = *it;
        else
            m_pSortHead = *it;
    }
#endif
}

// Returns a pointer to the entry at the specified index, or 0 if the index is out-of-range.
IPlaylistEntry*
CPogoPlaylist::GetEntry(int index, PlaylistMode dummy)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:GetEntry\n");
    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
        case REPEAT_ALL:
        case REPEAT_TRACK:  
        case PRAGMATIC_RANDOM:
        {
            PlaylistIterator it = FindEntryByIndex(index);
            return it != m_slPlaylistEntries.GetEnd() ? *it : 0;
        }
        case RANDOM:
        case REPEAT_RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
        {
            PlaylistIterator it = FindEntryByEntry(m_pSortHead);
            while (it != m_slPlaylistEntries.GetEnd() && index--)
                it = GetNextEntry(it, dummy);
            return it == m_slPlaylistEntries.GetEnd() ? 0 : *it;
        }
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "pgp:invalid pl mode %d\n", (int)m_ePogoPlaylistMode);
            return (IPlaylistEntry*) 0;
    }
}

// Returns the zero-based index of the given playlist entry.
// Returns -1 if the entry is not in the playlist.
int
CPogoPlaylist::GetEntryIndex(const IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; it != m_slPlaylistEntries.GetEnd(); ++i, ++it)
        if (*it == pEntry)
            return i;

    return -1;
}

// Returns a pointer to the current entry.
IPlaylistEntry*
CPogoPlaylist::GetCurrentEntry()
{
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Sets the current entry pointer to the given record.
// If the entry isn't in the list, then the current entry pointer is set to 0.
IPlaylistEntry*
CPogoPlaylist::SetCurrentEntry(IPlaylistEntry* pEntry)
{
    m_itCurrentEntry = FindEntryByEntry(pEntry);
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Returns a pointer to the next entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CPogoPlaylist::GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode dummy)
{
    PlaylistIterator it = GetNextEntry(FindEntryByEntry(pEntry), dummy);
    if (it == m_slPlaylistEntries.GetEnd())
        return 0;
    else
        return *it;
}

// Sets the current entry to the next entry in the playlist as determined by the mode.
// Returns a pointer to the new current entry, or 0 if the end of the list was reached
// (and the play mode isn't repeating).
IPlaylistEntry*
CPogoPlaylist::SetNextEntry(PlaylistMode dummy)
{
    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        return 0;

    PlaylistIterator it = GetNextEntry(m_itCurrentEntry, dummy);
    if (m_ePogoPlaylistMode == PRAGMATIC_RANDOM)
        SetNextPragRandEntry();
    if (it != m_slPlaylistEntries.GetEnd())
    {
        m_itCurrentEntry = it;
        return *it;
    }
    else
        return 0;
}

// Returns an iterator that points to the next entry in the playlist as determined by the mode.
// If the end of the list was reached and the play mode isn't repeating
// then the iterator points to the end of the list (i.e., 0).
CPogoPlaylist::PlaylistIterator
CPogoPlaylist::GetNextEntry(PlaylistIterator it, PlaylistMode dummy)
{
    if (it == m_slPlaylistEntries.GetEnd())
        return it;

    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
            return it + 1;

        case REPEAT_ALL:
            if (it + 1 == m_slPlaylistEntries.GetEnd())
                return m_slPlaylistEntries.GetHead();
            else
                return it + 1;

        case RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
            if ((*it)->m_pSortNext)
                return FindEntryByEntry((*it)->m_pSortNext);
            else
                return m_slPlaylistEntries.GetEnd();

        case REPEAT_RANDOM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
            if ((*it)->m_pSortNext)
                return FindEntryByEntry((*it)->m_pSortNext);
            else
                return FindEntryByEntry(m_pSortHead);

        case REPEAT_TRACK:
            return it;
        case PRAGMATIC_RANDOM:
            return GetNextPragRandEntry(it);
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:getnext ent bad listmode\n");
    }
}

IPlaylistEntry* CPogoPlaylist::GetHeadEntry()
{
    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
        case REPEAT_ALL:
        case REPEAT_TRACK:
            return (*m_slPlaylistEntries.GetHead());
        case RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
        case REPEAT_RANDOM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
        {
            return m_pSortHead;
        }
        case PRAGMATIC_RANDOM:
            return (*m_slPragRandContext.GetHead());
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:GetHead bad listmode\n");
            return 0;
    }
}

IPlaylistEntry* CPogoPlaylist::GetTailEntry()
{
    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
        case REPEAT_ALL:
        case REPEAT_TRACK:
            return (*m_slPlaylistEntries.GetTail());
        case RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
        case REPEAT_RANDOM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
        {
            CPogoPlaylistEntry* ent = m_pSortHead;
            while (ent->m_pSortNext)
                ent = ent->m_pSortNext;
            return ent;
        }
        case PRAGMATIC_RANDOM:
            return (*m_slPragRandContext.GetTail());
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:GetTail bad listmode\n");
            return 0;
    }
}

// Returns a pointer to the previous entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CPogoPlaylist::GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode dummy)
{
    PlaylistIterator it = GetPreviousEntry(FindEntryByEntry(pEntry), dummy);
    if (it == m_slPlaylistEntries.GetEnd())
        return 0;
    else
        return *it;
}

// Sets the current entry to the previous entry in the playlist as determined by the mode.
// Returns a pointer to the new current entry, or 0 if the end of the list was reached
// (and the play mode isn't repeating).
IPlaylistEntry*
CPogoPlaylist::SetPreviousEntry(PlaylistMode dummy)
{
    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        return 0;

    PlaylistIterator it = GetPreviousEntry(m_itCurrentEntry, dummy);
    if (m_ePogoPlaylistMode == PRAGMATIC_RANDOM)
        SetPrevPragRandEntry();
    if (it != m_slPlaylistEntries.GetEnd())
    {
        m_itCurrentEntry = it;
        return *it;
    }
    else
        return 0;
}

// Returns an iterator that points to the previous entry in the playlist as determined by the mode.
// If the end of the list was reached and the play mode isn't repeating
// then the iterator points to the end of the list (i.e., 0).
CPogoPlaylist::PlaylistIterator
CPogoPlaylist::GetPreviousEntry(PlaylistIterator it, PlaylistMode dummy)
{
    if (it == m_slPlaylistEntries.GetEnd())
        return it;

    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
            return it - 1;

        case REPEAT_ALL:
            if (it - 1 == m_slPlaylistEntries.GetEnd())
                return m_slPlaylistEntries.GetTail();
            else
                return it - 1;

        case RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
            if ((*it)->m_pSortPrev)
                return FindEntryByEntry((*it)->m_pSortPrev);
            else
                return m_slPlaylistEntries.GetEnd();

        case REPEAT_RANDOM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
            if ((*it)->m_pSortPrev)
                return FindEntryByEntry((*it)->m_pSortPrev);
            else
            {
                it = m_slPlaylistEntries.GetHead();
                while ((*it)->m_pSortNext)
                    ++it;
                return it;
            }

        case REPEAT_TRACK:
            return it;
        case PRAGMATIC_RANDOM:
            return GetPrevPragRandEntry(it);
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:getprv ent bad listmode\n");
    }
}

// Returns an iterator that points to the entry at the specified index.
// If the index is out-of-range, then the iterator points to the end of the list (i.e., 0).
CPogoPlaylist::PlaylistIterator
CPogoPlaylist::FindEntryByIndex(int index)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; (i < index) && (it != m_slPlaylistEntries.GetEnd()); ++it, ++i)
        ;
    return it;
}

// Returns an iterator that points to the given playlist entry.
// If the entry isn't in the list, then the iterator points to the end of the list (i.e., 0).
CPogoPlaylist::PlaylistIterator
CPogoPlaylist::FindEntryByEntry(IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (; (it != m_slPlaylistEntries.GetEnd()) && (*it != pEntry); ++it)
        ;
    return it;
}

// Removes the entry from the list and deletes it.
void
CPogoPlaylist::DeleteEntry(PlaylistIterator it)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:del ent\n");
    if (it != m_slPlaylistEntries.GetEnd())
    {
        if (m_itCurrentEntry == it)
            ++m_itCurrentEntry;

        if ((*it)->m_pSortNext)
            (*it)->m_pSortNext->m_pSortPrev = (*it)->m_pSortPrev;
        if ((*it)->m_pSortPrev)
            (*it)->m_pSortPrev->m_pSortNext = (*it)->m_pSortNext;
        else
            m_pSortHead = (*it)->m_pSortNext;

        delete m_slPlaylistEntries.Remove(it);
    }
}

// Finds a new place to put the entry in the sorted list.
void
CPogoPlaylist::SortEntry(CPogoPlaylistEntry* pEntry)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:sort ent\n");
    int iSortIndex = rand() % GetSize();

    CPogoPlaylistEntry* pSortNext = m_pSortHead;
    CPogoPlaylistEntry* pSortPrev = 0;
    while (iSortIndex--)
    {
        pSortPrev = pSortNext;
        pSortNext = pSortNext->m_pSortNext;
    }
    pEntry->m_pSortNext = pSortNext;
    pEntry->m_pSortPrev = pSortPrev;
    if (pSortNext)
        pSortNext->m_pSortPrev = pEntry;
    if (pSortPrev)
        pSortPrev->m_pSortNext = pEntry;
    else
        m_pSortHead = pEntry;
}

// Set the PogoPlaylistMode to be used until further notice.  This is necessary to support
// custom sorting in AddEntries, which has no mode parameter.
void 
CPogoPlaylist::SetPogoPlaylistMode(PogoPlaylistMode mode)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:SetPMode\n");

    if (m_ePogoPlaylistMode != mode)
    {
        switch (mode)
        {
            // these modes don't use the sorted ordering, so don't bother setting them up.
            case NORMAL:
            case REPEAT_ALL:
            case REPEAT_TRACK:
                break;
            case RANDOM:
            case REPEAT_RANDOM:
                // if we're just toggling the repeat attribute, don't re-sort.
                if (m_ePogoPlaylistMode != RANDOM && m_ePogoPlaylistMode != REPEAT_RANDOM)
                    ReshuffleRandomEntries();            
                break;
            case ALBUM:
            case REPEAT_ALBUM:
                // if we're just toggling the repeat attribute, don't re-sort.
                if (m_ePogoPlaylistMode != ALBUM && m_ePogoPlaylistMode != REPEAT_ALBUM)
                    SortEntriesByAlbumOrder();
                break;
            case RANDOM_ALBUM:
            case REPEAT_RANDOM_ALBUM:
                if (m_ePogoPlaylistMode != RANDOM_ALBUM && m_ePogoPlaylistMode != REPEAT_RANDOM_ALBUM)
                    SortEntriesByRandomAlbumOrder();
                break;
            case PRAGMATIC_RANDOM:
                DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "setting pragmatic random playmode\n"); 
                if (m_ePogoPlaylistMode != PRAGMATIC_RANDOM)
                    InitPragRandContext(m_itCurrentEntry);
                break;
            default:
                DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:setmode bad listmode\n");
        }
        m_ePogoPlaylistMode = mode;
        if (!IsModeAlbumBased())
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->DecayAlbumTimeDisplay();
    }
}

CPogoPlaylist::PogoPlaylistMode
CPogoPlaylist::GetPogoPlaylistMode()
{
    return m_ePogoPlaylistMode;
}

bool CPogoPlaylist::IsModeAlbumBased()
{
    return ((m_ePogoPlaylistMode == ALBUM) 
         || (m_ePogoPlaylistMode == REPEAT_ALBUM) 
         || (m_ePogoPlaylistMode == RANDOM_ALBUM) 
         || (m_ePogoPlaylistMode == REPEAT_RANDOM_ALBUM));
}

// return the album key associated with the playlist entry passed in.
int GetAlbumKey(IPlaylistEntry* pEntry)
{
    void* pdata = 0;
    pEntry->GetContentRecord()->GetAttribute(MDA_ALBUM,&pdata);
    IQueryableContentManager* qcm = (IQueryableContentManager*)CPlayManager::GetInstance()->GetContentManager();
    return qcm->GetAlbumKey((TCHAR*)pdata);
}

// returns the track start time of the entry passed in.  in album order modes, this will be
// relative to the start of the album, as currently ordered.  time returned has an elapsed sense
int CPogoPlaylist::GetTrackStartTime(CPogoPlaylistEntry* pEntry)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:GetTrackStart\n");

    if (!IsModeAlbumBased())
        return 0;
    void* pdata = 0;
    int nAlbumKey = GetAlbumKey(pEntry);
    CPogoPlaylistEntry* pIter = pEntry;   
    // walk back on the list to the first album entry
    while ((pIter->m_pSortPrev) && GetAlbumKey(pIter) == nAlbumKey )
        pIter = pIter->m_pSortPrev;
    if (GetAlbumKey(pIter) != nAlbumKey)
        pIter = pIter->m_pSortNext;
    int nAlbumTime = 0;
    while (pIter != pEntry)
    {
        pIter->GetContentRecord()->GetAttribute(MDA_DURATION, &pdata);
        nAlbumTime += (int)pdata;
        pIter = pIter->m_pSortNext;
    }
    return nAlbumTime;
}

// returns the duration of the album of which the entry passed in is a member.
int CPogoPlaylist::GetAlbumDuration(CPogoPlaylistEntry* pEntry)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:GetAlbumDur\n");
    if (!IsModeAlbumBased())
        return 0;
    void* pdata = 0;
    int nAlbumKey = GetAlbumKey(pEntry);
    if (nAlbumKey != m_nAlbumDurationAlbumKey)
    {
        CPogoPlaylistEntry* pIter = pEntry;   
        // walk back on the list to the first album entry
        while ((pIter->m_pSortPrev) && GetAlbumKey(pIter) == nAlbumKey)
            pIter = pIter->m_pSortPrev;
        if (GetAlbumKey(pIter) != nAlbumKey)
            pIter = pIter->m_pSortNext;
        // count up the album time.
        int nAlbumTime = 0;
        while (pIter && GetAlbumKey(pIter) == nAlbumKey)
        {
            pIter->GetContentRecord()->GetAttribute(MDA_DURATION, &pdata);
            nAlbumTime += (int)pdata;
            pIter = pIter->m_pSortNext;
        }
        m_nAlbumDurationAlbumKey = nAlbumKey;
        m_nAlbumDuration = nAlbumTime;
    }
    return m_nAlbumDuration;
}

// randomize the seed, and return it for serialization
int CPogoPlaylist::GetRandomNumberSeed()
{
    m_nRandomNumberSeed = rand();
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl: returning %d as rand seed\n",m_nRandomNumberSeed); 
    return m_nRandomNumberSeed;
}

// accept a new seed value from serialization, and reset the rand function based on it.
void CPogoPlaylist::SetRandomNumberSeed(int nSeed)
{
    m_nRandomNumberSeed = nSeed;
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl: rand seed set to %d from external source\n",nSeed); 
    srand((unsigned int) m_nRandomNumberSeed);
}

IPlaylistEntry* CPogoPlaylist::FindEntryByContentRecord(IContentRecord* cr)
{
    for (PlaylistIterator itrEntry = m_slPlaylistEntries.GetHead(); itrEntry != m_slPlaylistEntries.GetEnd(); ++itrEntry)
    {
        if ((*itrEntry)->GetContentRecord() == cr)
            return *itrEntry;
    }
    return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
//	CPogoPlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////

CPogoPlaylistEntry::CPogoPlaylistEntry(CPogoPlaylist* pPlaylist, IMediaContentRecord* pContentRecord)
    : m_pPlaylist(pPlaylist),
    m_pContentRecord(pContentRecord),
    m_pSortPrev(0),
    m_pSortNext(0)
{
}

CPogoPlaylistEntry::~CPogoPlaylistEntry()
{
}

void CPogoPlaylist::ResortSortEntries()
{
    switch (m_ePogoPlaylistMode)
    {
        case NORMAL:
        case REPEAT_ALL:
        case REPEAT_TRACK:
            // no sorting needed
            break;
        case PRAGMATIC_RANDOM:
            InitPragRandContext(m_itCurrentEntry);
            break;
        case RANDOM:
        case REPEAT_RANDOM:
            SortToPlaylistOrder();
            ReshuffleRandomEntries();
            break;
        case ALBUM:
        case REPEAT_ALBUM:
            SortEntriesByAlbumOrder();            
            break;
        case RANDOM_ALBUM:
        case REPEAT_RANDOM_ALBUM:
            SortEntriesByRandomAlbumOrder();
            break;
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:unknown list mode\n");
    }
}

bool CPogoPlaylist::IsModeRepeating()
{
    switch (m_ePogoPlaylistMode)
    {
        case REPEAT_ALL:
        case REPEAT_TRACK:
        case REPEAT_RANDOM:
        case REPEAT_ALBUM:
        case REPEAT_RANDOM_ALBUM:
            return true;
        default:
            return false;
    }
}


// advance in the pragmatic random contect, feeding the future end with a random entry
CPogoPlaylist::PlaylistIterator
CPogoPlaylist::GetNextPragRandEntry(PlaylistIterator itFrom)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "get next pre from %s\n",(*itFrom)->GetContentRecord()->GetFileNameRef()->LongName()); 
    int nEntries = m_slPlaylistEntries.Size();
    // if there are no entries, return 0.
    if (nEntries < 1)
        return NULL;
    // if there is one entry, return it.
    if (nEntries == 1)
        return m_slPlaylistEntries.GetHead();
    // for very short lists, just return the native ordering.  can't assume the iter passed in is really from the native list, so 
    // generate one on the fly.
    if (m_bPragRandShortCircuit)
        return GetPreviousEntry(FindEntryByEntry(*itFrom), IPlaylist::NORMAL);

    PlaylistIterator itWalk = m_slPragRandContext.GetHead();
    // advance to the source entry

    while (itWalk != m_slPragRandContext.GetEnd() && *itWalk != *itFrom)
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "skipping %s\n",(*itWalk)->GetContentRecord()->GetFileNameRef()->LongName()); 
        ++itWalk;
    }
    if (itWalk == m_slPragRandContext.GetEnd())
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:couldn't find source iterator\n"); 
        return itWalk;
    }
    ++itWalk;
    if (itWalk == m_slPragRandContext.GetEnd())
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "PPL:requested next pragrand too far into future\n"); 
    }
    else
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "incremented to %s\n",(*itWalk)->GetContentRecord()->GetFileNameRef()->LongName()); 
    }

    return itWalk;
}

// back up in pragmatic random context, feeding the pastmost end with a random entry
CPogoPlaylist::PlaylistIterator 
CPogoPlaylist::GetPrevPragRandEntry(PlaylistIterator itFrom)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "get prev pre from %s\n",(*itFrom)->GetContentRecord()->GetFileNameRef()->LongName()); 
    int nEntries = m_slPlaylistEntries.Size();
    // if there are no entries, return 0.
    if (nEntries < 1)
        return NULL;
    // if there is one entry, return it.
    if (nEntries == 1)
        return m_slPlaylistEntries.GetHead();
    // for very short lists, just return the native ordering.  can't assume the iter passed in is really from the native list, so 
    // generate one on the fly.
    if (m_bPragRandShortCircuit)
        return GetPreviousEntry(FindEntryByEntry(*itFrom), IPlaylist::NORMAL);
        
    PlaylistIterator itWalk = m_slPragRandContext.GetHead();
    // advance to the source entry
    while (itWalk != m_slPragRandContext.GetEnd() && *itWalk != *itFrom)
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "skipping past %s\n",(*itWalk)->GetContentRecord()->GetFileNameRef()->LongName()); 
        ++itWalk;
    }
    if (itWalk == m_slPragRandContext.GetEnd())
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:couldn't find source iterator\n"); 
        return itWalk;
    }
    --itWalk;
    if (itWalk == m_slPragRandContext.GetEnd())
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "PPL:requested prev pragrand too far into past\n"); 
    }
    else
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "decremented back to %s\n",(*itWalk)->GetContentRecord()->GetFileNameRef()->LongName()); 
    }
    return itWalk;
}

void CPogoPlaylist::SetCurrentPragRandDynamically()
{
    int nEntries = m_slPlaylistEntries.Size();
    int iLast = (*m_itCurrentEntry)->GetIndex();
    int iRand = rand() % (nEntries - 1);
    // if the list is long enough, disallow immediate repeats
    if (nEntries > 2)
        while (iRand == iLast)
            iRand = rand() % (nEntries - 1);
    m_itCurrentEntry = m_slPlaylistEntries.GetHead();
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "ppl: dyn select %d\n",iRand); 
    while (iRand-- > 0);
        ++m_itCurrentEntry;
    return;
}

// move the current entry iterator within the prag rand context list
void CPogoPlaylist::SetNextPragRandEntry()
{
    int nEntries = m_slPlaylistEntries.Size();
    if (nEntries <= 1)
        return;
    // for very short lists, no context is maintained.
    if (m_bPragRandShortCircuit)
        SetCurrentPragRandDynamically();
    // main line for larger lists
    if (m_itPragRandEntry != m_slPragRandContext.GetEnd())
    {
        ++m_itPragRandEntry;
        // discard the pastmost entry
        m_slPragRandContext.PopFront();
        m_slContextIndexes.PopFront();
        // push on a new futuremost entry
        AddFuturemostPragRand();
    }
    else
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "PPL: null prag rand itr\n"); 
    }
}

// move the current entry iterator within the prag rand context list
void CPogoPlaylist::SetPrevPragRandEntry()
{
    int nEntries = m_slPlaylistEntries.Size();
    if (nEntries <= 1)
        return;
    // for very short lists, no context is maintained.
    // for very short lists, no context is maintained.
    if (m_bPragRandShortCircuit)
        SetCurrentPragRandDynamically();
    // main line for larger lists
    if (m_itPragRandEntry != m_slPragRandContext.GetEnd())
    {
        --m_itPragRandEntry;
        // discard the futuremost entry
        m_slPragRandContext.PopBack();
        m_slContextIndexes.PopBack();
        // push on a new pastmost entry
        AddPastmostPragRand();
    }
    else
    {
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "PPL: null prag rand itr\n"); 
    }
}

// generate a fresh random context list, anchored at the current entry.
void CPogoPlaylist::InitPragRandContext(PlaylistIterator itCurrent)
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:init prag rand\n"); 
    int nEntries = m_slPlaylistEntries.Size();
    m_slPragRandContext.Clear();
    m_slContextIndexes.Clear();
    // if there are no entries, return 0.
    if (nEntries < 1)
        return;
    int iLast = -1;

    // fall back on the first playlist entry as current.
    if (itCurrent == 0)
        itCurrent = m_slPlaylistEntries.GetHead();

    // special case for 4 entries or less.
    if (nEntries <= 4)
    {
        m_bPragRandShortCircuit = true;
        return;
    }
    else
        m_bPragRandShortCircuit = false;

    // restrict the context radius on shorter lists
    if ((nEntries - 3)/2 < PRAGMATIC_RANDOM_CONTEXT_RADIUS)
    {
        m_nContextRadius = (nEntries - 3)/2;
        DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "ppl: list size %d thresholds context radius to %d\n",nEntries,m_nContextRadius); 
    }
    else
        m_nContextRadius = PRAGMATIC_RANDOM_CONTEXT_RADIUS;
    
    // current entry
    int iCurrent = (*itCurrent)->GetIndex();
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "crnt=%d %s\n",iCurrent,(*itCurrent)->GetContentRecord()->GetFileNameRef()->LongName()); 
    m_slPragRandContext.PushBack(*itCurrent);
    m_slContextIndexes.PushBack(iCurrent);
    // prepend past entries
    iLast = iCurrent;
    for (int i = 0; i < m_nContextRadius; ++i)
        AddPastmostPragRand();
    // append future entries
    iLast = iCurrent;
    for (int i = 0; i < m_nContextRadius; ++i)
        AddFuturemostPragRand();
    // position the current prag rand iterator in the middle of the list.
    m_itPragRandEntry = m_slPragRandContext.GetHead();
    for (int i = 0; i < m_nContextRadius; ++i)
        ++m_itPragRandEntry;
}

// given the current context, select a new entry to be added to the context which doesn't break the repetition rules
int CPogoPlaylist::GenerateNewPragRandIndex()
{
    int nEntries = m_slPlaylistEntries.Size();
    int iEntry = rand() % (nEntries-1);
    bool bReject;
    // disallow repeats within context
    do {
        bReject = false;
        for (SimpleListIterator<int> itWalk = m_slContextIndexes.GetHead(); itWalk != m_slContextIndexes.GetEnd(); ++itWalk)
        {
            if (*itWalk == iEntry)
            {
                bReject = true;
                break;
            }
        }
        if (bReject)
            iEntry = rand() % (nEntries-1);
    } while (bReject);
    return iEntry;
}

// add a new index to the beginning of the context list
void CPogoPlaylist::AddPastmostPragRand()
{
    int iEntry = GenerateNewPragRandIndex();

    CPogoPlaylistEntry* entry = (CPogoPlaylistEntry*)GetEntry(iEntry,IPlaylist::NORMAL);
    m_slPragRandContext.PushFront(entry);
    m_slContextIndexes.PushFront(iEntry);
}

// add a new index to the end of the context list
void CPogoPlaylist::AddFuturemostPragRand()
{
    int iEntry = GenerateNewPragRandIndex();

    CPogoPlaylistEntry* entry = (CPogoPlaylistEntry*)GetEntry(iEntry,IPlaylist::NORMAL);
    m_slPragRandContext.PushBack(entry);
    m_slContextIndexes.PushBack(iEntry);
}