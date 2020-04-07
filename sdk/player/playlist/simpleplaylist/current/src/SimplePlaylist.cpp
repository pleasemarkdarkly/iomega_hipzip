//
// SimplePlaylist.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <playlist/simpleplaylist/SimplePlaylist.h>
#include <stdlib.h> // rand

#include <util/debug/debug.h>
DEBUG_MODULE_S(DBG_SIMPLE_PLAYLIST, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_SIMPLE_PLAYLIST);  // debugging prefix : (16) sp

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimplePlaylist
//////////////////////////////////////////////////////////////////////////////////////////

CSimplePlaylist::CSimplePlaylist(const char* szPlaylistName)
    : m_pShuffleFirst(0)
{
    m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
}

CSimplePlaylist::~CSimplePlaylist()
{
    Clear();
}

// Returns the playlist's name.
const char*
CSimplePlaylist::GetName() const
{
    return 0;
}

// Clears the playlist and frees up its memory.
void
CSimplePlaylist::Clear()
{
    while (!m_slPlaylistEntries.IsEmpty())
        delete m_slPlaylistEntries.PopFront();
    m_itCurrentEntry = m_slPlaylistEntries.GetEnd();
    m_pShuffleFirst = 0;
}

// Returns the number of entries in the playlist.
int
CSimplePlaylist::GetSize()
{
    return m_slPlaylistEntries.Size();
}

// Returns true if the playlist is empty, false otherwise.
bool
CSimplePlaylist::IsEmpty() const
{
    return m_slPlaylistEntries.IsEmpty();
}

// Adds an entry to the end of the playlist.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CSimplePlaylist::AddEntry(IMediaContentRecord* pContentRecord)
{
    CSimplePlaylistEntry* pEntry = new CSimplePlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.PushBack(pEntry);

    ShuffleEntry(pEntry);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    return pEntry;
}

// Inserts an entry to the playlist at the specified zero-based index.
// Returns a pointer to the new playlist entry.
IPlaylistEntry*
CSimplePlaylist::InsertEntry(IMediaContentRecord* pContentRecord, int index)
{
    CSimplePlaylistEntry* pEntry = new CSimplePlaylistEntry(this, pContentRecord);
    m_slPlaylistEntries.Insert(pEntry, index);

    ShuffleEntry(pEntry);

    if (m_itCurrentEntry == m_slPlaylistEntries.GetEnd())
        m_itCurrentEntry = m_slPlaylistEntries.GetHead();

    return pEntry;
}

// Adds all entries from the content record list to the end of the playlist.
void
CSimplePlaylist::AddEntries(MediaRecordList& records)
{
    for (MediaRecordIterator it = records.GetHead(); it != records.GetEnd(); ++it)
        AddEntry(*it);
}


// Removes an entry from the playlist.
void
CSimplePlaylist::DeleteEntry(IPlaylistEntry* pEntry)
{
    DeleteEntry(FindEntryByEntry(pEntry));
}

// Removes all entries from the given data source from the playlist.
void
CSimplePlaylist::DeleteEntriesFromDataSource(int iDataSourceID)
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
CSimplePlaylist::ReshuffleRandomEntries() 
{
    CSimplePlaylistEntry* pCurrent = (CSimplePlaylistEntry*)GetCurrentEntry();
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
            CSimplePlaylistEntry* pShuffleNext = m_pShuffleFirst;
            CSimplePlaylistEntry* pShufflePrev = 0;
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
}

// Returns a pointer to the entry at the specified zero-based index using the given playlist mode.
// If the index is out-of-range, then 0 is returned.
IPlaylistEntry*
CSimplePlaylist::GetEntry(int index, IPlaylist::PlaylistMode mode = NORMAL)
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
CSimplePlaylist::GetEntryIndex(const IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; it != m_slPlaylistEntries.GetEnd(); ++i, ++it)
        if (*it == pEntry)
            return i;

    return -1;
}

// Returns a pointer to the current entry.
IPlaylistEntry*
CSimplePlaylist::GetCurrentEntry()
{
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Sets the current entry pointer to the given record.
// If the entry isn't in the list, then the current entry pointer is set to 0.
IPlaylistEntry*
CSimplePlaylist::SetCurrentEntry(IPlaylistEntry* pEntry)
{
    m_itCurrentEntry = FindEntryByEntry(pEntry);
    return m_itCurrentEntry != m_slPlaylistEntries.GetEnd() ? *m_itCurrentEntry : 0;
}

// Returns a pointer to the next entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CSimplePlaylist::GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode mode)
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
CSimplePlaylist::SetNextEntry(PlaylistMode mode)
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
CSimplePlaylist::PlaylistIterator
CSimplePlaylist::GetNextEntry(PlaylistIterator it, PlaylistMode mode)
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
}

// Returns a pointer to the previous entry in the playlist as determined by the mode.
// Returns 0 if the end of the list was reached and the play mode isn't repeating.
IPlaylistEntry*
CSimplePlaylist::GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode mode)
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
CSimplePlaylist::SetPreviousEntry(PlaylistMode mode)
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
CSimplePlaylist::PlaylistIterator
CSimplePlaylist::GetPreviousEntry(PlaylistIterator it, PlaylistMode mode)
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
}

// Returns an iterator that points to the entry at the specified index.
// If the index is out-of-range, then the iterator points to the end of the list (i.e., 0).
CSimplePlaylist::PlaylistIterator
CSimplePlaylist::FindEntryByIndex(int index)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (int i = 0; (i < index) && (it != m_slPlaylistEntries.GetEnd()); ++it, ++i)
        ;
    return it;
}

// Returns an iterator that points to the given playlist entry.
// If the entry isn't in the list, then the iterator points to the end of the list (i.e., 0).
CSimplePlaylist::PlaylistIterator
CSimplePlaylist::FindEntryByEntry(IPlaylistEntry* pEntry)
{
    PlaylistIterator it = m_slPlaylistEntries.GetHead();
    for (; (it != m_slPlaylistEntries.GetEnd()) && (*it != pEntry); ++it)
        ;
    return it;
}

// Removes the entry from the list and deletes it.
void
CSimplePlaylist::DeleteEntry(PlaylistIterator it)
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
    }
}

// Finds a new place to put the entry in the shuffled list.
void
CSimplePlaylist::ShuffleEntry(CSimplePlaylistEntry* pEntry)
{
    int iShuffleIndex = rand() % GetSize();

    CSimplePlaylistEntry* pShuffleNext = m_pShuffleFirst;
    CSimplePlaylistEntry* pShufflePrev = 0;
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

//////////////////////////////////////////////////////////////////////////////////////////
//	CSimplePlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////

CSimplePlaylistEntry::CSimplePlaylistEntry(CSimplePlaylist* pPlaylist, IMediaContentRecord* pContentRecord)
    : m_pPlaylist(pPlaylist),
    m_pContentRecord(pContentRecord),
    m_pShufflePrev(0),
    m_pShuffleNext(0)
{
}

CSimplePlaylistEntry::~CSimplePlaylistEntry()
{
}
