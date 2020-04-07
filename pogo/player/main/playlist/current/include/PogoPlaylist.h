#ifndef POGOPLAYLIST_H_
#define POGOPLAYLIST_H_

#include <playlist/common/Playlist.h>
#include <util/datastructures/SimpleList.h>

//////////////////////////////////////////////////////////////////////////////////////////
//	CPogoPlaylist
//  This version of the playlist exists primarily to offer a richer set of PlaylistModes
//  than the basic IPlaylist interface provides for.  Since the interface has specific 
//  reference to the more basic PlaylistMode enum, I'll be doing a bit of casting to fit
//  into the interface.
//////////////////////////////////////////////////////////////////////////////////////////

class CPogoPlaylistEntry;

class CPogoPlaylist : public IPlaylist
{
public:
    typedef enum PogoPlaylistMode {
        NORMAL = 0,     /*!< Use the normal sequence when determining the next track. \hideinitializer */
        RANDOM,         /*!< Use the random sequence when determining the next track. */
        REPEAT_ALL,     /*!< Use the normal sequence when determining the next track.
                             When the current track is the last playlist entry, the
                             next track becomes the first playlist entry.             */
        REPEAT_RANDOM,  /*!< Use the random sequence when determining the next track.
                             If the current track is the last entry in the random
                             sequence, use the first entry in the random sequence as
                             the next track.                                          */
        REPEAT_TRACK,    /*!< Use the current track as the next track.                 */
        ALBUM,
        RANDOM_ALBUM,
        REPEAT_ALBUM,
        RANDOM_REPEAT_ALBUM
    };
    CPogoPlaylist(const char* szPlaylistName);
	~CPogoPlaylist();

	// Returns the playlist's name.
	const char* GetName() const;

	// Clears the playlist and frees up its memory.
	void Clear();

	// Returns the number of entries in the playlist.
	int GetSize();

	// Returns true if the playlist is empty, false otherwise.
	bool IsEmpty() const;

	// Adds an entry to the end of the playlist.
	// Returns a pointer to the new playlist entry.
	IPlaylistEntry* AddEntry(IMediaContentRecord* pContentRecord);

	// Inserts an entry to the playlist at the specified zero-based index.
	// Returns a pointer to the new playlist entry.
	IPlaylistEntry* InsertEntry(IMediaContentRecord* pContentRecord, int index);

    // Adds all entries from the content record list to the end of the playlist.
    void AddEntries(MediaRecordList& records);

	// Removes an entry from the playlist.
	void DeleteEntry(IPlaylistEntry* pEntry);

    // Removes all entries from the given data source from the playlist.
    void DeleteEntriesFromDataSource(int iDataSourceID);

    // Resorts the random links in the playlist.  The current entry will be set as the first
    // sorted entry.
    // Exception: In pogo, on shuffle-album, the current album will be set as the first sorted
    // album, but the current entry will still be offset by it's album-position into the list.
    // To support keeping the current entry 
    void ReshuffleRandomEntries();

    // This member will interpret the current PogoPlaylistMode and use it to generate the correct
    // structure in the Sort links in the entries.
    void ResortSortEntries();

	// Returns a pointer to the entry at the specified index, or 0 if the index is out-of-range.
	IPlaylistEntry* GetEntry(int index, PlaylistMode mode);

	// Returns the zero-based index of the given playlist entry.
    // Returns -1 if the entry is not in the playlist.
	int GetEntryIndex(const IPlaylistEntry* pEntry);

	// Returns a pointer to the current entry.
	IPlaylistEntry* GetCurrentEntry();

	// Sets the current entry pointer to the given record.
	// If the entry isn't in the list, then the current entry pointer is set to 0.
	IPlaylistEntry* SetCurrentEntry(IPlaylistEntry* pEntry);

	// Returns a pointer to the next entry in the playlist as determined by the mode.
    // Returns 0 if the end of the list was reached and the play mode isn't repeating.
	IPlaylistEntry* GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode mode);

	// Sets the current entry to the next entry in the playlist as determined by the mode.
    // Returns a pointer to the new current entry, or 0 if the end of the list was reached
    // (and the play mode isn't repeating).
	IPlaylistEntry* SetNextEntry(PlaylistMode mode);

	// Returns a pointer to the previous entry in the playlist as determined by the mode.
    // Returns 0 if the end of the list was reached and the play mode isn't repeating.
	IPlaylistEntry* GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode mode);

    // return last entry in the list, according to play order.
    IPlaylistEntry* GetTailEntry();
    // return first entry in the list, according to play order.
    IPlaylistEntry* GetHeadEntry();

	// Sets the current entry to the previous entry in the playlist as determined by the mode.
    // Returns a pointer to the new current entry, or 0 if the end of the list was reached
    // (and the play mode isn't repeating).
	IPlaylistEntry* SetPreviousEntry(PlaylistMode mode);

    // Set the PogoPlaylistMode to be used until further notice.  This is necessary to support
    // custom sorting in AddEntries, which has no mode parameter.
    void SetPogoPlaylistMode(PogoPlaylistMode mode);

    // Get the PogoPlaylistMode currently in use.
    PogoPlaylistMode GetPogoPlaylistMode();

    bool IsModeAlbumBased();

    // returns the track start time of the entry passed in.  in album order modes, this will be
    // relative to the start of the album, as currently ordered.
    int GetTrackStartTime(CPogoPlaylistEntry* pEntry);

    // returns the duration of the album of which the entry passed in is a member.
    int GetAlbumDuration(CPogoPlaylistEntry* pEntry);

    int GetRandomNumberSeed();
    void SetRandomNumberSeed(int nSeed);

