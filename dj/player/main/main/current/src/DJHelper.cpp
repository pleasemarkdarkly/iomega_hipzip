//
// DJHelper.cpp
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//

#include <main/main/DJHelper.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/Recording.h>
#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/playlist/djplaylist/DJPlaylist.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/Strings.hpp>
#include <util/datastructures/SimpleList.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( DJHELPER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DJHELPER );


#define MAX_URL_ERROR_COUNT 10


ERESULT
DJNextTrack(bool bBacktrackIfNeeded)
{
    DEBUGP( DJHELPER, DBGLEV_INFO, "djh:NT\n");

    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pNextEntry = pCurrentPlaylist->SetNextEntry(CPlayManager::GetInstance()->GetPlaylistMode());

        if (pNextEntry)
            return DJSetCurrentOrNext(bBacktrackIfNeeded);
        else
            return PM_PLAYLIST_END;
    }
    return PM_ERROR;
}

ERESULT
DJPreviousTrack(bool bBacktrackIfNeeded)
{
    DEBUGP( DJHELPER, DBGLEV_INFO, "djh:PT\n");

    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pPreviousEntry = pCurrentPlaylist->SetPreviousEntry(CPlayManager::GetInstance()->GetPlaylistMode());

        if (pPreviousEntry)
            return DJSetCurrentOrPrevious(bBacktrackIfNeeded);
        else
            return PM_PLAYLIST_END;
    }
    return PM_ERROR;
}

typedef struct
{
    const char*     szURL;      // Bogus URL in question.
    unsigned int    nErrors;    // Number of times this URL has failed.
} bad_url_t;

typedef SimpleList<bad_url_t> BadURLList;

// This function searches the list of bad URL for the given URL.
// If not found, it adds a new node to the list and starts tracking its error count.
// If found, it increments the error count.  If the count is past the error threshold then
// all tracks sharing that URL's IP are removed from the playlist.
// This function returns true if the playlist is scrubbed of the given URL, false otherwise.
// (Only network URLs are checked; local URLs are ignored.)
static bool AddBadURL(CDJPlaylist* pPlaylist, BadURLList& slURLs, const char* szBadURL)
{
    DEBUGP( DJHELPER, DBGLEV_INFO, "djh: AddBadURL: %s\n", szBadURL);

    // We only care about network tracks.
    if (strnicmp(szBadURL, "http://", 7))
    {
        DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: AddBadURL: not a net stream\n");
        return false;
    }

    char* pAddrEnd = strchr(szBadURL + 7, '/');
    if (!pAddrEnd)
    {
        DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: AddBadURL: can't locate IP address\n");
        return false;
    }

    for (SimpleListIterator<bad_url_t> it = slURLs.GetHead(); it != slURLs.GetEnd(); ++it)
    {
        if (!strnicmp(szBadURL, (*it).szURL, pAddrEnd - szBadURL))
        {
            if (++(*it).nErrors > MAX_URL_ERROR_COUNT)
            {
                DEBUGP( DJHELPER, DBGLEV_INFO, "djh: AddBadURL: max errors reached\n");

                char* szDelURL = new char[pAddrEnd - szBadURL + 1];
                strncpy(szDelURL, szBadURL, pAddrEnd - szBadURL);
                szDelURL[pAddrEnd - szBadURL] = '\0';
                pPlaylist->ClearURL(szDelURL);
                delete [] szDelURL;

                return true;
            }
            else
            {
                DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: AddBadURL: error count: %d\n", (*it).nErrors);
                return false;
            }
        }
    }

    DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: AddBadURL: adding new node\n");
    bad_url_t bad_url = { szURL : szBadURL, nErrors : 1 };
    slURLs.PushBack(bad_url);

    return false;
}

// Routine to clear the error count for a given url (host). This allows nonsequntial
// errors to be ignored on FML traffic
static bool ClearErrorCount( BadURLList& slURLs, const char* szURL )
{
    // We only care about network tracks.
    if (strnicmp(szURL, "http://", 7))
    {
        DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: ClearErrorCount: not a net stream\n");
        return false;
    }

    char* pAddrEnd = strchr(szURL + 7, '/');
    if (!pAddrEnd)
    {
        DEBUGP( DJHELPER, DBGLEV_TRACE, "djh: ClearErrorCount: can't locate IP address\n");
        return false;
    }
    for( SimpleListIterator<bad_url_t> it = slURLs.GetHead(); it != slURLs.GetEnd(); ++it )
    {
        if (!strnicmp(szURL, (*it).szURL, pAddrEnd - szURL))
        {
            // match
            (*it).nErrors = 0;
            return true;
        }
    }
    return false;
}



