//
// DJPlaylist.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <main/playlist/djplaylist/DJPlaylist.h>
#include <main/main/AppSettings.h>  // MAX_PLAYLIST_TRACKS
#include <stdlib.h> // rand

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_DJ_PLAYLIST, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_DJ_PLAYLIST);  // debugging prefix : (16) sp

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJPlaylist
//////////////////////////////////////////////////////////////////////////////////////////

CDJPlaylist::CDJPlaylist(const char* szPlaylistName)
    : m_pShuffleFirst(0),
    m_bDirty(false)
{
    m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
}

CDJPlaylist::~CDJPlaylist()
{
    Clear();
}

// Returns the playlist's name.
const char*
CDJPlaylist::GetName() const
{
    return 0;
}

// Clears the playlist and frees up its memory.
void
CDJPlaylist::Clear()
{
    if (!m_slPlaylistEntries.IsEmpty())
    {
        while (!m_slPlaylistEntries.IsEmpty())
            delete m_slPlaylistEntries.PopFront();
        m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
        m_pShuffleFirst = 0;

        m_bDirty = true;
    }
}

// Returns the number of entries in the playlist.
int
CDJPlaylist::GetSize()
{
    return m_slPlaylistEntries.Size();
}

// Returns true if the playlist is empty, false otherwise.
bool
CDJPlaylist::IsEmpty() const
{
    return m_slPlaylistEntries.IsEmpty();
}

// Adds an entry to the end of the playlist.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CDJPlaylist::AddEntry(IMediaContentRecord* pContentRecord)
{
    CDJPlaylistEntry* pEntry = AddEntryInternal(pContentRecord);

    ShuffleEntry(pEntry);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    m_bDirty = true;

    return pEntry;
}

// Inserts an entry to the playlist at the specified zero-based index.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CDJPlaylist::InsertEntry(IMediaContentRecord* pContentRecord, int index)
{
    CDJPlaylistEntry* pEntry = new CDJPlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.Insert(pEntry, index);

    ShuffleEntry(pEntry);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    m_bDirty = true;

    return pEntry;
}

//#define PROFILE_TIME
#ifdef PROFILE_TIME
#include <cyg/kernel/kapi.h>
#endif  // PROFILE_TIME

// Adds all entries from the content record list to the end of the playlist.
void
CDJPlaylist::AddEntries(MediaRecordList& records)
{
#ifdef PROFILE_TIME
    cyg_tick_count_t tickStart = cyg_current_time();
    diag_printf("%s: Start tick count: %d\n", __FUNCTION__, tickStart);
#endif  // PROFILE_TIME

    MediaRecordIterator it = records.GetHead();
    int iToAdd;
#ifdef MAX_PLAYLIST_TRACKS
    iToAdd = GetSize() + records.Size() > MAX_PLAYLIST_TRACKS ? MAX_PLAYLIST_TRACKS - GetSize() : records.Size();
#else
    iToAdd = records.Size();
#endif
    while (iToAdd-- > 0)
    {
        AddEntryInternal(*it);
        ++it;
    }

    m_bDirty = true;

    ReshuffleRandomEntries();

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();
    
#ifdef PROFILE_TIME
    cyg_tick_count_t tickEnd = cyg_current_time();
    diag_printf("%s: Tick count: %d Track count: %d Tracks per tick: %d\n", __FUNCTION__,
        (int)(tickEnd - tickStart),
        GetSize(),
        GetSize() / (int)(tickEnd - tickStart));
#endif  // PROFILE_TIME
}

// Adds an entry to the playlist but doesn't adjust the shuffle order.
CDJPlaylistEntry*
CDJPlaylist::AddEntryInternal(IMediaContentRecord* pContentRecord)
{
    CDJPlaylistEntry* pEntry = new CDJPlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.PushBack(pEntry);

    return pEntry;
}

// Removes an entry from the playlist.
void
CDJPlaylist::DeleteEntry(IPlaylistEntry* pEntry)
{
    DeleteEntry(FindEntryByEntry(pEntry));
}