private:
    typedef SimpleList<CPogoPlaylistEntry*> PlaylistEntryList;
    typedef SimpleListIterator<CPogoPlaylistEntry*> PlaylistIterator;
    typedef bool (*PlaylistEntryLessThanFunction)(CPogoPlaylistEntry* left, CPogoPlaylistEntry* right);

    // place the sort links in playlist order just as a baseline so the links are a correct linked list.
    void SortToPlaylistOrder();
    
    // print out each entry's attributes, in the current sorted order.
    void PrintSortedPlaylistEntryAttributes(const char* szCaption);

    // use merge-sort to sort the playlist entries with the lessthan function supplied.
    void MergeSortEntries(PlaylistEntryLessThanFunction lt);
    
    // sort the entries with the lessthan function supplied.
    void SortEntries(PlaylistEntryLessThanFunction lt);
    
    // order the sort links in the entries to reflect an "album" ordering.  
    // albums should be grouped and arranged alphabetically.  tracks should
    // be ordered by album-position when available, and alphabetically otherwise.
    // this works out to a sort by name, then album position, then album name.
    void SortEntriesByAlbumOrder();

	// Returns an iterator that points to the entry at the specified index.
    // If the index is out-of-range, then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator FindEntryByIndex(int index);

	// Returns an iterator that points to the given playlist entry.
    // If the entry isn't in the list, then the iterator points to the end of the list (i.e., 0).
    PlaylistIterator FindEntryByEntry(IPlaylistEntry* pEntry);

    // Removes the entry from the list and deletes it.
    void DeleteEntry(PlaylistIterator it);

    // Finds a new place to put the entry in the shuffled list.
    void SortEntry(CPogoPlaylistEntry* pEntry);

	// Returns an iterator that points to the next entry in the playlist as determined by the mode.
    // If the end of the list was reached and the play mode isn't repeating
    // then the iterator points to the end of the list (i.e., 0).
	PlaylistIterator GetNextEntry(PlaylistIterator it, PlaylistMode mode);

	// Returns an iterator that points to the previous entry in the playlist as determined by the mode.
    // If the end of the list was reached and the play mode isn't repeating
    // then the iterator points to the end of the list (i.e., 0).
	PlaylistIterator GetPreviousEntry(PlaylistIterator it, PlaylistMode mode);

    enum PogoPlaylistMode   m_ePogoPlaylistMode;
    PlaylistEntryList       m_slPlaylistEntries;
    PlaylistIterator        m_itCurrentEntry;
    CPogoPlaylistEntry*     m_pSortHead;    // Pointer to the first entry in the sorted playlist

    // the last album we got a duration on, had this id.
    int m_nAlbumDurationAlbumKey;
    // the duration of the last album we looked up.
    int m_nAlbumDuration;
    // the current random number seed for playlist shuffling.
    int m_nRandomNumberSeed;

};

//////////////////////////////////////////////////////////////////////////////////////////
//	CPogoPlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////

class CPogoPlaylistEntry : public IPlaylistEntry
{
public:

    IMediaContentRecord* GetContentRecord() const
        { return m_pContentRecord; }

    // Returns the index of this entry in the playlist.
    int GetIndex() const
        { return m_pPlaylist->GetEntryIndex(this); }

friend class CPogoPlaylist;

private:

    CPogoPlaylistEntry(CPogoPlaylist* pPlaylist, IMediaContentRecord* pContentRecord);
    ~CPogoPlaylistEntry();

    CPogoPlaylist*        m_pPlaylist;
    IMediaContentRecord*    m_pContentRecord;
    CPogoPlaylistEntry*   m_pSortPrev; // Pointer to the previous entry in the sorted playlist
    CPogoPlaylistEntry*   m_pSortNext; // Pointer to the next entry in the sorted playlist
};


#endif	// POGOPLAYLIST_H_
