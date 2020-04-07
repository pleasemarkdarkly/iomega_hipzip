#include <main/playlist/PogoPlaylist.h>
#include <stdlib.h> // rand
#include <content/common/QueryableContentManager.h>
#include <core/playmanager/PlayManager.h>
#include <main/ui/PlayerScreen.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_POGOPLAYLIST, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_POGOPLAYLIST );

//////////////////////////////////////////////////////////////////////////////////////////
//	CPogoPlaylist
//////////////////////////////////////////////////////////////////////////////////////////

CPogoPlaylist::CPogoPlaylist(const char* szPlaylistName)
    : m_ePogoPlaylistMode(NORMAL), m_pSortHead(0), m_nAlbumDurationAlbumKey(-1), m_nRandomNumberSeed(47)
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
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "track (%x) %d has ", (int)pEntry, iTrackNum);

    // and artist
    if (SUCCEEDED(cr->GetAttribute(MDA_ARTIST, &pdata)))
        TcharToChar(temp,(TCHAR*)pdata);
    else
        strcpy (temp, "<?>");
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, "artist '%s'", temp);
    
    // and album
    if (SUCCEEDED(cr->GetAttribute(MDA_ALBUM, &pdata)))
        TcharToChar(temp,(TCHAR*)pdata);
    else
        strcpy (temp, "<?>");
     DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, ", album '%s'", temp);

    // and genre
    if (SUCCEEDED(cr->GetAttribute(MDA_GENRE, &pdata)))
        TcharToChar(temp,(TCHAR*)pdata);
    else
        strcpy (temp, "<?>");
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_INFO, ", genre '%s'", temp);
    
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

// order the sort links in the entries to reflect an "album" ordering.  
// albums should be grouped and arranged alphabetically.  tracks should
// be ordered by album-position when available, and alphabetically otherwise.
// this works out to a sort by name, then album position, then album name.
void CPogoPlaylist::SortEntriesByAlbumOrder()
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:sort album\n");
    // initialize the sort links to playlist order
    SortToPlaylistOrder();
    //PrintSortedPlaylistEntryAttributes("Playlist Order");

    // alphabetize by name
    SortEntries((PlaylistEntryLessThanFunction) TrackNameLessThan);
    //PrintSortedPlaylistEntryAttributes("Track Name");

    // sort by album-position
    SortEntries((PlaylistEntryLessThanFunction) AlbumPositionLessThan);
    //PrintSortedPlaylistEntryAttributes("Album Position");

    // alphabetize by album name
    SortEntries((PlaylistEntryLessThanFunction) AlbumNameLessThan);
    //PrintSortedPlaylistEntryAttributes("Album Name");
}

// Reshuffles the random links in the playlist.  The current entry will be set as the first
// random entry.
void 
CPogoPlaylist::ReshuffleRandomEntries() 
{
    DEBUGP( DBG_POGOPLAYLIST, DBGLEV_TRACE, "ppl:Reshuf\n");
    srand((unsigned int) m_nRandomNumberSeed);
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
        int iSortIndex = rand() % i + 1;
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
    m_nRandomNumberSeed = rand();
    //PrintSortedPlaylistEntryAttributes("Random");
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
        {
            PlaylistIterator it = FindEntryByIndex(index);
            return it != m_slPlaylistEntries.GetEnd() ? *it : 0;
        }
        case RANDOM:
        case REPEAT_RANDOM:
        case ALBUM:
        case RANDOM_ALBUM:
        case REPEAT_ALBUM:
        case RANDOM_REPEAT_ALBUM:
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
        case RANDOM_REPEAT_ALBUM:
            if ((*it)->m_pSortNext)
                return FindEntryByEntry((*it)->m_pSortNext);
            else
                return FindEntryByEntry(m_pSortHead);

        case REPEAT_TRACK:
            return it;
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
        case RANDOM_REPEAT_ALBUM:
        {
            return m_pSortHead;
        }
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
        case RANDOM_REPEAT_ALBUM:
        {
            CPogoPlaylistEntry* ent = m_pSortHead;
            while (ent->m_pSortNext)
                ent = ent->m_pSortNext;
            return ent;
        }
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
        case RANDOM_REPEAT_ALBUM:
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
            case RANDOM_REPEAT_ALBUM:
                DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:unimplemented playlist mode selected!\n");
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
         || (m_ePogoPlaylistMode == RANDOM_REPEAT_ALBUM));
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

int CPogoPlaylist::GetRandomNumberSeed()
{
    return m_nRandomNumberSeed;
}

void CPogoPlaylist::SetRandomNumberSeed(int nSeed)
{
    m_nRandomNumberSeed = nSeed;
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
        case RANDOM_REPEAT_ALBUM:
            // (epg,10/17/2001): TODO: final sort category permutations aren't yet ironed out in the Pogo project.
            break;
        default:
            DEBUGP( DBG_POGOPLAYLIST, DBGLEV_WARNING, "PPL:unknown list mode\n");
    }
}