ERESULT
DJSetCurrentOrNext(bool bBacktrackIfNeeded)
{
    DEBUGP( DJHELPER, DBGLEV_INFO, "djh:SCNT\n");

    CPlayManager* pPM = CPlayManager::GetInstance();
    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
    CRecordingManager* pRM = CRecordingManager::GetInstance();
    CQuickBrowseMenuScreen* pQB = CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen();
    BadURLList slURLs;

    ERESULT res = PM_ERROR;

    bool bRipping = pRM->IsRipping();
    bool bPlay = (pPM->GetPlayState() == CMediaPlayer::PLAYING) || (pPM->GetPlayState() == CMediaPlayer::PAUSED);

    IPlaylist* pCurrentPlaylist = pPM->GetPlaylist();

    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pNextEntry = pCurrentPlaylist->GetCurrentEntry();
        IPlaylistEntry* pEnd = 0;

        if (pNextEntry)
        {
            do
            {
                pPS->SetTrackInfo(pNextEntry);

                // If ripping, skip tracks that can't be recorded.
                if (bRipping)
                {
                    if (pEnd == pNextEntry)
                    {
                        return PM_PLAYLIST_END;
                    }
                    else if (unsigned int uiMessage = pRM->CannotRecord(pNextEntry->GetContentRecord()))
                    {
                        if (pEnd == 0)
                            pEnd = pNextEntry;
                        pPS->SetMessageText(LS(uiMessage), CSystemMessageString::REALTIME_INFO);
                        
                        IPlaylistEntry* pCurEntry = pNextEntry;
                        pNextEntry = pCurrentPlaylist->SetNextEntry(pPM->GetPlaylistMode());

                        // Clear the error count on this url, since this is a track that is either a.
                        //  on the hard drive (which makes this call a relative nop), or b. on an fml
                        //  that we've already recorded from, which makes the errors to date non sequential
                        ClearErrorCount( slURLs, pCurEntry->GetContentRecord()->GetURL() );
                        
                        // Return if no entries are left in the playlist.
                        // This leaves the player without a song currently set.
                        // It's the responsibility of the caller to stop ripping and to try to set
                        // another song.
                        if (!pNextEntry)
                            return PM_PLAYLIST_END;

                        continue;
                    }
                }

                if( pRM->IsRadioStream( pNextEntry->GetContentRecord() ) ) {
                    diag_printf(" enable prebuffer progress\n");
                    pPS->EnablePrebufferProgress();
                }
                res = pPM->SetSong( pNextEntry );

                if (SUCCEEDED(res))
                {
                    if (bPlay)
                        pPM->Play();
                    return res;
                }
                else
                {
                    DEBUG(DJHELPER, DBGLEV_WARNING, "Unable to set track: %s\n", pNextEntry->GetContentRecord()->GetURL());
                    pPS->DisablePrebufferProgress();
                    // dc- Print something so the user knows why we didn't play that track
                    pPS->SetMessageText(LS(SID_CANT_OPEN_TRACK), CSystemMessageString::REALTIME_INFO);

                    // Remove the track from the playlist.
                    IPlaylistEntry* pCurEntry = pNextEntry;
                    pNextEntry = pCurrentPlaylist->SetNextEntry(pPM->GetPlaylistMode());

                    if (AddBadURL((CDJPlaylist*)pCurrentPlaylist, slURLs, pCurEntry->GetContentRecord()->GetURL()))
                    {
                        if (pCurrentPlaylist->GetEntryIndex(pNextEntry) == -1)
                            pNextEntry = pCurrentPlaylist->GetCurrentEntry();
                    }
                    else
                    {
                        if (pCurEntry == pNextEntry)
                            pNextEntry = 0;
                        int iDeletedEntryIndex = pCurEntry->GetIndex();
                        pCurrentPlaylist->DeleteEntry(pCurEntry);
                        pQB->SynchAfterPlaylistEntryDeletion(iDeletedEntryIndex);
                    }
                }

            } while (pNextEntry);

            if (bBacktrackIfNeeded)
            {
                // If a good track wasn't found, then try previous tracks.
                DEBUG(DJHELPER, DBGLEV_WARNING, "Unable to set current or next track, trying previous tracks\n");
                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(pCurrentPlaylist->GetSize() - 1));
                if (FAILED(DJSetCurrentOrPrevious(false)))
                {
                    DEBUG(DJHELPER, DBGLEV_WARNING, "No good files found\n");
                    // Make sure the QB screen is updated; also pull us out of qb
                    pQB->RefreshPlaylist();
                    pPS->HideMenus();
                
                    // This playlist is invalid.  Tell someone.
                    pPM->Stop();
                    pPS->DisplayNoContentScreen();
                    pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::INFO);
                    return PM_NO_GOOD_TRACKS;
                }
                else
                {
                    if (bPlay)
                        pPM->Play();
                }
            }

            // If we're still running along, and didn't set a track, and the playlist is now empty, throw some feedback
            // up and reset the playerscreen
            if( pCurrentPlaylist->IsEmpty() ) {
                DEBUG( DJHELPER, DBGLEV_WARNING, "No files in current playlist\n" );

                // Make sure the QB screen is updated; also pull us out of qb
                pQB->RefreshPlaylist();
                pPS->HideMenus();
                
                pPM->Stop();
                pPS->DisplayNoContentScreen();
                pPS->SetMessageText( LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::INFO );
                return PM_NO_GOOD_TRACKS;
            }
            
            return res;
        }
        else
            return PM_PLAYLIST_END;
    }
    return res;
}