// Removes all entries from the given data source from the playlist.
void
CDJPlaylist::DeleteEntriesFromDataSource(int iDataSourceID)
{
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

// Reshuffles the random links in the playlist.  The current entry will be set as the first
// random entry.
void 
CDJPlaylist::ReshuffleRandomEntries() 
{
#if 1

    int nEntries = m_slPlaylistEntries.Size();
    if (nEntries == 0)
        return;
    else if (nEntries == 1)
    {
        m_pShuffleFirst = *m_slPlaylistEntries.GetHead();
        return;
    }

    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:reshuf\n"); 
    // remember the current entry
    CDJPlaylistEntry* pCurrent = (CDJPlaylistEntry*)GetCurrentEntry();
    
    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:array to native\n"); 
    // create an array of all the entries, in native order
    // (epg,1/29/2002): space req'd to shuffle all of filernew: 40k
    CDJPlaylistEntry** entries = new CDJPlaylistEntry*[m_slPlaylistEntries.Size()];
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; i < nEntries; ++i, ++it)
    {
        DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:init ary[%d] to %x\n",i,(int)(*it)); 
        entries[i] = (*it);
    }
    
    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:shuf\n"); 
    // randomize them.
    int iSwap;
    CDJPlaylistEntry* temp;
    for (int iTarget = nEntries - 1; iTarget > 0; --iTarget)
    {
        // select one from the iTarget
        iSwap = rand() % iTarget;
        DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:settings %d to %d\n",iTarget,iSwap); 
        
        // and swap it with the top entry
        temp = entries[iTarget];
        entries[iTarget] = entries[iSwap];
        entries[iSwap] = temp;
    }

    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:put crnt first\n"); 
    // place the current entry first
    if (pCurrent != NULL)
    {
        int iCurrent = -1;
        while (entries[++iCurrent] != pCurrent);
        DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:crnt had idx %d\n",iCurrent); 
        entries[iCurrent] = entries[0];
        entries[0] = pCurrent;
    }

    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:re list\n"); 
    // reconstruct the list links from the array
    // handle the head
    m_pShuffleFirst = entries[0];
    m_pShuffleFirst->m_pShufflePrev = NULL;
    // handle the body
    for (int i = 1; i < nEntries; ++i)
    {
        entries[i]->m_pShufflePrev = entries[i-1];
        entries[i-1]->m_pShuffleNext = entries[i];
    }
    // handle the tail
    entries[nEntries-1]->m_pShuffleNext = NULL;

    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:del ary %x\n",(int)entries); 
    delete [] entries;

#else

    CDJPlaylistEntry* pCurrent = (CDJPlaylistEntry*)GetCurrentEntry();
    if (pCurrent)
    {
        m_pShuffleFirst = pCurrent;
        m_pShuffleFirst->m_pShuffleNext = m_pShuffleFirst->m_pShufflePrev = 0;
        PlaylistIterator it = m_slPlaylistEntries.GetHead();
        for (int i = 0; it != m_slPlaylistEntries.GetEnd(); ++i, ++it)
        {
            if (*it == m_pShuffleFirst)
            {
                --i;
                continue;
            }
            int iShuffleIndex = rand() % i + 1;
            CDJPlaylistEntry* pShuffleNext = m_pShuffleFirst;
            CDJPlaylistEntry* pShufflePrev = 0;
            while (iShuffleIndex--)
            {
                pShufflePrev = pShuffleNext;
                pShuffleNext = pShuffleNext->m_pShuffleNext;
            }
            (*it)->m_pShuffleNext = pShuffleNext;
            (*it)->m_pShufflePrev = pShufflePrev;
            if (pShuffleNext)
                pShuffleNext->m_pShufflePrev = *it;
            if (pShufflePrev)
                pShufflePrev->m_pShuffleNext = *it;
            else
                m_pShuffleFirst = *it;
        }
    }

#endif
}

// Returns a pointer to the entry at the specified zero-based index using the given playlist mode.
// If the index is out-of-range, then 0 is returned.
IPlaylistEntry*
CDJPlaylist::GetEntry(int index, IPlaylist::PlaylistMode mode = NORMAL)
{
    if (mode == NORMAL)
    {
        PlaylistIterator it = FindEntryByIndex(index);
        return it != m_slPlaylistEntries.GetEnd() ? *it : 0;
    }

    PlaylistIterator it;
    if ((mode == RANDOM) || (mode == REPEAT_RANDOM))
        it = FindEntryByEntry(m_pShuffleFirst);
    else
        it = m_slPlaylistEntries.GetHead();
    while (it != m_slPlaylistEntries.GetEnd() && index--)
        it = GetNextEntry(it, mode);
    return (it == m_slPlaylistEntries.GetEnd()) ? 0 : *it;
}

