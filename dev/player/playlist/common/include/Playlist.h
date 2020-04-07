//
// Playlist.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#ifndef PLAYLIST_H_
#define PLAYLIST_H_

#include <content/common/ContentManager.h>  // MediaRecordList

//////////////////////////////////////////////////////////////////////////////////////////
//	IPlaylist
//////////////////////////////////////////////////////////////////////////////////////////

class IPlaylistEntry;

//! The playlist class is an abstract base class that describes functions to access
//! a sequential list of content records.
class IPlaylist
{
public:

//! Used to determine how to traverse the list of entries.
typedef enum PlaylistMode {
        NORMAL = 0,     /*!< Use the normal sequence when determining the next track. \hideinitializer */
        RANDOM,         /*!< Use the random sequence when determining the next track. */
        REPEAT_ALL,     /*!< Use the normal sequence when determining the next track.
                             When the current track is the last playlist entry, the
                             next track becomes the first playlist entry.             */
        REPEAT_RANDOM,  /*!< Use the random sequence when determining the next track.
                             If the current track is the last entry in the random
                             sequence, use the first entry in the random sequence as
                             the next track.                                          */
        REPEAT_TRACK    /*!< Use the current track as the next track.                 */
    };

	virtual ~IPlaylist()
        { }

	//! Returns the playlist's name.
	virtual const char* GetName() const = 0;

	//! Clears the playlist and frees up its memory.
	virtual void Clear() = 0;

	//! Returns the number of entries in the playlist.
	virtual int GetSize() = 0;

	//! Returns true if the playlist is empty, false otherwise.
	virtual bool IsEmpty() const = 0;

	//! Adds an entry to the end of the playlist.
	//! Returns a pointer to the new playlist entry.
	virtual IPlaylistEntry* AddEntry(IMediaContentRecord* pContentRecord) = 0;

	//! Inserts an entry to the playlist at the specified zero-based index.
	//! Returns a pointer to the new playlist entry.
	virtual IPlaylistEntry* InsertEntry(IMediaContentRecord* pContentRecord, int index) = 0;

    //! Adds all entries from the content record list to the end of the playlist.
    virtual void AddEntries(MediaRecordList& records) = 0;

	//! Removes an entry from the playlist.
	virtual void DeleteEntry(IPlaylistEntry* pEntry) = 0;

    //! Removes all entries from the given data source from the playlist.
    virtual void DeleteEntriesFromDataSource(int iDataSourceID) = 0;

    //! Reshuffles the random links in the playlist.  The current entry will be set as the first
    //! random entry.
    virtual void ReshuffleRandomEntries() = 0;

    //! Returns a pointer to the entry at the specified zero-based index using the given playlist mode.
    //! If the index is out-of-range, then 0 is returned.
	virtual IPlaylistEntry* GetEntry(int index, PlaylistMode mode = NORMAL) = 0;

	//! Returns the zero-based index of the given playlist entry.
    //! Returns -1 if the entry is not in the playlist.
	virtual int GetEntryIndex(const IPlaylistEntry* pEntry) = 0;

	//! Returns a pointer to the current entry.
	virtual IPlaylistEntry* GetCurrentEntry() = 0;

	//! Sets the current entry pointer to the given record.
	//! If the entry isn't in the list, then the current entry pointer is set to 0.
	virtual IPlaylistEntry* SetCurrentEntry(IPlaylistEntry* pEntry) = 0;

	//! Returns a pointer to the next entry in the playlist as determined by the mode.
    //! Returns 0 if the end of the list was reached and the play mode isn't repeating.
	virtual IPlaylistEntry* GetNextEntry(IPlaylistEntry* pEntry, PlaylistMode mode) = 0;

	//! Sets the current entry to the next entry in the playlist as determined by the mode.
    //! Returns a pointer to the new current entry, or 0 if the end of the list was reached
    //! (and the play mode isn't repeating).
	virtual IPlaylistEntry* SetNextEntry(PlaylistMode mode) = 0;

	//! Returns a pointer to the previous entry in the playlist as determined by the mode.
    //! Returns 0 if the end of the list was reached and the play mode isn't repeating.
	virtual IPlaylistEntry* GetPreviousEntry(IPlaylistEntry* pEntry, PlaylistMode mode) = 0;

	//! Sets the current entry to the previous entry in the playlist as determined by the mode.
    //! Returns a pointer to the new current entry, or 0 if the end of the list was reached
    //! (and the play mode isn't repeating).
	virtual IPlaylistEntry* SetPreviousEntry(PlaylistMode mode) = 0;

};


//////////////////////////////////////////////////////////////////////////////////////////
//	IPlaylistEntry
//////////////////////////////////////////////////////////////////////////////////////////

//! The playlist entry class is used to access the individual content records
//! that comprise the playlist.
class IPlaylistEntry
{
public:

	virtual ~IPlaylistEntry()
        { }

    virtual IMediaContentRecord* GetContentRecord() const = 0;

    //! Returns the index of this entry in the playlist.
    virtual int GetIndex() const = 0;

};


#endif	// PLAYLIST_H_
