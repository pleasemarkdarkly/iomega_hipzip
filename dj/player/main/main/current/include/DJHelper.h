//
// DJHelper.h
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//

#ifndef DJHELPER_H_
#define DJHELPER_H_

#include <cyg/kernel/kapi.h>
#include <util/eresult/eresult.h>

class IPlaylistEntry;

//! Tries to set the next song in the playlist.  If that fails it tries
//! to set the next song, and the next, until it finds a good track or until
//! the end of the playlist is reached.
//! Each track's info is displayed in the UI before being set, so the user
//! can tell what the player is doing.
//! Tracks that can't be set are removed from the playlist.
//! If the DJ is currently ripping then tracks that have already been copied
//! to the HD are skipped.
//! If the end of the playlist is reached without a good track being found,
//! and if bBacktrackIfNeeded is true and the DJ is not ripping, then the playlist
//! will be scanned for previous tracks to set.
ERESULT DJNextTrack(bool bBacktrackIfNeeded = false);

//! Tries to set the previous song in the playlist.  If that fails it tries
//! to set the previous song, and the previous, until it finds a good track
//! or until the beginning of the playlist is reached.
//! Each track's info is displayed in the UI before being set, so the user
//! can tell what the player is doing.
//! Tracks that can't be set are removed from the playlist.
//! If the DJ is currently ripping then tracks that have already been copied
//! to the HD are skipped.
//! If the end of the playlist is reached without a good track being found,
//! and if bBacktrackIfNeeded is true and the DJ is not ripping, then the playlist
//! will be scanned for next tracks to set.
ERESULT DJPreviousTrack(bool bBacktrackIfNeeded = false);

//! Tries to set the current song in the playlist.  If that fails it tries
//! to set the next song, and the next, until it finds a good track or until
//! the end of the playlist is reached.
//! Each track's info is displayed in the UI before being set, so the user
//! can tell what the player is doing.
//! Tracks that can't be set are removed from the playlist.
//! If the DJ is currently ripping then tracks that have already been copied
//! to the HD are skipped.
//! If the end of the playlist is reached without a good track being found,
//! and if bBacktrackIfNeeded is true and the DJ is not ripping, then the playlist
//! will be scanned for previous tracks to set.
ERESULT DJSetCurrentOrNext(bool bBacktrackIfNeeded = false);

//! Tries to set the current song in the playlist.  If that fails it tries
//! to set the previous song, and the previous, until it finds a good track
//! or until the beginning of the playlist is reached.
//! Each track's info is displayed in the UI before being set, so the user
//! can tell what the player is doing.
//! Tracks that can't be set are removed from the playlist.
//! If the DJ is currently ripping then tracks that have already been copied
//! to the HD are skipped.
//! If the end of the playlist is reached without a good track being found,
//! and if bBacktrackIfNeeded is true and the DJ is not ripping, then the playlist
//! will be scanned for next tracks to set.
ERESULT DJSetCurrentOrPrevious(bool bBacktrackIfNeeded = false);

//! Returns TRUE if the keycode button has been pressed within the last # of ticks 
bool DebounceButton(unsigned int keycode, cyg_tick_count_t ticks);

//! Called when changes are made to the registry but not saved.
//! Marks the registry as dirty -- when playback stops, the registry will be committed.
void SetRegistryDirty();

//! Called when playback stops.  Commits the content manager and the registry, if needed.
//void CommitUpdates(IUserInterface* pUserInterface);
void CommitUpdates();

//! Commits the database and registry if they're dirty and we're not playing back or ripping.
void CommitUpdatesIfSafe();

#endif  // DJHELPER_H_
