//
// DJPlaylist.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef DJPLAYLIST_H_
#define DJPLAYLIST_H_

#include <playlist/common/Playlist.h>
#include <util/datastructures/SimpleList.h>

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJPlaylist
//////////////////////////////////////////////////////////////////////////////////////////

class CDJPlaylistEntry;

// CDJPlaylist
//! This is an extension of the SimplePlaylist with two major changes:
//!     - A "dirty bit" for tracking modifications
//!     - A faster random algorithm (to be done)
class CDJPlaylist : public IPlaylist
{
  public:
    CDJPlaylist(const char* szPlaylistName);
    ~CDJPlaylist();

    //! Returns the playlist's name.
    const char* GetName() const;

    //! Clears the playlist and frees up its memory.
    void Clear();

    //! Returns the number of entries in the playlist.
    int GetSize();

    //! Returns true if the playlist is empty, false otherwise.
    bool IsEmpty() const;

    //! Adds an entry to the end of the playlist.
    //! Returns a pointer to the new playlist entry.
    IPlaylistEntry* AddEntry(IMediaContentRecord* pContentRecord);

    //! Inserts an entry to the playlist at the specified zero-based index.
    //! Returns a pointer to the new playlist entry.
    IPlaylistEntry* InsertEntry(IMediaContentRecord* pContentRecord, int index);

    //! Adds all entries from the content record list to the end of the playlist.
    void AddEntries(MediaRecordList& records);

    //! Removes an entry from the playlist.
    void DeleteEntry(IPlaylistEntry* pEntry);

    //! Removes all entries from the given data source from the playlist.
    void DeleteEntriesFromDataSource(int iDataSourceID);

    //! Reshuffles the random links in the playlist.  The current entry will be set as the first
    //! random entry.
    void ReshuffleRandomEntries();

    //! Returns a pointer to the entry at the specified zero-based index using the given playlist mode.
    //! If the index is out-of-range, then 0 is returned.
    IPlaylistEntry* GetEntry(int index, PlaylistMode mode = NORMAL);

    //! Returns the zero-based index of the given playlist entry.
    //! Returns -1 if the entry is not in the playlist.
    int GetEntryIndex(const IPlaylistEntry* pEntry);

    //! Returns a pointer to the current entry.
    IPlaylistEntry* GetCurrentEntry();

    //! Sets the current entry pointer to the given record.
    //! If the entry isn't in the list, then the current entry pointer is set to 0.
    IPlaylistEntry* SetCurrentEntry(IPlaylistEntry* pEntry);

    //! Returns a pointer to the next entry in the playlist as determined by the mode.
    //! Returns 0 if the end of the list was reached and the play mode isn't repeating.
    IPlaylistEntry* GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode mode);

    //! Sets the current entry to the next entry in the playlist as determined by the mode.
    //! Returns a pointer to the new current entry, or 0 if the end of the list was reached
    //! (and the play mode isn't repeating).
    IPlaylistEntry* SetNextEntry(PlaylistMode mode);

    //! Returns a pointer to the previous entry in the playlist as determined by the mode.
    //! Returns 0 if the end of the list was reached and the play mode isn't repeating.
    IPlaylistEntry* GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode mode);

    //! Sets the current entry to the previous entry in the playlist as determined by the mode.
    //! Returns a pointer to the new current entry, or 0 if the end of the list was reached
    //! (and the play mode isn't repeating).
    IPlaylistEntry* SetPreviousEntry(PlaylistMode mode);

    //! Sets the dirty bit.
    void SetDirty(bool bDirty)
        { m_bDirty = bDirty; }

    //! Returns true if the playlist has been modified since the dirty bit was cleared.
    bool IsDirty() const
        { return m_bDirty; }

    //! Clears all entries that start with the given URL.
    void ClearURL(const char* szURL);

    typedef SimpleList<CDJPlaylistEntry*> PlaylistEntryList;
    typedef SimpleListIterator<CDJPlaylistEntry*> PlaylistIterator;

    const PlaylistEntryList* GetEntryList() const { return &m_slPlaylistEntries; }

  private:

    PlaylistEntryList       m_slPlaylistEntries;
    PlaylistIterator        m_itCurrentEntry;
    CDJPlaylistEntry*   m_pShuffleFirst;    // Pointer to the first entry in the shuffled playlist

    bool    m_bDirty;   // Set to true when the playlist changes.

    // Returns an iterator that points to the entry at the specified index.
    // If the index is out-of-range, then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator FindEntryByIndex(int index);

    // Returns an iterator that points to the given playlist entry.
    // If the entry isn't in the list, then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator FindEntryByEntry(IPlaylistEntry* pEntry);

    // Adds an entry to the playlist but doesn't adjust the shuffle order.
    CDJPlaylistEntry* AddEntryInternal(IMediaContentRecord* pContentRecord);

    // Removes the entry from the list and deletes it.
    void DeleteEntry(PlaylistIterator it);

    // Finds a new place to put the entry in the shuffled list.
    void ShuffleEntry(CDJPlaylistEntry* pEntry);

    // Returns an iterator that points to the next entry in the playlist as determined by the mode.
    // If the end of the list was reached and the play mode isn't repeating
    // then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator GetNextEntry(PlaylistIterator it, PlaylistMode mode);

    // Returns an iterator that points to the previous entry in the playlist as determined by the mode.
    // If the end of the list was reached and the play mode isn't repeating
    // then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator GetPreviousEntry(PlaylistIterator it, PlaylistMode mode);

};

//////////////////////////////////////////////////////////////////////////////////////////
//	CDJPlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////


// CDJPlaylistEntry
//! CDJPlaylist consists of CDJPlaylistEntry objects, which
//! define the necessary information for a playlist entry in this
//! derived playlist. This is not part of the SDK proper, and is provided
//! as a demonstration of an implementation of IPlaylistEntry.
class CDJPlaylistEntry : public IPlaylistEntry
{
  public:

    IMediaContentRecord* GetContentRecord() const
        { return m_pContentRecord; }

    // Returns the index of this entry in the playlist.
    int GetIndex() const
        { return m_pPlaylist->GetEntryIndex(this); }

    friend class CDJPlaylist;

  private:

    CDJPlaylistEntry(CDJPlaylist* pPlaylist, IMediaContentRecord* pContentRecord);
    ~CDJPlaylistEntry();

    CDJPlaylist*        m_pPlaylist;
    IMediaContentRecord*    m_pContentRecord;
    CDJPlaylistEntry*   m_pShufflePrev; // Pointer to the previous entry in the shuffled playlist
    CDJPlaylistEntry*   m_pShuffleNext; // Pointer to the next entry in the shuffled playlist
};

//@}


#endif	// DJPLAYLIST_H_