// Returns the zero-based index of the given playlist entry.
// Returns -1 if the entry is not in the playlist.
int
CDJPlaylist::GetEntryIndex(const IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; it != m_slPlaylistEntries.GetEnd(); ++i, ++it)
        if (*it == pEntry)
            return i;

    return -1;
}

// Returns a pointer to the current entry.
IPlaylistEntry*
CDJPlaylist::GetCurrentEntry()
{
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Sets the current entry pointer to the given record.
// If the entry isn't in the list, then the current entry pointer is set to 0.
IPlaylistEntry*
CDJPlaylist::SetCurrentEntry(IPlaylistEntry* pEntry)
{
    m_itCurrentEntry = FindEntryByEntry(pEntry);
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Returns a pointer to the next entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CDJPlaylist::GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode mode)
{
    PlaylistIterator it = GetNextEntry(FindEntryByEntry(pEntry), mode);
    if (it == m_slPlaylistEntries.GetEnd())
        return 0;
    else
        return *it;
}

// Sets the current entry to the next entry in the playlist as determined by the mode.
// Returns a pointer to the new current entry, or 0 if the end of the list was reached
// (and the play mode isn't repeating).
IPlaylistEntry*
CDJPlaylist::SetNextEntry(PlaylistMode mode)
{
    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        return 0;

    PlaylistIterator it = GetNextEntry(m_itCurrentEntry, mode);
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
CDJPlaylist::PlaylistIterator
CDJPlaylist::GetNextEntry(PlaylistIterator it, PlaylistMode mode)
{
    if (it == m_slPlaylistEntries.GetEnd())
        return it;

    switch (mode)
    {
        case NORMAL:
            return it + 1;

        case REPEAT_ALL:
            if (it + 1 == m_slPlaylistEntries.GetEnd())
                return m_slPlaylistEntries.GetHead();
            else
                return it + 1;

        case RANDOM:
            if ((*it)->m_pShuffleNext)
                return FindEntryByEntry((*it)->m_pShuffleNext);
            else
                return m_slPlaylistEntries.GetEnd();

        case REPEAT_RANDOM:
            if ((*it)->m_pShuffleNext)
                return FindEntryByEntry((*it)->m_pShuffleNext);
            else
                return FindEntryByEntry(m_pShuffleFirst);

        case REPEAT_TRACK:
            return it;
    }
    // this line should never happen, but we want to appease the gcc gods
    DBASSERT(DBG_DJ_PLAYLIST, 0, "Unknown playlist mode %d\n", (int) mode);
    return it;
}

// Returns a pointer to the previous entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CDJPlaylist::GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode mode)
{
    PlaylistIterator it = GetPreviousEntry(FindEntryByEntry(pEntry), mode);
    if (it == m_slPlaylistEntries.GetEnd())
        return 0;
    else
        return *it;
}

// Sets the current entry to the previous entry in the playlist as determined by the mode.
// Returns a pointer to the new current entry, or 0 if the end of the list was reached
// (and the play mode isn't repeating).
IPlaylistEntry*
CDJPlaylist::SetPreviousEntry(PlaylistMode mode)
{
    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        return 0;

    PlaylistIterator it = GetPreviousEntry(m_itCurrentEntry, mode);
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
CDJPlaylist::PlaylistIterator
CDJPlaylist::GetPreviousEntry(PlaylistIterator it, PlaylistMode mode)
{
    if (it == m_slPlaylistEntries.GetEnd())
        return it;

    switch (mode)
    {
        case NORMAL:
            return it - 1;

        case REPEAT_ALL:
            if (it - 1 == m_slPlaylistEntries.GetEnd())
                return m_slPlaylistEntries.GetTail();
            else
                return it - 1;

        case RANDOM:
            if ((*it)->m_pShufflePrev)
                return FindEntryByEntry((*it)->m_pShufflePrev);
            else
                return m_slPlaylistEntries.GetEnd();

        case REPEAT_RANDOM:
            if ((*it)->m_pShufflePrev)
                return FindEntryByEntry((*it)->m_pShufflePrev);
            else
            {
                it = m_slPlaylistEntries.GetHead();
                while ((*it)->m_pShuffleNext)
                    ++it;
                return it;
            }

        case REPEAT_TRACK:
            return it;
    }
    // this line should never happen, but we want to appease the gcc gods
    DBASSERT(DBG_DJ_PLAYLIST, 0, "Unknown playlist mode %d\n", (int) mode);
    return it;
}

// Returns an iterator that points to the entry at the specified index.
// If the index is out-of-range, then the iterator points to the end of the list (i.e., 0).
CDJPlaylist::PlaylistIterator
CDJPlaylist::FindEntryByIndex(int index)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; (i < index) && (it != m_slPlaylistEntries.GetEnd()); ++it, ++i)
        ;
    return it;
}

// Returns an iterator that points to the given playlist entry.
// If the entry isn't in the list, then the iterator points to the end of the list (i.e., 0).
CDJPlaylist::PlaylistIterator
CDJPlaylist::FindEntryByEntry(IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (; (it != m_slPlaylistEntries.GetEnd()) && (*it != pEntry); ++it)
        ;
    return it;
}

// Removes the entry from the list and deletes it.
void
CDJPlaylist::DeleteEntry(PlaylistIterator it)
{
    if (it != m_slPlaylistEntries.GetEnd())
    {
        if (m_itCurrentEntry == it)
            ++m_itCurrentEntry;

        if ((*it)->m_pShuffleNext)
            (*it)->m_pShuffleNext->m_pShufflePrev = (*it)->m_pShufflePrev;
        if ((*it)->m_pShufflePrev)
            (*it)->m_pShufflePrev->m_pShuffleNext = (*it)->m_pShuffleNext;
        else
            m_pShuffleFirst = (*it)->m_pShuffleNext;

        delete m_slPlaylistEntries.Remove(it);

        m_bDirty = true;
    }
}

// Finds a new place to put the entry in the shuffled list.
void
CDJPlaylist::ShuffleEntry(CDJPlaylistEntry* pEntry)
{
    int iShuffleIndex = rand() % GetSize();

    CDJPlaylistEntry* pShuffleNext = m_pShuffleFirst;
    CDJPlaylistEntry* pShufflePrev = 0;

    while (iShuffleIndex--)
    {
        pShufflePrev = pShuffleNext;
        pShuffleNext = pShuffleNext->m_pShuffleNext;
    }

    pEntry->m_pShuffleNext = pShuffleNext;
    pEntry->m_pShufflePrev = pShufflePrev;
    if (pShuffleNext)
        pShuffleNext->m_pShufflePrev = pEntry;
    if (pShufflePrev)
        pShufflePrev->m_pShuffleNext = pEntry;
    else
        m_pShuffleFirst = pEntry;
}

//! Clears all entries that start with the given URL.
void
CDJPlaylist::ClearURL(const char* szURL)
{
    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:ClearURL: %d start entries\n"); 

    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    PlaylistIterator itNext;
    unsigned int len = strlen(szURL);
    while (it != m_slPlaylistEntries.GetEnd())
    {
        itNext = it + 1;
        if (!strnicmp(szURL, (*it)->GetContentRecord()->GetURL(), len))
        {
            if (m_itCurrentEntry == it)
                m_itCurrentEntry = itNext;
            m_slPlaylistEntries.Remove(it);
            m_bDirty = true;
        }
        it = itNext;
    }

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    ReshuffleRandomEntries();

    DEBUGP( DBG_DJ_PLAYLIST, DBGLEV_TRACE, "djpl:ClearURL: %d end entries\n"); 
}

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJPlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////

CDJPlaylistEntry::CDJPlaylistEntry(CDJPlaylist* pPlaylist, IMediaContentRecord* pContentRecord)
    : m_pPlaylist(pPlaylist),
    m_pContentRecord(pContentRecord),
    m_pShufflePrev(0),
    m_pShuffleNext(0)
{
}

CDJPlaylistEntry::~CDJPlaylistEntry()
{
}