ERESULT
DJSetCurrentOrPrevious(bool bBacktrackIfNeeded)
{
    DEBUGP( DJHELPER, DBGLEV_INFO, "djh:SCPT\n");

    CPlayManager* pPM = CPlayManager::GetInstance();
    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
    CRecordingManager* pRM = CRecordingManager::GetInstance();
    CQuickBrowseMenuScreen* pQB = CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen();
    BadURLList slURLs;

    ERESULT res = PM_ERROR;

    bool bRipping = pRM->IsRipping();
    bool bPlay = (pPM->GetPlayState() == CMediaPlayer::PLAYING) || (pPM->GetPlayState() == CMediaPlayer::PAUSED);

    IPlaylist* pCurrentPlaylist = pPM->GetPlaylist();

    if (pCurrentPlaylist)
    {
        IPlaylistEntry* pPreviousEntry = pCurrentPlaylist->GetCurrentEntry();
        IPlaylistEntry* pEnd = 0;

        if (pPreviousEntry)
        {
            do
            {
                pPS->SetTrackInfo(pPreviousEntry);

                // If ripping, skip tracks that can't be recorded.
                if (bRipping)
                {
                    if (pEnd == pPreviousEntry)
                    {
                        return PM_PLAYLIST_END;
                    }
                    else if (unsigned int uiMessage = pRM->CannotRecord(pPreviousEntry->GetContentRecord()))
                    {
                        if (pEnd == 0)
                            pEnd = pPreviousEntry;
                        pPS->SetMessageText(LS(uiMessage), CSystemMessageString::REALTIME_INFO);
                        pPreviousEntry = pCurrentPlaylist->SetPreviousEntry(pPM->GetPlaylistMode());
                        // Return if no entries are left in the playlist.
                        // This leaves the player without a song currently set.
                        // It's the responsibility of the caller to stop ripping and to try to set
                        // another song.
                        if (!pPreviousEntry)
                            return PM_PLAYLIST_END;

                        continue;
                    }
                }

                res = pPM->SetSong( pPreviousEntry );

                if (SUCCEEDED(res))
                {
                    if (bPlay)
                        pPM->Play();
                    return res;
                }
                else
                {
                    DEBUG(DJHELPER, DBGLEV_WARNING, "Unable to set track: %s\n", pPreviousEntry->GetContentRecord()->GetURL());
                    // dc- Print something so the user knows why we didn't play that track
                    pPS->SetMessageText(LS(SID_CANT_OPEN_TRACK), CSystemMessageString::REALTIME_INFO);
                    // Remove the track from the playlist.
                    IPlaylistEntry* pCurEntry = pPreviousEntry;
                    pPreviousEntry = pCurrentPlaylist->SetPreviousEntry(pPM->GetPlaylistMode());

                    if (AddBadURL((CDJPlaylist*)pCurrentPlaylist, slURLs, pCurEntry->GetContentRecord()->GetURL()))
                    {
                        if (pCurrentPlaylist->GetEntryIndex(pPreviousEntry) == -1)
                            pPreviousEntry = pCurrentPlaylist->SetPreviousEntry(pPM->GetPlaylistMode());
                    }
                    else
                    {
                        if (pCurEntry == pPreviousEntry)
                            pPreviousEntry = 0;
                        int iDeletedEntryIndex = pCurEntry->GetIndex();
                        pCurrentPlaylist->DeleteEntry(pCurEntry);
                        pQB->SynchAfterPlaylistEntryDeletion(iDeletedEntryIndex);
                    }
                }

            } while (pPreviousEntry);

            if (bBacktrackIfNeeded)
            {
                // If a good track wasn't found, then try previous tracks.
                DEBUG(DJHELPER, DBGLEV_WARNING, "Unable to set current or previous tracks, trying next tracks\n");
                pCurrentPlaylist->SetCurrentEntry(pCurrentPlaylist->GetEntry(0));
                if (FAILED(DJSetCurrentOrNext(false)))
                {
                    DEBUG(DJHELPER, DBGLEV_WARNING, "No good files found\n");
                    // This playlist is invalid.  Tell someone.
                    pPM->Stop();
                    pPS->DisplayNoContentScreen();
                    pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                    return PM_NO_GOOD_TRACKS;
                }
                else
                {
                    if (bPlay)
                        pPM->Play();
                }
            }

            return res;
        }
        else
            return PM_PLAYLIST_END;
    }
    return res;
}

