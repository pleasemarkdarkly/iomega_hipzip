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

// how far the prag rand context will extend into the future and past.
#define PRAGMATIC_RANDOM_CONTEXT_RADIUS (4)
// future, past, plus the current entry.
#define PRAGMATIC_RANDOM_CONTEXT_SIZE_MAX (2*PRAGMATIC_RANDOM_CONTEXT_RADIUS+1)

struct tEntryMetadataMapping;
class CPogoPlaylistEntry;
typedef int QCompareDataFunction(const void* a, const void* b);

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
        REPEAT_RANDOM_ALBUM,
        PRAGMATIC_RANDOM,    /* Jump to a random track on each transition, without no full shuffling.  
                           No metric to determine playlist ending, so repeats by definition. */
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
    bool IsModeRepeating();

    // returns the track start time of the entry passed in.  in album order modes, this will be
    // relative to the start of the album, as currently ordered.
    int GetTrackStartTime(CPogoPlaylistEntry* pEntry);

    // returns the duration of the album of which the entry passed in is a member.
    int GetAlbumDuration(CPogoPlaylistEntry* pEntry);

    int GetRandomNumberSeed();
    void SetRandomNumberSeed(int nSeed);

    IPlaylistEntry* FindEntryByContentRecord(IContentRecord* cr);

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

    // order the sort links by random album.  ie, albums are selected at random, and then the 
    // whole album is played in album order.
    void SortEntriesByRandomAlbumOrder();
    
    // quick metadata sorting support    
    void InitMapToNativeOrdering(tEntryMetadataMapping* map, int nMappings);
    void SetMapToAlbumPositionAndTitle(tEntryMetadataMapping* map, int nMappings);
    void SetMapToAlbumName(tEntryMetadataMapping* map, int nMappings);
    void SortMapByString(tEntryMetadataMapping* map, int nMappings);
    void SortMapByInteger(tEntryMetadataMapping* map, int nMappings);
    void RandomizeOrderOfAlbums();

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

    /* pragmatic random play order */
    // choose a random entry
    PlaylistIterator GetNextPragRandEntry(PlaylistIterator itFrom);
    // back up in pragmatic random history
    PlaylistIterator GetPrevPragRandEntry(PlaylistIterator itFrom);
    // move the current entry iterator within the prag rand context list
    void SetNextPragRandEntry();
    void SetPrevPragRandEntry();
    // generate an ordering context around the current entry
    void InitPragRandContext(PlaylistIterator itCurrent);
    void SetCurrentPragRandDynamically();
    int GenerateNewPragRandIndex();
    void AddFuturemostPragRand();
    void AddPastmostPragRand();

    PlaylistEntryList m_slPragRandContext;
    SimpleList<int> m_slContextIndexes;
    PlaylistIterator m_itPragRandEntry;
    // there is a max radius that comes into play for longer lists, but the radius has to be 
    // dynamically adapted to smaller lists, to allow lack of duplication yet continued randomness
    int m_nContextRadius;
    // if the playlist is sufficiently short, then essentially the context won't be maintained, 
    // but more or less simulated.  the get next/prev functions will report the native ordering,
    // and the set next/prev fns will do a repeat-1 disallowed jump.  this should be ok since buffering
    // will buffer all the tracks anyway, although with long tracks this won't be too hot.  but oh well.
    bool m_bPragRandShortCircuit;
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