// used for generalizing debouncing button presses
typedef struct debounce_button_s {
    unsigned int        keycode;
    cyg_tick_count_t    tick;
} debounce_button_t;
typedef SimpleList<debounce_button_t> DebounceButtonList;
typedef SimpleListIterator<debounce_button_t> DebounceButtonListIterator;

// returns TRUE if the keycode button has been pressed within the last # of ticks 
bool DebounceButton(unsigned int keycode, cyg_tick_count_t ticks)
{
    bool bDebounce = false;
    static DebounceButtonList s_lstButtons;
    cyg_tick_count_t tick = cyg_current_time();
    debounce_button_t button;
    button.keycode = keycode;
    button.tick = tick;

    if(s_lstButtons.IsEmpty())
        s_lstButtons.PushFront(button);
    else
    {
        // look for this keycode in our list
        DebounceButtonListIterator it = s_lstButtons.GetHead();
        while(it != s_lstButtons.GetEnd())
        {
            if((*it).keycode == keycode)
            {
                // check to see the difference in ticks
                if (abs(tick - (*it).tick) < ticks)
                    bDebounce = true;
                else
                    (*it).tick = tick;
                it = 0;
            }
            else if(it == s_lstButtons.GetTail())
            {
                // if we can't find it, make a new entry and return false
                s_lstButtons.PushBack(button);
                it = 0;
            }
            else 
                ++it;
        }
    }
    
    return bDebounce;
}

static bool s_bRegistryDirty = false;

// Called when changes are made to the registry but not saved.
// Marks the registry as dirty -- when playback stops, the registry will be committed.
void
SetRegistryDirty()
{
    s_bRegistryDirty = true;
}

// Called when playback stops.  Commits the content manager and the registry, if needed.
void
CommitUpdates()
{
    if (((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->CommitIfDirty())
    {
        // Save the content manager's new state.
        DEBUGP( DJHELPER, DBGLEV_INFO, "djh:Committed db\n" );
    }

    // Save the registry if changes have been made.
    if (s_bRegistryDirty)
    {
        CDJPlayerState::GetInstance()->SaveRegistry();
        s_bRegistryDirty = false;
    }
}

//! Commits the database and registry if they're dirty and we're not playing back or ripping.
void
CommitUpdatesIfSafe()
{
    if ((CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED ||
        CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::NOT_CONFIGURED) &&
        !CRecordingManager::GetInstance()->IsRipping())
    {
        CommitUpdates();
    }
}

