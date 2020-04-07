//
// LibraryMenuScreen.cpp: the first menu screen that a user sees for dj
// danb@fullplaymedia.com 02/07/02
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/LibraryEntryMenuScreen.h>
#include <main/ui/PlaylistConstraint.h>
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/YesNoScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/EditScreen.h>
#include <main/ui/InfoMenuScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>
#include <main/ui/Messages.h>

#include <core/playmanager/PlayManager.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <main/main/AppSettings.h>
#include <main/main/DJHelper.h>
#include <main/main/DJPlayerState.h>
#include <main/main/ProgressWatcher.h>
#include <main/main/Recording.h>
#include <main/playlist/djplaylist/DJPlaylist.h>
#include <main/ui/ContentDeleteEvents.h>
#include <main/ui/PlaylistLoadEvents.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datastream/fatfile/FatFile.h>
#include <main/metadata/metadatafiletag/MetadataFileTag.h>

#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/content/simplercontentmanager/SimplerContentManager.h>

#include <extras/idlecoder/IdleCoder.h>
#include <fs/fat/sdapi.h>
#include <main/main/FatHelper.h>

#ifndef NO_UPNP
#include <core/mediaplayer/MediaPlayer.h>
#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#include <main/iml/query/QueryResultManager.h>
#endif  // NO_UPNP

#ifdef DDOMOD_DJ_BUFFERING
#include <main/buffering/BufferInStream.h>
#endif

#include <util/utils/utils.h>
#include <util/debug/debug.h>

DEBUG_MODULE_S( DBG_LIBRARY_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_LIBRARY_SCREEN );  // debugging prefix : (26) lms

CLibraryMenuScreen* CLibraryMenuScreen::s_pLibraryMenuScreen = 0;

// If SAVE_PLAYLIST_ON_SELECT is defined, then the current playlist will be
// saved to file after a selection completes.
//#define SAVE_PLAYLIST_ON_SELECT

#define QUERY_BLOCK_SIZE    20

// If a query times out then try it again until QUERY_RETRY_COUNT is reached.
#define QUERY_RETRY_COUNT   3

// Number of (local) playlist records to process on each update.
// Smaller numbers make the system more responsive but take longer to load.
#define PLAYLIST_LOAD_CHUNK_SIZE    100

// Number of tracks to delete each pass through the UI.
#define DELETE_CHUNK_SIZE   30


extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();


// This is a singleton class.
CScreen*
CLibraryMenuScreen::GetLibraryMenuScreen()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:GetInstance\n");
	if (!s_pLibraryMenuScreen) {
		s_pLibraryMenuScreen = new CLibraryMenuScreen();
	}
	return s_pLibraryMenuScreen;
}

CLibraryMenuScreen::CLibraryMenuScreen()
	: CDynamicMenuScreen(NULL, SID_ALBUMS),
#ifndef NO_UPNP
    m_pCurrentQR(0),
    m_pMediaItemListQR(0),
    m_pMediaItemAppendQR(0),
    m_pRadioStationQR(0),
    m_iHoleStart(-1),
    m_iHoleEnd(-1),
    m_iCurrentQueryID(0),
    m_iPreviousHoleSize(0),
    m_iPreviousHoleStart(-1),
#endif // NO_UPNP
    m_iQueryItemsLeft(0),
    m_bLoadingLocalPlaylist(false),
    m_bCancelLocalPlaylist(false),
    m_bCancelContentDeletion(false),
    m_iGenreKey(CMK_ALL),
    m_iArtistKey(CMK_ALL),
    m_iAlbumKey(CMK_ALL),
    m_pszGenre(NULL),
    m_pszArtist(NULL),
    m_pszAlbum(NULL),
	m_bAppend(false),
    m_bInvalidLibrary(false),
    m_iIterIndex(-1),
    m_iCurrentSourceIndex(0),
    m_iGenreTopIndex(-1),
    m_iArtistTopIndex(-1),
    m_iAlbumTopIndex(-1),
    m_iTrackTopIndex(-1),
    m_iPlaylistTopIndex(-1),
    m_iRadioTopIndex(-1),
    m_iDesiredIndex(-1),
	m_eBrowseMode(ALBUM),
    m_eBrowseSource(CDJPlayerState::HD)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Ctor\n");
#ifndef NO_UPNP
    // this shouldn't fail, or we're all in BIG trouble.  :)
    m_pIMLManager = CIMLManager::GetInstance();
    m_pDJPlayerState = CDJPlayerState::GetInstance();
#endif // NO_UPNP
}

CLibraryMenuScreen::~CLibraryMenuScreen()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Dtor\n");
}

// Hides any visible menu screens.
void
CLibraryMenuScreen::HideScreen()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Hide\n");

    // stop all the current queries to save cycles for other fun things, like playing audio
    m_bDropQueryResponses = true;
    m_bDropMediaItemListQueryResponses = true;
    m_bDropRadioStationQueryResponses = true;
	
	CDynamicMenuScreen::HideScreen();

    // delete all the persitent titles
    m_pszGenre = SaveString(m_pszGenre, NULL);
    m_pszArtist = SaveString(m_pszArtist, NULL);
    m_pszAlbum = SaveString(m_pszAlbum, NULL);
}

// old compare function
/*
static bool
CompareKeyValueRecord(const cm_key_value_record_t& a, const cm_key_value_record_t& b)
{
    return tstricmp(a.szValue, b.szValue) <= 0;
}
*/

// Used for sorting a ContentKeyValueVector.
int QCompareKeyValueRecord( const void* a, const void* b)
{
    //char temp [128];
    //char temp2 [128];
    //DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "cmp %s to %s",TcharToChar(temp,((cm_key_value_record_t*)a)->szValue),TcharToChar(temp2,((cm_key_value_record_t*)b)->szValue)); 
    int ret = tstricmp( ((cm_key_value_record_t*)a)->szValue, ((cm_key_value_record_t*)b)->szValue);
    //DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, " => %d\n",ret); 
    return ret;
}


SIGNED
CLibraryMenuScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Msg\n");
	switch (Mesg.wType)
	{
	case PM_KEY:

		// always reset append functionality

		switch (Mesg.iData)
		{
        case IR_KEY_ADD:
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:AppendToPL\n");
                if (m_cItems > 0)
                {
                    m_bAppend = true;
                    ProcessMenuOption(CScrollingListScreen::GetHighlightedIndex());
                    m_bAppend = false;
                }
                return 0;
            }
        case IR_KEY_DELETE:
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:DelSel\n");
                DeleteItemInFocus();
                return 0;
            }

        case IR_KEY_CLEAR: 
            {
                // ignore clear.  the user can clear the playlist from the quickbrowse screen
                return 0;
            }
        case IR_KEY_MENU:
        case KEY_MENU:
            {
                GotoPreviousMenu();
                return 0;
            }
        case IR_KEY_EDIT:
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:EditSel\n");
                EditItemInFocus();
                return 0;
            }
        case IR_KEY_INFO:
            {
                ShowMenuItemInfo(GetHighlightedIndex());
                return 0;
            }
        case IR_KEY_ABC_UP:
            JumpToNextLetter();
            return 0;

        case IR_KEY_ABC_DOWN:
            JumpToPreviousLetter();
            return 0;

        case IR_KEY_UP:
        case KEY_UP:
        case IR_KEY_DOWN:
        case KEY_DOWN:
            m_iDesiredIndex = -1;
            break;
            
        default:
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:KeyNotUsed\n");
                break;
            }
		}
        break;

    case IOPM_METADATA_UPDATE_END:
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:UpdateDS %d\n", Mesg.iData);
        return 0;

    case IOPM_CANCEL_QUERIES:
        CancelPendingQueries();
        return 0;

    case IOPM_PLAYLIST_LOAD_BEGIN:
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:IOPM_PLAYLIST_LOAD_BEGIN: %d\n", Mesg.lData);
        m_bLoadingLocalPlaylist = true;
        m_bCancelLocalPlaylist = false;
        CAlertScreen::GetInstance()->ResetProgressBar(0, Mesg.lData);
        m_iQueryItemsLeft = Mesg.lData;
        return 0;

    case IOPM_PLAYLIST_LOAD_PROGRESS:
    {
        playlist_load_info_t* pLoadInfo = (playlist_load_info_t*)Mesg.pData;

        // See if the playlist load has been cancelled
        if (m_bCancelLocalPlaylist)
        {
            // Yep, free the remaining URLs and delete the event struct.
            for (int i = pLoadInfo->index; i < pLoadInfo->records.Size(); ++i)
                free(pLoadInfo->records[i].szURL);
            delete pLoadInfo;

            // Let's end this politely.
            CEventQueue::GetInstance()->PutEvent(EVENT_PLAYLIST_LOAD_END, (void*)0);

            return 1;
        }

        // Adjust thread priority so playback doesn't stutter.
        int nPrio = GetMainThreadPriority();
        SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

        MediaRecordList mrlTracks;
        IPlaylist* pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
        bool bSetSong = pPlaylist->IsEmpty();

        // Grab the next batch of tracks.
        bool bStopLoading = false;
        int iStopIndex = pLoadInfo->index + PLAYLIST_LOAD_CHUNK_SIZE;
        if (iStopIndex > pLoadInfo->records.Size())
        {
            iStopIndex = pLoadInfo->records.Size();
            bStopLoading = true;
        }
#ifdef MAX_PLAYLIST_TRACKS
        if (iStopIndex - pLoadInfo->index + pPlaylist->GetSize() > MAX_PLAYLIST_TRACKS)
        {
            iStopIndex = MAX_PLAYLIST_TRACKS - pPlaylist->GetSize() + pLoadInfo->index;
            bStopLoading = true;
            // Free memory of unused entries.
            for (int i = iStopIndex; i < pLoadInfo->records.Size(); ++i)
                free(pLoadInfo->records[i].szURL);
        }
#endif  // MAX_PLAYLIST_TRACKS
        for (int i = pLoadInfo->index; i < iStopIndex; ++i)
        {
            // Look up the record by its url.
            IMediaContentRecord* pRecord = CDJPlayerState::GetMediaContentRecord(&pLoadInfo->records[i]);
            free(pLoadInfo->records[i].szURL);
            if (pRecord)
                mrlTracks.PushBack(pRecord);
        }

        // Add the entries we found to the playlist.
        CPlayManager::GetInstance()->GetPlaylist()->AddEntries(mrlTracks);

        SetMainThreadPriority(nPrio);

        // If the playlist is empty, then set the first song.
        if (bSetSong)
        {
            if (FAILED(DJSetCurrentOrNext()))
            {
                // This batch of tracks failed.  Are there more to load?
                if (bStopLoading)
                {
                    // Nope, show the "No tracks found" message.
                    CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                    CPlayerScreen::GetPlayerScreen()->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                }
                else
                {
                    // Yep, show the "Loading" screen.
                    CPlayerScreen::GetPlayerScreen()->SetMessageText(LS(SID_LOADING_PLAYLIST), CSystemMessageString::REALTIME_INFO);
                    CPlayerScreen::GetPlayerScreen()->ClearTrack();
                    CPlayerScreen::GetPlayerScreen()->SetTrackText(LS(SID_LOADING));
                }
            }
            else
            {
                // Start playback on select or append if the playlist was empty.
                CPlayManager::GetInstance()->Play();
            }
        }

        // Have we reached the end?
        if (bStopLoading)
        {
            // Yep, delete the info struct and post a message to the system that loading has finished.
            delete pLoadInfo;
            CEventQueue::GetInstance()->PutEvent(EVENT_PLAYLIST_LOAD_END, (void*)0);
            m_iQueryItemsLeft = 0;
        }
        else
        {
            // Nope, update the current index and post a message to keep the load going.
            pLoadInfo->index = iStopIndex;
            m_iQueryItemsLeft = pLoadInfo->records.Size() - iStopIndex;
            CEventQueue::GetInstance()->PutEvent(EVENT_PLAYLIST_LOAD, Mesg.pData);
        }

        // Update progress.
        CAlertScreen::GetInstance()->UpdateProgressBar(iStopIndex);
        return 0;
    }

    case IOPM_PLAYLIST_LOAD_END:
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:IOPM_PLAYLIST_LOAD_END\n");

        if (!m_bAppendLocalPlaylist)
        {
            if (CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
            {
                // The playlist was empty, so tell the user.
                CPlayerScreen::GetPlayerScreen()->DisplayNoContentScreen();
                CPlayerScreen::GetPlayerScreen()->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
            }
        }
        else
            CAlertScreen::GetInstance()->HideScreen();

        m_bLoadingLocalPlaylist = false;
        m_bCancelLocalPlaylist = false;
        return 0;
    }

    case IOPM_DELETE_CONTENT:
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:IOPM_DELETE_CONTENT\n");
        CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
        int iDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

        content_delete_info_t* pDeleteInfo = (content_delete_info_t*)Mesg.pData;

        int iMenuIndex = GetHighlightedIndex();

        // Adjust thread priority so playback doesn't stutter.
        int nPrio = GetMainThreadPriority();
        SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

        bool bDone = m_bCancelContentDeletion || RemoveMediaRecords(pDeleteInfo);

        if (bDone)
        {
            switch(m_eBrowseMode)
            {

            case GENRE:
            {
                // Requery, since pointers in the database may be invalid now.
                m_MenuItems.Clear();
                pCM->GetGenres(m_MenuItems, m_iArtistKey, m_iAlbumKey, iDSID);
                if (m_MenuItems.Size() > 1)
                    m_MenuItems.QSort(QCompareKeyValueRecord);
                SetItemCount(m_MenuItems.Size());

                if (m_MenuItems.IsEmpty())
                {
                    GotoPreviousMenu();
                }
                else if (iMenuIndex >= m_MenuItems.Size())
                {
                    // select last item
                    m_iGenreTopIndex = m_MenuItems.Size() - 1;
                    --m_iTopIndex;
                }
                else
                    m_iGenreTopIndex = iMenuIndex;

                break;
            }
                
            case ARTIST:
            {
                // Requery, since pointers in the database may be invalid now.
                m_MenuItems.Clear();
                pCM->GetArtists(m_MenuItems, m_iAlbumKey, m_iGenreKey, iDSID);
                if (m_MenuItems.Size() > 1)
                    m_MenuItems.QSort(QCompareKeyValueRecord);
                SetItemCount(m_MenuItems.Size());

                if (m_MenuItems.IsEmpty())
                {
                    GotoPreviousMenu();
                }
                else if (iMenuIndex >= m_MenuItems.Size())
                {
                    // select last item
                    m_iArtistTopIndex = m_MenuItems.Size() - 1;
                    --m_iTopIndex;
                }
                else
                    m_iArtistTopIndex = iMenuIndex;

                break;
            }
                
            case ALBUM:
            {
                // Requery, since pointers in the database may be invalid now.
                m_MenuItems.Clear();
                pCM->GetAlbums(m_MenuItems, m_iArtistKey, m_iGenreKey, iDSID);
                if (m_MenuItems.Size() > 1)
                    m_MenuItems.QSort(QCompareKeyValueRecord);
                SetItemCount(m_MenuItems.Size());

                if (m_MenuItems.IsEmpty())
                {
                    GotoPreviousMenu();
                }
                else if (iMenuIndex >= m_MenuItems.Size())
                {
                    // select last item
                    m_iAlbumTopIndex = m_MenuItems.Size() - 1;
                    --m_iTopIndex;
                }
                else
                    m_iAlbumTopIndex = iMenuIndex;

                break;
            }
            }

            // Reset the thread priority.
            SetMainThreadPriority(nPrio);

            delete pDeleteInfo;
            // Save the content manager database if we're not playing audio.
            CommitUpdatesIfSafe();

            // synch the player screen in case we've deleted an item in playlist
            // and the current tracks playlist number has changed
            if (CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
                CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
            else
                CPlayerScreen::GetPlayerScreen()->RefreshCurrentTrackMetadata();

            CAlertScreen::GetInstance()->HideScreen();
        }
        else
        {
cyg_thread_delay(100);
            // Reset the thread priority.
            SetMainThreadPriority(nPrio);
        }


        return 0;
    }


    }
	return CDynamicMenuScreen::Message(Mesg);
}

#define TOUPPER(c) ( (c >= 'a' && c <= 'z') ? c-'a'+'A' : c )

// warning: this fn specific to the m_MenuItems datastructure
TCHAR CLibraryMenuScreen::FirstLetterOfMenuItemWithHoles(int nIndex)
{
    TCHAR cResult;
    if (nIndex < m_iHoleStart)
    {
        cResult = m_MenuItems[nIndex].szValue[0];
    }
    else if (nIndex == m_iHoleStart)
    {
        // apply the genre version generally, as they all start with 'Q' anyway.  this is slightly more safe than just return 'Q', in case
        // the prefix changes across the board.
        cResult = (LS(SID_QUERYING_GENRES))[0];
    }
    else
    {
        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
        cResult = m_MenuItems[nIndex + iHoleAdjustment].szValue[0];
    }
    return TOUPPER(cResult);
}

// warning: this fn specific to the m_miTracks datastructure
TCHAR CLibraryMenuScreen::FirstLetterOfTrackTitleWithHoles(int nIndex)
{
    TCHAR cResult;
    if (nIndex < m_iHoleStart)
    {
        cResult = m_miTracks[nIndex].szMediaTitle[0];
    }
    else if (nIndex == m_iHoleStart)
    {
        cResult = (LS(SID_QUERYING_TRACKS))[0];
    }
    else
    {
        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
        cResult = m_miTracks[nIndex + iHoleAdjustment].szMediaTitle[0];
    }
    return TOUPPER(cResult);
}

// specific to m_miRadioStations.
TCHAR CLibraryMenuScreen::FirstLetterOfRadioNameWithHoles(int nIndex)
{
    TCHAR cResult;
    if (nIndex < m_iHoleStart)
        cResult = m_miRadioStations[nIndex].szStationName[0];
    else if (nIndex == m_iHoleStart)
        cResult = (LS(SID_QUERYING_RADIO_STATIONS))[0];
    else
    {
        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
        cResult = m_miRadioStations[nIndex + iHoleAdjustment].szStationName[0];
    }
    return TOUPPER(cResult);
}

void CLibraryMenuScreen::JumpToNextLetter()
{
    // make sure there's something below the current entry
    int iCurrentIndex = m_iTopIndex+1;


    if (iCurrentIndex >= m_cItems-1)
    {
        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:already at end\n"); 
        // (epg,9/13/2002): wrap to the top, same as other menu navigators.
        m_iTopIndex = -1;
        ForceRedraw();
        return;
    }
    // determine the target index

    int nIndex = 0;
    switch(m_eBrowseMode)
	{
		case GENRE:
		case ARTIST:
		case ALBUM:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    TCHAR cStart = TOUPPER(m_MenuItems[iCurrentIndex].szValue[0]);
                    nIndex = iCurrentIndex;
                    TCHAR cWalker = TOUPPER(m_MenuItems[nIndex].szValue[0]);
                    while ((cWalker == cStart) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        cWalker = TOUPPER(m_MenuItems[nIndex].szValue[0]);
                    }
                    break;
                }
                case CDJPlayerState::FML:
                {       
                    TCHAR cStart = FirstLetterOfMenuItemWithHoles(iCurrentIndex);
                    // queue up the next entry
                    nIndex = iCurrentIndex+1;
                    TCHAR cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    // walk until done
                    while ((cStart == cWalker) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    }
                    break;
                }
            }
            break;
        }
		case PLAYLIST:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                    // (epg,7/3/2002): need to account for "Play Everything" entry, which throws off the comparison of m_iTopIndex to m_prlPlaylistMenuItems.Size.
                    DBASSERT( DBG_LIBRARY_SCREEN , ( (m_prlPlaylistMenuItems.Size()+1 > iCurrentIndex) ) , "lms:top index out of range\n"); 

                    TCHAR cStart = 0;
                    const char* szFilename;
                    // consider the Play Everything case seperately
                    if (m_iTopIndex == -1)
                    {
                        cStart = ((TCHAR*)LS(SID_PLAY_EVERYTHING))[0];
                        nIndex = 0;
                    }
                    else
                    {
                        // index 1 corresponds to the playlist menu item list head.
                        nIndex = 1;
                        while (nIndex < iCurrentIndex)
                        {
                            ++nIndex;
                            ++itPlaylist;
                        }
                        szFilename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                        cStart = TOUPPER(szFilename[0]);
                        // in the base case, we want to advance the iterator, but not in the special case above, where the "Next" element is really the head of the list
                        ++itPlaylist;
                    }
                    // queue up the next list item, as the first to be considered as the target.
                    ++nIndex;
                    szFilename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                    char cWalker = TOUPPER(szFilename[0]);
                    while ((cWalker == cStart) && (nIndex-1 < m_prlPlaylistMenuItems.Size()-1))
                    {
                        ++nIndex;
                        ++itPlaylist;
                        szFilename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                        cWalker = TOUPPER(szFilename[0]);
                    }
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 
                    break;
                }
                case CDJPlayerState::FML:
                {
                    TCHAR cStart = FirstLetterOfMenuItemWithHoles(iCurrentIndex);
                    // queue up the next entry
                    nIndex = iCurrentIndex+1;
                    TCHAR cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    // walk until done
                    while ((cStart == cWalker) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    }
                    break;
                }
            }
            break;
        }
        case TRACK:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    if( m_iIterIndex == -1 ) 
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:no track iter active!\n"); 
                        return;
                    }
                    // sync the iterator to the starting location
                    while ( m_iIterIndex > iCurrentIndex )
                    {
                        --m_iIterIndex;
                        --m_itTrack;
                    }
                    while ( m_iIterIndex < iCurrentIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itTrack;
                    }
                    // look up the title and peel off the first char
                    void* pVoid;
                    (*m_itTrack)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cStart = TOUPPER(((TCHAR*)pVoid)[0]);

                    // start off considering the next entry.
                    MediaRecordIterator itTrack = m_itTrack+1;
                    nIndex = m_iIterIndex + 1;
                    (*itTrack)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cWalker = TOUPPER(((TCHAR*)pVoid)[0]);

                    // loop until we hit the end of the list, or a new letter.
                    while ((cWalker == cStart) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        ++itTrack;
                        (*itTrack)->GetAttribute(MDA_TITLE,&pVoid);
                        cWalker = TOUPPER(((TCHAR*)pVoid)[0]);
                    }
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 
                    break;
                }
                case CDJPlayerState::FML:
                {       
                    // here, we walk a different data structure, and need to skip over the hole in the data that may exist.
                    // (the hole is in the middle of the datastructure, as we get data from both ends, filling to the center)
                    // also, we need to account for the hole-placeholder entry, which goes at hole-start.  
                    
                    // find the start letter, the first letter of the currently highlighted entry.
                    TCHAR cStart = FirstLetterOfTrackTitleWithHoles(iCurrentIndex);
                    // ug.  need to keep checking whether we're running into the hole...
                    nIndex = iCurrentIndex + 1;
                    TCHAR cWalker = FirstLetterOfTrackTitleWithHoles(nIndex);
                    while ((cWalker == cStart) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        cWalker = FirstLetterOfTrackTitleWithHoles(nIndex);
                    }

                    break;
                }
            }
            break;
        }
		case RADIO:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    // There's no browsing Radio while in HD or CD mode currently
                    /*

                    if( m_iIterIndex == -1 ) 
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:no playlist iter active!\n"); 
                        return;
                    }
                    // sync the iterator to the starting location
                    while ( m_iIterIndex > iCurrentIndex )
                    {
                        --m_iIterIndex;
                        --m_itRadioStation;
                    }
                    while ( m_iIterIndex < iCurrentIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itRadioStation;
                    }
                    // look up the title and peel off the first char
                    void* pVoid;
                    (*m_itRadioStation)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cStart = TOUPPER(((TCHAR*)pVoid)[0]);

                    // start off considering the next entry.
                    MediaRecordIterator itRadioStation = m_itRadioStation+1;
                    nIndex = m_iIterIndex + 1;
                    (*itRadioStation)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cWalker = TOUPPER(((TCHAR*)pVoid)[0]);

                    // loop until we hit the end of the list, or a new letter.
                    while ((cWalker == cStart) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        ++itRadioStation;
                        (*itRadioStation)->GetAttribute(MDA_TITLE,&pVoid);
                        cWalker = TOUPPER(((TCHAR*)pVoid)[0]);
                    }
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 

                    */

                    break;
                }
                case CDJPlayerState::FML:
                {
                    TCHAR cStart = FirstLetterOfRadioNameWithHoles(iCurrentIndex);
                    nIndex = iCurrentIndex+1;
                    TCHAR cWalker = FirstLetterOfRadioNameWithHoles(nIndex);
                    while ((cWalker == cStart) && (nIndex < m_cItems-1))
                    {
                        ++nIndex;
                        cWalker = FirstLetterOfRadioNameWithHoles(nIndex);
                    }
                    break;
                }
            }
            break;
        }
		default:
            DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:unhandled browse mode %d\n",m_eBrowseMode);
			break;
	}
    m_iTopIndex = nIndex-1;
    ForceRedraw();
    return;
}

void CLibraryMenuScreen::JumpToPreviousLetter()
{
    // make sure there's something below the current entry
    int iCurrentIndex = m_iTopIndex+1;

    if (iCurrentIndex <= 0)
    {
        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:already at start\n"); 
        // (epg,9/13/2002): wrap to the bottom, the last element. (I'll do it this way even though otherwise you
        // will always end up at the start of a letter, so it is entirely obvious that the user wrapped-under)
        // Also, this is easier to code :)
        m_iTopIndex = m_cItems-1-1; // 1 for count->index, and another for the top->middle_of_screen.
        ForceRedraw();
        return;
    }
    // determine the target index

    int nIndex = 0;
    switch(m_eBrowseMode)
	{
		case GENRE:
		case ARTIST:
		case ALBUM:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    nIndex = iCurrentIndex;

                    // first, if possible, back up one space.
                    if (nIndex > 0)
                        --nIndex;
                    TCHAR cStart = TOUPPER(m_MenuItems[nIndex].szValue[0]);
                    TCHAR cWalker = cStart;
                    // next, back up until we reach the previous letter from the current location.
                    while ((cWalker == cStart) && (nIndex > 0))
                    {
                        --nIndex;
                        cWalker = TOUPPER(m_MenuItems[nIndex].szValue[0]);
                    }
                    // if we've run into the top, and it's not one of our goal letters, then go back down one to the start of the [previous] letter.
                    if (cWalker != cStart)
                        ++nIndex;
                    break;
                }
                case CDJPlayerState::FML:
                {       
                    nIndex = iCurrentIndex;
                    if (nIndex > 0)
                        --nIndex;
                    TCHAR cStart = FirstLetterOfMenuItemWithHoles(nIndex);
                    TCHAR cWalker = cStart;
                    while ((cStart == cWalker) && (nIndex > 0))
                    {
                        --nIndex;
                        cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    }
                    if (cWalker != cStart)
                        ++nIndex;
                    break;
                }
            }
            break;
        }
		case PLAYLIST:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    // special case where we will never be able to back up the iterator, since we start from the head.
                    if (iCurrentIndex == 1)
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:just jump to zero.\n"); 
                        nIndex = 0;
                        break;
                    }

                    PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                    // (epg,7/3/2002): need to account for "Play Everything" entry, which throws off the comparison of m_iTopIndex to m_prlPlaylistMenuItems.Size.
                    DBASSERT( DBG_LIBRARY_SCREEN , ( (m_prlPlaylistMenuItems.Size()+1 > iCurrentIndex) ) , "lms:top index out of range\n"); 

                    TCHAR cStart = 0;
                    const char* szFilename;

                    // index 1 corresponds to the playlist menu item list head.
                    nIndex = 1;
                    while (nIndex < iCurrentIndex)
                    {
                        ++nIndex;
                        ++itPlaylist;
                    }
                    if (nIndex > 1)
                    {
                        --nIndex;
                        --itPlaylist;
                    }
                    szFilename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                    cStart = TOUPPER(szFilename[0]);
                    char cWalker = cStart;
                    while ((cWalker == cStart) && (nIndex > 1))
                    {
                        --nIndex;
                        --itPlaylist;
                        szFilename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                        cWalker = TOUPPER(szFilename[0]);
                    }
                    // in the base case, we need to bump it back down the start of the next (previous from first) letter.
                    if (cWalker != cStart)
                        ++nIndex;
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 
                    break;
                }
                case CDJPlayerState::FML:
                {
                    // (epg,9/13/2002): fml now has a play ev entry, so same special case handling.
                    if (iCurrentIndex == 1)
                    {
                        nIndex = 0;
                        break;
                    }
                    nIndex = iCurrentIndex;
                    if (nIndex > 1)
                        --nIndex;
                    TCHAR cStart = FirstLetterOfMenuItemWithHoles(nIndex);
                    // queue up the next entry
                    TCHAR cWalker = cStart;
                    // walk until done
                    while ((cStart == cWalker) && (nIndex > 1))
                    {
                        --nIndex;
                        cWalker = FirstLetterOfMenuItemWithHoles(nIndex);
                    }
                    if (cWalker != cStart)
                        ++nIndex;
                    break;
                }
            }
            break;
        }
        case TRACK:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    if( m_iIterIndex == -1 ) 
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:no track iter active!\n"); 
                        return;
                    }
                    // sync the iterator to the starting location
                    while ( m_iIterIndex > iCurrentIndex )
                    {
                        --m_iIterIndex;
                        --m_itTrack;
                    }
                    while ( m_iIterIndex < iCurrentIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itTrack;
                    }

                    // some local tracking variables
                    MediaRecordIterator itTrack = m_itTrack;
                    nIndex = m_iIterIndex;

                    // start from previous entry if we're not at the beginning already
                    if (nIndex > 0)
                    {
                        --itTrack;
                        --nIndex;
                    }

                    // look up the title and peel off the first char
                    void* pVoid;
                    (*itTrack)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cStart = TOUPPER(((TCHAR*)pVoid)[0]);
                    TCHAR cWalker = cStart;

                    // loop until we hit the end of the list, or a new letter.
                    while ((cWalker == cStart) && (nIndex > 0))
                    {
                        --nIndex;
                        --itTrack;
                        (*itTrack)->GetAttribute(MDA_TITLE,&pVoid);
                        cWalker = TOUPPER(((TCHAR*)pVoid)[0]);
                    }

                    // if that leaves us beyond cMiddle (the base case), bump it back up to begin of next.
                    if (cStart != cWalker)
                    {
                        ++itTrack;
                        ++nIndex;
                    }

                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 
                    break;
                }
                case CDJPlayerState::FML:
                {       
                    // here, we walk a different data structure, and need to skip over the hole in the data that may exist.
                    // (the hole is in the middle of the datastructure, as we get data from both ends, filling to the center)
                    // also, we need to account for the hole-placeholder entry, which goes at hole-start.  
                    
                    // find the start letter, the first letter of the currently highlighted entry.
                    nIndex = iCurrentIndex;
                    if (nIndex > 0)
                        --nIndex;
                    TCHAR cStart = FirstLetterOfTrackTitleWithHoles(nIndex);
                    // ug.  need to keep checking whether we're running into the hole...
                    TCHAR cWalker = cStart;
                    while ((cWalker == cStart) && (nIndex >0))
                    {
                        --nIndex;
                        cWalker = FirstLetterOfTrackTitleWithHoles(nIndex);
                    }
                    if (cStart != cWalker)
                        ++nIndex;
                    break;
                }
            }
            break;
        }
		case RADIO:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    // There's no browsing Radio while in HD or CD mode currently
                    /*

                    if( m_iIterIndex == -1 ) 
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:no playlist iter active!\n"); 
                        return;
                    }
                    // sync the iterator to the starting location
                    while ( m_iIterIndex > iCurrentIndex )
                    {
                        --m_iIterIndex;
                        --m_itRadioStation;
                    }
                    while ( m_iIterIndex < iCurrentIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itRadioStation;
                    }

                    if (m_iIterIndex > 0)
                    {
                        --m_iIterIndex;
                        --m_itRadioStation;
                    }
                    // look up the title and peel off the first char
                    void* pVoid;
                    (*m_itRadioStation)->GetAttribute(MDA_TITLE,&pVoid);
                    TCHAR cStart = TOUPPER(((TCHAR*)pVoid)[0]);
                    TCHAR cWalker = cStart;

                    // loop until we hit the end of the list, or a new letter.
                    while ((cWalker == cStart) && (nIndex > 0))
                    {
                        --nIndex;
                        --m_itRadioStation;
                        (*m_itRadioStation)->GetAttribute(MDA_TITLE,&pVoid);
                        cWalker = TOUPPER(((TCHAR*)pVoid)[0]);
                    }
                    if (cStart != cWalker)
                        ++nIndex;
                    
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_TRACE, "lms:jumping to index %d, letter %d\n",nIndex,cWalker); 

                    */

                    break;
                }
                case CDJPlayerState::FML:
                {
                    nIndex = iCurrentIndex;
                    if (nIndex > 0)
                        --nIndex;
                    TCHAR cStart = FirstLetterOfRadioNameWithHoles(nIndex);
                    TCHAR cWalker = cStart;
                    while ((cWalker == cStart) && (nIndex > 0))
                    {
                        --nIndex;
                        cWalker = FirstLetterOfRadioNameWithHoles(nIndex);
                    }
                    if (cStart != cWalker)
                        ++nIndex;
                    
                    break;
                }
            }
            break;
        }
		default:
            DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:unhandled browse mode %d\n",m_eBrowseMode);
			break;
	}
    m_iTopIndex = nIndex-1;
    ForceRedraw();
    return;

}

// Notification from the scrolling list that the list has scrolled up.
void
CLibraryMenuScreen::NotifyScrollUp()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:NtfyScrlUp\n");
    CDynamicMenuScreen::NotifyScrollUp();
}

// Notification from the scrolling list that the list has scrolled down.
void
CLibraryMenuScreen::NotifyScrollDown()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:NtfyScrlDwn\n");
#if 0
    switch(m_eBrowseMode)
	{
        case GENRE:
        case ARTIST:
        case ALBUM:
#ifndef NO_UPNP
        case PLAYLIST:
            if ((m_eBrowseSource == CDJPlayerState::FML) && m_pCurrentQR &&
                (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount()))
            {
                // Make sure the query is in range.
                if ((m_pCurrentQR->GetFilledItemCount() < (m_iTopIndex + QUERY_BLOCK_SIZE)) ||
                    (m_pCurrentQR->GetTotalItemCount() == -1))
                {
                    ContentKeyValueVector temp;
                    int iStartIndex = m_pCurrentQR->GetFilledItemCount() > 0 ? m_pCurrentQR->GetFilledItemCount() - 1 : 0;
                    m_pCurrentQR->GetValues(temp, iStartIndex, QUERY_BLOCK_SIZE, true);
                }
            }
#endif // NO_UPNP
            break;

        case TRACK:
#ifndef NO_UPNP
            if ((m_eBrowseSource == CDJPlayerState::FML) && m_pMediaItemListQR &&
                (m_pMediaItemListQR->GetFilledItemCount() < m_pMediaItemListQR->GetTotalItemCount()))
            {
                // Make sure the query is in range.
                if ((m_pMediaItemListQR->GetFilledItemCount() < (m_iTopIndex + QUERY_BLOCK_SIZE)) ||
                    (m_pMediaItemListQR->GetTotalItemCount() == -1))
                {
                    IMLMediaInfoVector temp;
                    int iStartIndex = m_pMediaItemListQR->GetFilledItemCount() > 0 ? m_pMediaItemListQR->GetFilledItemCount() - 1 : 0;
                    m_pMediaItemListQR->GetValues(temp, iStartIndex, QUERY_BLOCK_SIZE, true);
                }
            }
#endif // NO_UPNP
            break;

        case RADIO:
#ifndef NO_UPNP
            if ((m_eBrowseSource == CDJPlayerState::FML) && m_pRadioStationQR && 
                (m_pRadioStationQR->GetFilledItemCount() < m_pRadioStationQR->GetTotalItemCount()))
            {
                // Make sure the query is in range.
                if ((m_pRadioStationQR->GetFilledItemCount() < (m_iTopIndex + QUERY_BLOCK_SIZE)) ||
                    (m_pRadioStationQR->GetTotalItemCount() == -1))
                {
                    IMLRadioStationInfoVector temp;
                    int iStartIndex = m_pRadioStationQR->GetFilledItemCount() > 0 ? m_pRadioStationQR->GetFilledItemCount() - 1 : 0;
                    m_pRadioStationQR->GetValues(temp, iStartIndex, QUERY_BLOCK_SIZE, true);
                }
            }
#endif // NO_UPNP
            break;
        default:
            break;
    }
#endif // 0
    CDynamicMenuScreen::NotifyScrollDown();
}

bool 
CLibraryMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MenuItemHasSubMenu\n");
    switch(m_eBrowseMode)
    {
    case GENRE:
        return true;
    case ARTIST:
        return true;
    case ALBUM:
        return true;
    case TRACK:
        return false;
    case PLAYLIST:
        return false;
    case RADIO:
        return false;
    default:
        // question reality.
        return false;
    }
}

const TCHAR* 
CLibraryMenuScreen::MenuItemCaption(int iMenuIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MenuCap %d\n", iMenuIndex);
    if(m_bInvalidLibrary) {
        return LS(SID_EMPTY_STRING);
    }

    switch(m_eBrowseMode)
	{
		case GENRE:
        {
            switch (m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                    return m_MenuItems[iMenuIndex].szValue;
#ifndef NO_UPNP                    
                case CDJPlayerState::FML:
                {
                    if (iMenuIndex < m_iHoleStart)
                    {
                        return m_MenuItems[iMenuIndex].szValue;
                    }
                    else if (iMenuIndex == m_iHoleStart)
                    {
                        return BuildQueryingString(LS(SID_QUERYING_GENRES));
                    }
                    else
                    {
                        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                        int iAdjustedIndex  = iMenuIndex + iHoleAdjustment;
                        if (iAdjustedIndex > m_MenuItems.Size())
                            return LS(SID_EMPTY_STRING);
                        else
                            return m_MenuItems[iAdjustedIndex].szValue;
                    }
                }
#endif
            }
        }
		case ARTIST:
        {
            switch (m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                    return m_MenuItems[iMenuIndex].szValue;
#ifndef NO_UPNP                    
                case CDJPlayerState::FML:
                {
                    if (iMenuIndex < m_iHoleStart)
                    {
                        return m_MenuItems[iMenuIndex].szValue;
                    }
                    else if (iMenuIndex == m_iHoleStart)
                    {
                        return BuildQueryingString(LS(SID_QUERYING_ARTISTS));
                    }
                    else
                    {
                        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                        int iAdjustedIndex  = iMenuIndex + iHoleAdjustment;
                         if (iAdjustedIndex > m_MenuItems.Size())
                            return LS(SID_EMPTY_STRING);
                        else
                            return m_MenuItems[iAdjustedIndex].szValue;
                    }
                }
#endif                
            }
        }
		case ALBUM:
        {
            switch (m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                    return m_MenuItems[iMenuIndex].szValue;
#ifndef NO_UPNP                    
                case CDJPlayerState::FML:
                {
                    if (iMenuIndex < m_iHoleStart)
                    {
                        return m_MenuItems[iMenuIndex].szValue;
                    }
                    else if (iMenuIndex == m_iHoleStart)
                    {
                        return BuildQueryingString(LS(SID_QUERYING_ALBUMS));
                    }
                    else
                    {
                        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                        int iAdjustedIndex  = iMenuIndex + iHoleAdjustment;
                        if (iAdjustedIndex > m_MenuItems.Size())
                            return LS(SID_EMPTY_STRING);
                        else
                            return m_MenuItems[iAdjustedIndex].szValue;
                    }
                }
#endif
            }
        }

		case TRACK:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    if( m_iIterIndex == -1 ) {
                        return LS(SID_EMPTY_STRING);
                    }
                    while ( m_iIterIndex > iMenuIndex )
                    {
                        --m_iIterIndex;
                        --m_itTrack;
                    }
                    while ( m_iIterIndex < iMenuIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itTrack;
                    }
                    void* pData = 0;
                    (*m_itTrack)->GetAttribute(MDA_TITLE,&pData);
                    return (TCHAR*) pData;
                }
#ifndef NO_UPNP
                case CDJPlayerState::FML:
                    if (iMenuIndex < m_iHoleStart)
                    {
                        return m_miTracks[iMenuIndex].szMediaTitle;
                    }
                    else if (iMenuIndex == m_iHoleStart)
                    {
                        return BuildQueryingString(LS(SID_QUERYING_TRACKS));
                    }
                    else
                    {
                        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                        int iAdjustedIndex  = iMenuIndex + iHoleAdjustment;
                        if (iAdjustedIndex > m_miTracks.Size())
                            return LS(SID_EMPTY_STRING);
                        else
                            return m_miTracks[iAdjustedIndex].szMediaTitle;
                    }
#endif  // NO_UPNP
                case CDJPlayerState::LINE_IN:
                default:
                    return LS(SID_EMPTY_STRING);
            }
        }

        case PLAYLIST:
        {
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                    if ( iMenuIndex == 0 )
                        return LS(SID_PLAY_EVERYTHING);
                    else
                    {
                        int i = 0;
                        for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                            itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
                        {
                            if((iMenuIndex - 1) == i)
                            {
                                const char* temp = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                                
                                // strip extension
                                static TCHAR tcTemp[128];
                                int len = strlen(temp);
                                len = (len < 127) ? len - 4 : 127;
                                // NOTE: kinda scary, but apparently safe
                                return CharToTcharN(tcTemp, temp, len);
                            }
                            
                            i++;
                        }
                    }                        
                    break;
#ifndef NO_UPNP
                case CDJPlayerState::FML:
                    if ( iMenuIndex == 0 )
                        return LS(SID_PLAY_EVERYTHING);
                    else
                    {
                        --iMenuIndex;
                        if (iMenuIndex < m_iHoleStart)
                        {
                            return m_MenuItems[iMenuIndex].szValue;
                        }
                        else if (iMenuIndex == m_iHoleStart)
                        {
                            return BuildQueryingString(LS(SID_QUERYING_PLAYLISTS));
                        }
                        else
                        {
                            int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                            int iAdjustedIndex  = iMenuIndex + iHoleAdjustment;
                            if (iAdjustedIndex > m_MenuItems.Size())
                                return LS(SID_EMPTY_STRING);
                            else
                                return m_MenuItems[iAdjustedIndex].szValue;
                        }
                    }
#endif  // NO_UPNP

                default:
                    return LS(SID_EMPTY_STRING);
            }
        }

        case RADIO:
            switch(m_eBrowseSource)
            {
                case CDJPlayerState::HD:
                case CDJPlayerState::CD:
                {
                    while ( m_iIterIndex > iMenuIndex )
                    {
                        --m_iIterIndex;
                        --m_itRadioStation;
                    }
                    while ( m_iIterIndex < iMenuIndex ) 
                    {
                        ++m_iIterIndex;
                        ++m_itRadioStation;
                    }
                    void* pData = 0;
                    (*m_itRadioStation)->GetAttribute(MDA_TITLE,&pData);
                    return (TCHAR*) pData;
                }
#ifndef NO_UPNP
                case CDJPlayerState::FML:
                    if (iMenuIndex < m_iHoleStart)
                    {
                        return m_miRadioStations[iMenuIndex].szStationName;
                    }
                    else if (iMenuIndex == m_iHoleStart)
                    {
                        return BuildQueryingString(LS(SID_QUERYING_RADIO_STATIONS));
                    }
                    else
                    {
                        int iHoleAdjustment = (m_iHoleStart == -1) ? 0 : (m_iHoleEnd - (m_iHoleStart + 1));
                        return m_miRadioStations[iMenuIndex + iHoleAdjustment].szStationName;
                    }
#endif  // NO_UPNP
                case CDJPlayerState::LINE_IN:
                default:
                    return LS(SID_EMPTY_STRING);
            }

		default:
            // question reality.
            return NULL;
	}
}


const TCHAR*
CLibraryMenuScreen::MenuTitleCaption()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MenuTitle\n");
    switch(m_eBrowseMode)
    {
    case ARTIST:
        if(m_pszGenre != NULL)
            return(m_pszGenre);
        break;
    case ALBUM:
        if(m_pszArtist != NULL)
            return(m_pszArtist);
        break;
    case TRACK:
        if(m_pszAlbum != NULL)
            return(m_pszAlbum);
        break;
    default:
        break;
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Using default menu title caption\n");
    return LS(m_wScreenTitleSID);
}


// Called when there are no items in the menu.   
const TCHAR*
CLibraryMenuScreen::EmptyMenuCaption()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:EmptyMenuCaption\n");
    switch(m_eBrowseMode)
    {
    case GENRE:
        return LS(SID_NO_GENRES_AVAILABLE);
    case ARTIST:
        return LS(SID_NO_ARTISTS_AVAILABLE);
    case ALBUM:
        return LS(SID_NO_ALBUMS_AVAILABLE);
    case TRACK:
        return LS(SID_NO_TRACKS_AVAILABLE);
    case PLAYLIST:
        return LS(SID_NO_PLAYLISTS_AVAILABLE);
    case RADIO:
        return LS(SID_NO_RADIO_STATIONS_AVAILABLE);
    default:
        return LS(SID_EMPTY_STRING);
    }
}


void
CLibraryMenuScreen::ShowMenuItemInfo(int iMenuIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ShowMenuItemInfo\n");
    switch(m_eBrowseSource)
    {
    case CDJPlayerState::HD:
    case CDJPlayerState::CD:
    {
        // check to see if this is a valid menu index
        if (m_cItems <= iMenuIndex)
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "lib:ShowMenuItemInfo: No Info for Invalid List Item\n");
            return;
        }

        switch(m_eBrowseMode)
        {
        case GENRE:
            CInfoMenuScreen::GetInfoMenuScreen()->SetGenreInfo(m_MenuItems[iMenuIndex].iKey, m_eBrowseSource);
            CInfoMenuScreen::GetInfoMenuScreen()->SetParent(this);
            Add(CInfoMenuScreen::GetInfoMenuScreen());
            Presentation()->MoveFocusTree(CInfoMenuScreen::GetInfoMenuScreen());
            break;
        case ARTIST:
            CInfoMenuScreen::GetInfoMenuScreen()->SetArtistInfo(m_MenuItems[iMenuIndex].iKey, m_eBrowseSource);
            CInfoMenuScreen::GetInfoMenuScreen()->SetParent(this);
            Add(CInfoMenuScreen::GetInfoMenuScreen());
            Presentation()->MoveFocusTree(CInfoMenuScreen::GetInfoMenuScreen());
            break;
        case ALBUM:
            CInfoMenuScreen::GetInfoMenuScreen()->SetAlbumInfo(m_MenuItems[iMenuIndex].iKey, m_eBrowseSource);
            CInfoMenuScreen::GetInfoMenuScreen()->SetParent(this);
            Add(CInfoMenuScreen::GetInfoMenuScreen());
            Presentation()->MoveFocusTree(CInfoMenuScreen::GetInfoMenuScreen());
            break;
        case TRACK:
        {
            while ( m_iIterIndex > iMenuIndex )
            {
                --m_iIterIndex;
                --m_itTrack;
            }
            while ( m_iIterIndex < iMenuIndex ) 
            {
                ++m_iIterIndex;
                ++m_itTrack;
            }
            CInfoMenuScreen::GetInfoMenuScreen()->SetTrackInfo(*m_itTrack);
            CInfoMenuScreen::GetInfoMenuScreen()->SetParent(this);
            Add(CInfoMenuScreen::GetInfoMenuScreen());
            Presentation()->MoveFocusTree(CInfoMenuScreen::GetInfoMenuScreen());
            break;
        }
        case PLAYLIST:
        case RADIO:
        default:
            break;
        }
    }
    default:
        break;
    }
}


// A helper function to save a string locally.
// Used for saving the menu titles when drilling into submenus
// If dst isn't null, it deletes the data it's pointing to 
// It returns a pointer to the new string
TCHAR*
CLibraryMenuScreen::SaveString(TCHAR* dst, const TCHAR* src)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:SaveString\n");

    if(dst)
        free(dst);

    if(src)
        dst = tstrdup(src);
    else
        dst = NULL;

    return dst;
}


// Cancel any pending media item append queries and drop the results on the floor
void
CLibraryMenuScreen::CancelPendingQueries(bool bShowAlert)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:CancelPendingQueries\n");
    m_iQueryItemsLeft = 0;

    // if there's query already outstanding, kill it.
    if (m_pMediaItemAppendQR)
    {
        CQueryResultManager::GetInstance()->RemoveQueryResult(m_pMediaItemAppendQR);
        m_pMediaItemAppendQR = 0;

/* TODO: show the alert screen longer, don't return to the player screen until results have appeared.
        // Show an alert screen if needed.
        if (bShowAlert)
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_HALTING_CURRENT_QUERY));
            Add(CAlertScreen::GetInstance());
        }
*/
    }

    // Kill playlist loading, too.
    if (m_bLoadingLocalPlaylist)
        m_bCancelLocalPlaylist = true;

    // Kill content deletion as well.
    CancelContentDeletion();
}


void QueryCancelCallback()
{
    ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->CancelPendingQueries();
}

// Stop deleting content.
void
CLibraryMenuScreen::CancelContentDeletion()
{
    m_bCancelContentDeletion = true;
}

void DeletionCancelCallback()
{
    ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->CancelContentDeletion();
}

    
// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CLibraryMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ProcessMenuOption\n");
	// use the current selection to set the current query info 
	// as the playlist criteria and exit to the player screen
    CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
    CPlayManager* pPM = CPlayManager::GetInstance();


	DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "Clear Playlist = %d\n",m_bAppend);

#ifndef NO_UPNP
    // Remember iMenuIndex before we adjust it so that calls to MenuItemCaption don't need to
    // adjust the index every time.
    int iRelativeMenuIndex = iMenuIndex;
    
    if (m_eBrowseSource == CDJPlayerState::FML)
    {
        // See if fml still exists
        if (!m_pIMLManager->IsAvailable(m_pBrowseIML))
        {
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "FML Unavailable, Ignoring Selection.\n");
            return;
        }
        
        // Update iMenuIndex to reflect a hole if one exists.
        if (m_iHoleStart != -1)
        {
            if (iMenuIndex == m_iHoleStart)
            {
                // Ignore key presses on "Querying..." string.
                return;
            }
            else if (iMenuIndex > m_iHoleStart)
            {
                iMenuIndex += GetHoleSize() - 1;
            }
        }
    }
#endif // NO_UPNP
    
    // If we're browsing a source that's not officially our current source, we need to
    // switch to that source, then do the selection or add operation
    if (m_eBrowseSource != m_pDJPlayerState->GetSource())
    {
        if (m_pDJPlayerState->SetSource(m_eBrowseSource, true, false))
            pPS->ClearTrack();
        pPS->SetEventHandlerMode(ePSPlaybackEvents);
    }

#ifndef NO_UPNP
    // Make sure we're synched on our FML source as well
    if (m_eBrowseSource == CDJPlayerState::FML)
    {
        if (m_pBrowseIML != m_pDJPlayerState->GetCurrentIML())
        {
            m_pDJPlayerState->SetCurrentIML(m_pBrowseIML);
            if (m_pDJPlayerState->SetSource(CDJPlayerState::FML, true, false))
                pPS->ClearTrack();
            pPS->SetEventHandlerMode(ePSPlaybackEvents);
        }
    }
#endif // NO_UPNP


#ifdef MAX_PLAYLIST_TRACKS
    // Don't exceed the playlist track limit.
    if (m_bAppend && (pPL->GetSize() >= MAX_PLAYLIST_TRACKS))
    {
        CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
        CAlertScreen::GetInstance()->SetActionText(LS(SID_PLAYLIST_TRACK_LIMIT_REACHED));
        Add(CAlertScreen::GetInstance());
        return;
    }
#endif  // MAX_PLAYLIST_TRACKS

    switch(m_eBrowseMode)
	{
		case GENRE:
        {
            if (m_eBrowseSource == CDJPlayerState::HD || m_eBrowseSource == CDJPlayerState::CD)
            {
                // If there's a query already outstanding, kill it.
                CancelPendingQueries(true);

				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_GENRE), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_GENRE), MenuItemCaption(iMenuIndex));
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this, 120);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_GENRE));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                    if (pPL->IsEmpty())
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_GENRE), MenuItemCaption(iMenuIndex));
                    else
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain(CMK_ALL, CMK_ALL, m_MenuItems[iMenuIndex].iKey);
                pPC->UpdatePlaylist();

				if(!m_bAppend)
                {
#ifdef SAVE_PLAYLIST_ON_SELECT
                    // Save the updated playlist.
                    m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                    // Clear out any raw files that need to be deleted and replaced with encoded files.
                    CRecordingManager::GetInstance()->ProcessEncodingUpdates();
                }

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);
            }
#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_GENRE), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_GENRE),
                            MenuItemCaption(iRelativeMenuIndex));
                    }
                    else
                    {
                        if (pPL->IsEmpty())
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_GENRE),
                                MenuItemCaption(iRelativeMenuIndex));
                        else
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }

                    // if there's query already outstanding, kill it.
                    if (m_pMediaItemAppendQR)
                    {
                        CQueryResultManager::GetInstance()->RemoveQueryResult(m_pMediaItemAppendQR);
                        m_pMediaItemAppendQR = 0;
                    }

                    // start the media item query
                    m_pMediaItemAppendQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, m_iArtistKey, m_iAlbumKey, m_MenuItems[iMenuIndex].iKey, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "MEN");

                    // set the qr callback
                    m_nMediaItemAppendRetries = 0;
	    			if (m_bAppend)
                    {
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemAppendQueryFuncCB, (void*)this);
                        // show the alert screen if the query isn't cached...
                        if (m_pMediaItemAppendQR->GetViewID() == -1)
                        {
                            m_iCurrentQueryID = m_pMediaItemAppendQR->GetQueryResultID();
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_GENRE));
                            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                            CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                            Add(CAlertScreen::GetInstance());
                        }
                    }
                    else
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemSelectQueryFuncCB, (void*)this);
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
			break;
        }
		case ARTIST:
        {
            if (m_eBrowseSource == CDJPlayerState::HD || m_eBrowseSource == CDJPlayerState::CD)
            {
                // If there's a query already outstanding, kill it.
                CancelPendingQueries(true);

				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_ARTIST), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ARTIST), MenuItemCaption(iMenuIndex));
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this, 120);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_ARTIST));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                    if (pPL->IsEmpty())
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ARTIST), MenuItemCaption(iMenuIndex));
                    else
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain(m_MenuItems[iMenuIndex].iKey, CMK_ALL, m_iGenreKey);
                pPC->UpdatePlaylist();

				if(!m_bAppend)
                {
#ifdef SAVE_PLAYLIST_ON_SELECT
                    // Save the updated playlist.
                    m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                    // Clear out any raw files that need to be deleted and replaced with encoded files.
                    CRecordingManager::GetInstance()->ProcessEncodingUpdates();
                }

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);
            }
#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_ARTIST), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ARTIST),
                            MenuItemCaption(iRelativeMenuIndex));
                    }
                    else
                    {
                        if (pPL->IsEmpty())
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ARTIST),
                                MenuItemCaption(iRelativeMenuIndex));
                        else
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }

                    // start the media item query
                    m_pMediaItemAppendQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, m_MenuItems[iMenuIndex].iKey, m_iAlbumKey, m_iGenreKey, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "MEN");

                    // set the qr callback
                    m_nMediaItemAppendRetries = 0;
	    			if (m_bAppend)
                    {
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemAppendQueryFuncCB, (void*)this);
                        // show the alert screen if the query isn't cached...
                        if (m_pMediaItemAppendQR->GetViewID() == -1)
                        {
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Starting MediaItemAppendQuery %d\n", m_pMediaItemAppendQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pMediaItemAppendQR->GetQueryResultID();
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_ARTIST));
                            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                            CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                            Add(CAlertScreen::GetInstance());
                        }
                    }
                    else
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemSelectQueryFuncCB, (void*)this);
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
			break;
        }
		case ALBUM:
        {
            if (m_eBrowseSource == CDJPlayerState::HD || m_eBrowseSource == CDJPlayerState::CD)
            {
                // If there's a query already outstanding, kill it.
                CancelPendingQueries(true);

				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_ALBUM), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ALBUM), MenuItemCaption(iMenuIndex));
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this, 120);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_ALBUM));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                    if (pPL->IsEmpty())
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ALBUM), MenuItemCaption(iMenuIndex));
                    else
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain(m_iArtistKey, m_MenuItems[iMenuIndex].iKey, m_iGenreKey);
                pPC->UpdatePlaylist();

				if(!m_bAppend)
                {
#ifdef SAVE_PLAYLIST_ON_SELECT
                    // Save the updated playlist.
                    m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                    // Clear out any raw files that need to be deleted and replaced with encoded files.
                    CRecordingManager::GetInstance()->ProcessEncodingUpdates();
                }

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);
            }
#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_ALBUM), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ALBUM),
                            MenuItemCaption(iRelativeMenuIndex));
                    }
                    else
                    {
                        if (pPL->IsEmpty())
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_ALBUM),
                                MenuItemCaption(iRelativeMenuIndex));
                        else
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }

                    // start the media item query
                    m_pMediaItemAppendQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, m_iArtistKey, m_MenuItems[iMenuIndex].iKey, m_iGenreKey, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "ALO/MEN");

                    // set the qr callback
                    m_nMediaItemAppendRetries = 0;
	    			if (m_bAppend)
                    {
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemAppendQueryFuncCB, (void*)this);
                        // show the alert screen if the query isn't cached...
                        if (m_pMediaItemAppendQR->GetViewID() == -1)
                        {
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Starting MediaItemAppendQuery %d\n", m_pMediaItemAppendQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pMediaItemAppendQR->GetQueryResultID();
                            CAlertScreen::GetInstance()->Config(this, 120);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_ALBUM));
                            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                            CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                            Add(CAlertScreen::GetInstance());
                        }
                    }
                    else
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemSelectQueryFuncCB, (void*)this);
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
			break;
        }
		case TRACK:
            if (m_eBrowseSource == CDJPlayerState::HD || m_eBrowseSource == CDJPlayerState::CD)
            {
                // If there's a query already outstanding, kill it.
                CancelPendingQueries(true);

				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_TRACK), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_TRACK), MenuItemCaption(iMenuIndex));
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this, 120);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_TRACK));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                    if (pPL->IsEmpty())
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_TRACK), MenuItemCaption(iMenuIndex));
                    else
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                }

                while ( m_iIterIndex > iMenuIndex )
                {
                    --m_iIterIndex;
                    --m_itTrack;
                }
                while ( m_iIterIndex < iMenuIndex ) 
                {
                    ++m_iIterIndex;
                    ++m_itTrack;
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain();
                pPC->SetTrack((*m_itTrack));
                pPC->UpdatePlaylist();

				if(!m_bAppend)
                {
#ifdef SAVE_PLAYLIST_ON_SELECT
                    // Save the updated playlist.
                    m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                    // Clear out any raw files that need to be deleted and replaced with encoded files.
                    CRecordingManager::GetInstance()->ProcessEncodingUpdates();
                }

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);
            }

#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_TRACK), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_TRACK),
                            MenuItemCaption(iRelativeMenuIndex));
                    }
                    else
                    {
                        CAlertScreen::GetInstance()->Config(this, 120);
                        CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                        CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_TRACK));
                        CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                        CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                        Add(CAlertScreen::GetInstance());
                        if (pPL->IsEmpty())
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_TRACK),
                                MenuItemCaption(iRelativeMenuIndex));
                        else
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }

                    bool bStartPlayback = pPL->IsEmpty();
                    IPlaylistEntry* pEntry = pPL->AddEntry(m_pIMLManager->CreateMediaRecord(m_miTracks[iMenuIndex], m_pBrowseIML->GetMediaBaseURL()));
                    if (pEntry && bStartPlayback)
                    {
                        // Start playback if we've just added the first thing to this playlist
                        pPL->SetCurrentEntry(pEntry);
                        if (SUCCEEDED(DJSetCurrentOrNext(true)))
                            pPM->Play();
                    }
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
			break;
		case PLAYLIST:
        {
            if (0 == iMenuIndex &&
                (m_eBrowseSource == CDJPlayerState::HD ||
                m_eBrowseSource == CDJPlayerState::CD))
            {
                // If there's a query already outstanding, kill it.
                CancelPendingQueries(true);

				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_PLAYLIST), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_PLAYLIST));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain(CMK_ALL, CMK_ALL, CMK_ALL);
                pPC->UpdatePlaylist();

#ifdef SAVE_PLAYLIST_ON_SELECT
                // Save the updated playlist.
                m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                // Clear out any raw files that need to be deleted and replaced with encoded files.
                CRecordingManager::GetInstance()->ProcessEncodingUpdates();

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                    else
                        pPS->DisplaySelectTracksScreen();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);

				if(m_bAppend)
                    CAlertScreen::GetInstance()->HideScreen();
                // give the playlist a name in the quickbrowse screen
                CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_PLAY_EVERYTHING));
            }
#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_PLAYLIST), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                    }

                    // start the media item query
                    bool bDone = false;
                    if( iMenuIndex == 0 )
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "Play Everything from the FML\n");

                        // This query should always be cached, since it's the top-level tracks query.
                        m_pMediaItemAppendQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, CMK_ALL, CMK_ALL, CMK_ALL, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "MEN");

                        if (m_pMediaItemAppendQR->GetViewID() != -1)
                        {
                            // Find the index of the first hole.
                            SimpleVector<int> svRanges;
                            m_pMediaItemAppendQR->GetFilledRanges(svRanges);

                            // Pretend we just received the first cached results in order to force the next query.
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Starting MediaItemAppendQuery %d\n", m_pMediaItemAppendQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pMediaItemAppendQR->GetQueryResultID();
                            bDone = ProcessMediaItemResults(svRanges[0], svRanges[1], m_bAppend);
                        }

                        if(!m_bAppend)
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_PLAY_EVERYTHING));
                    }
                    else
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "Play Playlist #%d\n", iMenuIndex-1);
                        m_pMediaItemAppendQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, CMK_ALL, CMK_ALL, CMK_ALL, m_MenuItems[iMenuIndex-1].iKey, NULL, QUERY_BLOCK_SIZE);

                        if(!m_bAppend)
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(MenuItemCaption(iMenuIndex));
                    }

                    // set the qr callback
                    m_nMediaItemAppendRetries = 0;
	    			if (m_bAppend)
                    {
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemAppendQueryFuncCB, (void*)this);
                        // show the alert screen if the query isn't completely cached...
                        // TODO: Flash some info even if the query is cached.  It's spooky pressing append and not getting any feedback.
                        if ((m_pMediaItemAppendQR->GetViewID() == -1) || !bDone)
                        {
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Starting MediaItemAppendQuery %d\n", m_pMediaItemAppendQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pMediaItemAppendQR->GetQueryResultID();
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_PLAYLIST));
                            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                            CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                            Add(CAlertScreen::GetInstance());
                        }

                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }
                    else
                        m_pMediaItemAppendQR->SetNewResultsCallback(MediaItemSelectQueryFuncCB, (void*)this);

                    // If we're done then clear the query pointer.
                    if (bDone)
                        CancelPendingQueries();
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
            else
            {
                int i = 0;
                for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
                {
                    if((iMenuIndex - 1) == i)
                    {	
                        // If there's a query already outstanding, kill it.
                        CancelPendingQueries(true);

				        if(!m_bAppend)
                        {
                            pPS->SetMessageText(LS(SID_LOADING_PLAYLIST), CSystemMessageString::REALTIME_INFO);
					        pPL->Clear();
                            pPM->Deconfigure();
                            pPS->ClearTrack();
                            pPS->SetTrackText(LS(SID_LOADING));
                            pPS->StopRipping();
                            pPS->HideMenus();
                            m_pIMLManager->ClearMediaRecords();
                            m_bAppendLocalPlaylist = false;
                        }
                        else
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_PLAYLIST));
                            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                            CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                            Add(CAlertScreen::GetInstance());
                            m_bAppendLocalPlaylist = true;
                        }

                        bool bStartPlayback = pPL->IsEmpty();

                        pPC->SetPlaylistURL((char *)(*itPlaylist)->GetURL());
                        pPC->UpdatePlaylist();

#ifdef SAVE_PLAYLIST_ON_SELECT
                        // Save the updated playlist.
				        if(!m_bAppend)
                            m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                        // Clear out any raw files that need to be deleted and replaced with encoded files.
                        CRecordingManager::GetInstance()->ProcessEncodingUpdates();

                        // Start playback if the playlist was empty
                        if(bStartPlayback)
                        {
                            if (SUCCEEDED(DJSetCurrentOrNext(true)))
                                pPM->Play();
                        }
                        else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                            DJSetCurrentOrNext(true);

                        if(!m_bAppend)
                        {
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(MenuItemCaption(iMenuIndex));
                        }
                        else
                        {
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
//                            CAlertScreen::GetInstance()->HideScreen();
                        }
                        break;
                    }
                    i++;
                }
            }
            break;
        }
		case RADIO:
            // connect to a radio stream
            if (m_eBrowseSource == CDJPlayerState::HD || m_eBrowseSource == CDJPlayerState::CD)
            {
#if 0  // need text entry support and the ability to store radio stations locally
				if(!m_bAppend)
                {
                    pPS->SetMessageText(LS(SID_LOADING_STATION), CSystemMessageString::REALTIME_INFO);
					pPL->Clear();
                    pPM->Deconfigure();
                    pPS->ClearTrack();
                    pPS->SetTrackText(LS(SID_LOADING));
                    pPS->StopRipping();
                    pPS->HideMenus();
                    m_pIMLManager->ClearMediaRecords();
                    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_RADIO), MenuItemCaption(iMenuIndex));
                }
                else
                {
                    CAlertScreen::GetInstance()->Config(this, 120);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_STATION));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_SAVE_TO_SAVE_CURRENT_PLAYLIST_TO_HD));
                    Add(CAlertScreen::GetInstance());
                    if (pPL->IsEmpty())
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_RADIO), MenuItemCaption(iMenuIndex));
                    else
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                }

                while ( m_iIterIndex > iMenuIndex )
                {
                    --m_iIterIndex;
                    --m_itRadioStation;
                }
                while ( m_iIterIndex < iMenuIndex ) 
                {
                    ++m_iIterIndex;
                    ++m_itRadioStation;
                }

                bool bStartPlayback = pPL->IsEmpty();

                pPC->Constrain();
                pPC->SetTrack((*m_itRadioStation));
                pPC->UpdatePlaylist();

				if(!m_bAppend)
                {
#ifdef SAVE_PLAYLIST_ON_SELECT
                    // Save the updated playlist.
                    m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
                    // Clear out any raw files that need to be deleted and replaced with encoded files.
                    CRecordingManager::GetInstance()->ProcessEncodingUpdates();
                }

                // Start playback if the playlist was empty
                if(bStartPlayback)
                {
                    if (SUCCEEDED(DJSetCurrentOrNext(true)))
                        pPM->Play();
                }
                else if (pPM->GetPlayState() == CMediaPlayer::NOT_CONFIGURED)
                    DJSetCurrentOrNext(true);
#endif // if 0
            }
#ifndef NO_UPNP
            else if (m_eBrowseSource == CDJPlayerState::FML)
            {
                if(m_pBrowseIML)
                {
                    // If there's a query already outstanding, kill it.
                    CancelPendingQueries(true);

				    if(!m_bAppend)
                    {
                        pPS->SetMessageText(LS(SID_LOADING_STATION), CSystemMessageString::REALTIME_INFO);
					    pPL->Clear();
                        pPM->Deconfigure();
                        pPS->ClearTrack();
                        pPS->SetTrackText(LS(SID_LOADING));
                        pPS->StopRipping();
                        pPS->HideMenus();
                        m_pIMLManager->ClearMediaRecords();
                        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_RADIO),
                            MenuItemCaption(iRelativeMenuIndex));
                    }
                    else
                    {
                        CAlertScreen::GetInstance()->Config(this, 120);
                        CAlertScreen::GetInstance()->SetTitleText(LS(SID_ADD));
                        CAlertScreen::GetInstance()->SetActionText(LS(SID_ADDING_STATION));
                        CAlertScreen::GetInstance()->SetMessageText(LS(SID_PRESS_EXIT_TO_CANCEL));
                        CAlertScreen::GetInstance()->SetCancellable(QueryCancelCallback);
                        Add(CAlertScreen::GetInstance());
                        if (pPL->IsEmpty())
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(LS(SID_RADIO),
                                MenuItemCaption(iRelativeMenuIndex));
                        else
                            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
                    }

                    bool bStartPlayback = pPL->IsEmpty();
                    IPlaylistEntry* pEntry = pPL->AddEntry(m_pIMLManager->CreateMediaRecord(m_miRadioStations[iMenuIndex]));
                    if (pEntry && bStartPlayback)
                    {
                        // Start playback if we've just added the first thing to this playlist
                        pPL->SetCurrentEntry(pEntry);
                        if (SUCCEEDED(DJSetCurrentOrNext(true)))
                            pPM->Play();
                    }
                }
                else
                {
                    // we've had an error with the FML.  
                }
            }
#endif  // NO_UPNP
			break;
		default:
			break;
	}
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CLibraryMenuScreen::GotoSubMenu(int iMenuIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:GotoSubMenu %d\n", iMenuIndex);

#ifndef NO_UPNP
    if (m_eBrowseSource == CDJPlayerState::FML)
    {
        // See if fml still exists
        if (!m_pIMLManager->IsAvailable(m_pBrowseIML))
        {
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
            return;
        }

        // Update iMenuIndex to reflect a hole if one exists.
        if (m_iHoleStart != -1)
        {
            if (iMenuIndex == m_iHoleStart)
            {
                // Ignore key presses on "Querying..." string.
                // TODO Flash name or something.
                return;
            }
            else if (iMenuIndex > m_iHoleStart)
            {
                iMenuIndex += GetHoleSize() - 1;
            }
        }
    }
#endif

    // for the menus that are able to constrain eachother,
    // make it so a key can be blank if we're scrolling horizontally
    int iCurrentKey = m_MenuItems[iMenuIndex].iKey;

    // dc- if this menu is empty, dont drill down (since this produces a bogus value for playlist constraints)
    if( m_MenuItems.Size() == 0 )
        return ;

	// reconfigure this screen to show the right query
	switch(m_eBrowseMode)
	{
		case GENRE:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:EnterArtist\n");
            m_pszGenre       = SaveString(m_pszGenre, MenuItemCaption(GetHighlightedIndex()));
            m_iGenreTopIndex = m_iTopIndex;
#ifndef NO_UPNP            
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iGenreTopIndex = iMenuIndex - 1;
                
                //m_iPreviousHoleSize  = GetHoleSize();
                //m_iPreviousHoleStart = m_iHoleStart;
            }
#endif            
			SetBrowseMode(ARTIST);
            SetConstraints(iCurrentKey);
            ResetToTop();
			break;
		case ARTIST:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:EnterAlbum\n");
            m_pszArtist       = SaveString(m_pszArtist, MenuItemCaption(GetHighlightedIndex()));
            m_iArtistTopIndex = m_iTopIndex;
#ifndef NO_UPNP            
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iArtistTopIndex = iMenuIndex - 1;
                
                //m_iPreviousHoleSize  = GetHoleSize();
                //m_iPreviousHoleStart = m_iHoleStart;
            }
#endif            
			SetBrowseMode(ALBUM);
            SetConstraints(m_iGenreKey, iCurrentKey);
            ResetToTop();
			break;
		case ALBUM:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:EnterTrack\n");
            m_pszAlbum       = SaveString(m_pszAlbum, MenuItemCaption(GetHighlightedIndex()));
            m_iAlbumTopIndex = m_iTopIndex;
#ifndef NO_UPNP            
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iAlbumTopIndex = iMenuIndex - 1;
                
                //m_iPreviousHoleSize  = GetHoleSize();
                //m_iPreviousHoleStart = m_iHoleStart;
            }
#endif            
			SetBrowseMode(TRACK);
            SetConstraints(m_iGenreKey, m_iArtistKey, iCurrentKey);
            ResetToTop();
            break;
		case TRACK:
		case PLAYLIST:
		case RADIO:
		default:
			break;
	}
    
	Draw();
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CLibraryMenuScreen::GotoPreviousMenu()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:PrevMenu\n");

#ifndef NO_UPNP    
    if (m_eBrowseSource == CDJPlayerState::FML)
    {
        // See if fml still exists
        if (!m_pIMLManager->IsAvailable(m_pBrowseIML))
        {
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
            return;
        }
    }
#endif
    
    if(m_eBrowseMode == m_eEntryScreen)
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:PrevToEntry\n");
        // reset the top indexes
        m_iTopIndex = m_iGenreTopIndex = m_iArtistTopIndex = m_iAlbumTopIndex = m_iTrackTopIndex = m_iPlaylistTopIndex = m_iRadioTopIndex = -1;
        ((CLibraryEntryMenuScreen*)CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen())->SetBrowseMode(m_eBrowseMode);
        CDynamicMenuScreen::GotoPreviousMenu();

        // delete all the persitent titles
        m_pszGenre = SaveString(m_pszGenre, NULL);
        m_pszArtist = SaveString(m_pszArtist, NULL);
        m_pszAlbum = SaveString(m_pszAlbum, NULL);
    }
    else
    {
        // reconfigure this screen to show the right query
        switch(m_eBrowseMode)
        {
        case ARTIST:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:PrevToGenre\n");
            SetBrowseMode(GENRE);
            SetConstraints();
#ifndef NO_UPNP
            m_iDesiredIndex = m_iGenreTopIndex + 1;
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iTopIndex = AdjustIndex(m_iGenreTopIndex + 1) - 1;
            }
            else
            {
                m_iTopIndex = m_iGenreTopIndex;
            }
#endif
            //ForceRedraw();
            break;
        case ALBUM:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:PrevToArtist\n");
            SetBrowseMode(ARTIST);
            SetConstraints(m_iGenreKey);
#ifndef NO_UPNP
            m_iDesiredIndex = m_iArtistTopIndex + 1;
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iTopIndex = AdjustIndex(m_iArtistTopIndex + 1) - 1;
            }
            else
            {
                m_iTopIndex = m_iArtistTopIndex;
            }
#endif            
            SetBrowseMode(ARTIST);
            //ForceRedraw();
            break;
        case TRACK:
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:PrevToAlbum\n");
            SetBrowseMode(ALBUM);
            SetConstraints(m_iGenreKey, m_iArtistKey);
#ifndef NO_UPNP
            m_iDesiredIndex = m_iAlbumTopIndex + 1;
            if (m_eBrowseSource == CDJPlayerState::FML)
            {
                m_iTopIndex = AdjustIndex(m_iAlbumTopIndex + 1) - 1;
            }
            else
            {
                m_iTopIndex = m_iAlbumTopIndex;
            }
#endif
            //ForceRedraw();
            break;
        default:
            break;
        }
    }
    
    ForceRedraw();
    //Draw();
}

// allow other screens to configure this screens browse mode
void
CLibraryMenuScreen::SetBrowseMode(eBrowseMode eMode, bool bEntryScreen)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:SetBrowseMode\n");
    if(bEntryScreen)
    {
        m_eEntryScreen = eMode;
        // delete all the persitent titles
        m_pszGenre = SaveString(m_pszGenre, NULL);
        m_pszArtist = SaveString(m_pszArtist, NULL);
        m_pszAlbum = SaveString(m_pszAlbum, NULL);
    }

    m_eBrowseMode = eMode;

    // turn off all queries, and turn on the one we want
    m_bDropQueryResponses = true;
    m_bDropMediaItemListQueryResponses = true;
    m_bDropRadioStationQueryResponses = true;
    
    switch(m_eBrowseMode)
    {
    case GENRE:
        m_wScreenTitleSID = SID_GENRES;
        m_bDropQueryResponses = false;
        break;
    case ARTIST:
        m_wScreenTitleSID = SID_ARTISTS;
        m_bDropQueryResponses = false;
        break;
    case ALBUM:
        m_wScreenTitleSID = SID_ALBUMS;
        m_bDropQueryResponses = false;
        break;
    case TRACK:
        m_wScreenTitleSID = SID_TRACKS;
        m_bDropMediaItemListQueryResponses = false;
        break;
    case PLAYLIST:
        m_wScreenTitleSID = SID_PLAYLISTS;
        m_bDropQueryResponses = false;
        break;
    case RADIO:
        m_wScreenTitleSID = SID_RADIO;
        m_bDropRadioStationQueryResponses = false;
        break;
    default:
        break;
    }

    // when we set the browse mode, it's assumed that we have a valid library.  ?
    m_bInvalidLibrary = false;
}

// allow other screens to configure the screens browse source
void
CLibraryMenuScreen::SetBrowseSource(CDJPlayerState::ESourceMode eSource, bool bAlertUserOfChange)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lms: set browse source %s\n",
        eSource == CDJPlayerState::CD ? "CD" : (eSource == CDJPlayerState::HD ? "HD" : (eSource == CDJPlayerState::FML ? "FML" : "LINE IN")));

    if (m_eBrowseSource != eSource)
    {
        m_bInvalidLibrary = true;
        m_eBrowseSource = eSource;
        if (bAlertUserOfChange && this == Presentation()->GetCurrentThing())
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lms: set browse source has changed\n");
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH, LostCurrentSourceCallback);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_MUSIC_SOURCE_HAS_CHANGED));
            Add(CAlertScreen::GetInstance());
        }
    }
}


void
CLibraryMenuScreen::SetBrowseIML(CIML* pIML)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lms: set browse iml\n");
    m_pBrowseIML = pIML;
}

bool 
CLibraryMenuScreen::ConstraintsHaveChanged(int iGenreKey,int iArtistKey,int iAlbumKey, CDJPlayerState::ESourceMode source, eBrowseMode mode)
{
    static int nLastBrowseSource = -1;
    static int nLastBrowseMode = -1;

    if ((m_iGenreKey == iGenreKey)
        && (m_iArtistKey == iArtistKey)
        && (m_iAlbumKey == iAlbumKey)
        && (nLastBrowseSource == (int)source)
        && (nLastBrowseMode == (int)mode)
       )
    {
        return false;
    }
    nLastBrowseMode = mode;
    nLastBrowseSource = source; 
    return true;
}

void
CLibraryMenuScreen::SetConstraints(int iGenreKey, int iArtistKey, int iAlbumKey)
{
    /*  // (epg,6/4/2002): todo: reactivate with consideration for content deletion/changes.
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:SetConstraints %d%d%d%d%d\n",iGenreKey,iArtistKey,iAlbumKey,(int)m_eBrowseSource,(int)m_eBrowseMode);
    if (!ConstraintsHaveChanged(iGenreKey,iArtistKey,iAlbumKey,m_eBrowseSource,m_eBrowseMode))
    {
        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:Same\n"); 
        return;
    }
    */

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:SetConstraints+\n");
    
    m_iGenreKey = iGenreKey;
    m_iArtistKey = iArtistKey;
    m_iAlbumKey = iAlbumKey;
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
    int iDSID = m_eBrowseSource == CDJPlayerState::CD ?
        CDJPlayerState::GetInstance()->GetCDDataSource()->GetInstanceID() :
        CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    switch (m_eBrowseSource)
    {
        case CDJPlayerState::HD:
        case CDJPlayerState::CD:
            switch (m_eBrowseMode)
            {
                case GENRE:
                {
                    m_MenuItems.Clear();

                    // lower thread prio to prevent audio drop out
                    int nPrio = GetMainThreadPriority();
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:UI prio %d from %d\n",UI_THREAD_BUSY_PRIORITY,nPrio);  
                    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:sorting\n"); 

                    pCM->GetGenres(m_MenuItems, m_iArtistKey, m_iAlbumKey, iDSID);
                    if (m_MenuItems.Size() > 1)
                        m_MenuItems.QSort(QCompareKeyValueRecord);

                    // return prio to normal level
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:prio back to %d\n",nPrio); 
                    SetMainThreadPriority(nPrio);

                    SetItemCount(m_MenuItems.Size());
                    break;
                }

		        case ARTIST:
                {
                    m_MenuItems.Clear();

                    // lower thread prio to prevent audio drop out
                    int nPrio = GetMainThreadPriority();
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:UI prio %d from %d\n",UI_THREAD_BUSY_PRIORITY,nPrio);  
                    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:sorting\n"); 

                    pCM->GetArtists(m_MenuItems, m_iAlbumKey, m_iGenreKey, iDSID);
                    if (m_MenuItems.Size() > 1)
                        m_MenuItems.QSort(QCompareKeyValueRecord);

                    // return prio to normal level
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:prio back to %d\n",nPrio); 
                    SetMainThreadPriority(nPrio);

                    SetItemCount(m_MenuItems.Size());
                    break;
                }

		        case ALBUM:
                {
                    m_MenuItems.Clear();

                    // lower thread prio to prevent audio drop out
                    int nPrio = GetMainThreadPriority();
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:UI prio %d from %d\n",UI_THREAD_BUSY_PRIORITY,nPrio);  
                    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:sorting\n"); 

                    pCM->GetAlbums(m_MenuItems, m_iArtistKey, m_iGenreKey, iDSID);
                    if (m_MenuItems.Size() > 1)
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "qsorting on %d records\n"); 
                        m_MenuItems.QSort(QCompareKeyValueRecord);
                    }

                    // return prio to normal level
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:prio back to %d\n",nPrio); 
                    SetMainThreadPriority(nPrio);

                    SetItemCount(m_MenuItems.Size());
			        break;
                }

		        case TRACK:
                {
                    m_mrlTracks.Clear();
                    // lower thread prio to prevent audio drop out
                    int nPrio = GetMainThreadPriority();
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:UI prio %d from %d\n",UI_THREAD_BUSY_PRIORITY,nPrio);  
                    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:sorting\n"); 
                    // get the sorted list of tracks
                    if (iAlbumKey != CMK_ALL)
                        pCM->GetMediaRecordsAlbumSorted(m_mrlTracks, iArtistKey, iAlbumKey, iGenreKey, iDSID);
                    else
                        pCM->GetMediaRecordsTitleSorted(m_mrlTracks, iArtistKey, iAlbumKey, iGenreKey, iDSID);
                    // return prio to normal level
                    DEBUGP( DBG_LIBRARY_SCREEN , DBGLEV_INFO, "lms:prio back to %d\n",nPrio); 
                    SetMainThreadPriority(nPrio);

                    m_iIterIndex = 0;
                    m_itTrack = m_mrlTracks.GetHead();
                    SetItemCount(m_mrlTracks.Size());
			        break;
                }

                case PLAYLIST:
                    m_prlPlaylistMenuItems.Clear();
                    CPlayManager::GetInstance()->GetContentManager()->GetPlaylistRecordsByDataSourceID(m_prlPlaylistMenuItems, iDSID);
                    SetItemCount(m_prlPlaylistMenuItems.Size() + 1);  // 1 for "Play Everything"
                    break;

                case RADIO:
                    m_mrlRadioStations.Clear();
                    //pCM->GetRadioStations???
                    m_iIterIndex = 0;
                    m_itRadioStation = m_mrlRadioStations.GetHead();
                    SetItemCount(m_mrlRadioStations.Size());
                    break;

                default:
                    break;
            }
            break;

        case CDJPlayerState::LINE_IN:
            switch (m_eBrowseMode)
            {
                default:
                    m_MenuItems.Clear();
                    m_mrlTracks.Clear();
                    m_mrlRadioStations.Clear();
                    SetItemCount(m_MenuItems.Size());
                    break;
            }
            break;

        case CDJPlayerState::FML:
            switch (m_eBrowseMode)
            {

#ifndef NO_UPNP
                case GENRE:
                    if (m_pBrowseIML)
                    {
                        // if there's query already outstanding, kill it.
                        if (m_pCurrentQR)
                        {
                            CQueryResultManager::GetInstance()->RemoveQueryResult(m_pCurrentQR);
                            m_pCurrentQR = 0;
                        }

                        m_MenuItems.Clear();
                        SetItemCount(0);
                        m_iTopIndex = -1;
                        m_nGeneralQueryRetries = 0;
                        //m_iGenreTopIndex = -1;
                        // start the artist query
                        m_pCurrentQR = m_pBrowseIML->InitialQueryGenres(CMK_ALL, CMK_ALL, QUERY_BLOCK_SIZE);
                        // set the qr callback
                        m_pCurrentQR->SetNewResultsCallback(QueryFuncCB, (void*)this);

                        // If this query isn't cached then show a alert screen.
                        if (m_pCurrentQR->GetViewID() == -1)
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_GENRES));
                            Add(CAlertScreen::GetInstance());
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Query %d\n", m_pCurrentQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pCurrentQR->GetQueryResultID();
                        }
                        else
                        {
                            // TODO: be smart about resizing
                            m_pCurrentQR->GetValues(m_MenuItems, 0, m_pCurrentQR->GetTotalItemCount(), false);
                            SetHoleSize(m_pCurrentQR);
                            if (m_pCurrentQR->GetFilledItemCount() == m_pCurrentQR->GetTotalItemCount())
                                SetItemCount(m_pCurrentQR->GetFilledItemCount());
                            else
                                SetItemCount(m_pCurrentQR->GetFilledItemCount()+1); // For "Querying..."
                            ForceRedraw();

                            // keep querying for the remaining items.
                            // alternate between asking for items from the front and the back so that we fill in from the ends.
                            if (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount())
                            {
                                int iStartIndex = GetStartOfNextQueryBlock(m_pCurrentQR, QUERY_BLOCK_SIZE);
                                m_pCurrentQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                            }
                        }
                    }
                    break;
                case ARTIST:
                    if (m_pBrowseIML)
                    {
                        // if there's query already outstanding, kill it.
                        if (m_pCurrentQR)
                        {
                            CQueryResultManager::GetInstance()->RemoveQueryResult(m_pCurrentQR);
                            m_pCurrentQR = 0;
                        }

                        m_MenuItems.Clear();
                        SetItemCount(0);
                        m_iTopIndex = -1;
                        m_nGeneralQueryRetries = 0;
                        //m_iArtistTopIndex = -1;
                        // start the artist query
                        m_pCurrentQR = m_pBrowseIML->InitialQueryArtists(CMK_ALL, m_iGenreKey, QUERY_BLOCK_SIZE);
                        // set the qr callback
                        m_pCurrentQR->SetNewResultsCallback(QueryFuncCB, (void*)this);

                        // If this query isn't cached then show a alert screen.
                        if (m_pCurrentQR->GetViewID() == -1)
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_ARTISTS));
                            Add(CAlertScreen::GetInstance());
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Query %d\n", m_pCurrentQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pCurrentQR->GetQueryResultID();
                        }
                        else
                        {
                            // TODO: be smart about resizing
                            m_pCurrentQR->GetValues(m_MenuItems, 0, m_pCurrentQR->GetTotalItemCount(), false);
                            SetHoleSize(m_pCurrentQR);
                            if (m_pCurrentQR->GetFilledItemCount() == m_pCurrentQR->GetTotalItemCount())
                                SetItemCount(m_pCurrentQR->GetFilledItemCount());
                            else
                                SetItemCount(m_pCurrentQR->GetFilledItemCount()+1); // For "Querying..."
                            ForceRedraw();

                            // keep querying for the remaining items.
                            // alternate between asking for items from the front and the back so that we fill in from the ends.
                            if (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount())
                            {
                                int iStartIndex = GetStartOfNextQueryBlock(m_pCurrentQR, QUERY_BLOCK_SIZE);
                                m_pCurrentQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                            }
                        }
                    }
                    break;
                case ALBUM:
                    if (m_pBrowseIML)
                    {
                        // if there's query already outstanding, kill it.
                        if (m_pCurrentQR)
                        {
                            CQueryResultManager::GetInstance()->RemoveQueryResult(m_pCurrentQR);
                            m_pCurrentQR = 0;
                        }

                        m_MenuItems.Clear();
                        SetItemCount(0);
                        m_iTopIndex = -1;
                        m_nGeneralQueryRetries = 0;
                        //m_iAlbumTopIndex = -1;
                        // start the artist query
                        m_pCurrentQR = m_pBrowseIML->InitialQueryAlbums(m_iArtistKey, m_iGenreKey, QUERY_BLOCK_SIZE);
                        // set the qr callback
                        m_pCurrentQR->SetNewResultsCallback(QueryFuncCB, (void*)this);

                        // If this query isn't cached then show a alert screen.
                        if (m_pCurrentQR->GetViewID() == -1)
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_ALBUMS));
                            Add(CAlertScreen::GetInstance());
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Query %d\n", m_pCurrentQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pCurrentQR->GetQueryResultID();
                        }
                        else
                        {
                            // TODO: be smart about resizing
                            m_pCurrentQR->GetValues(m_MenuItems, 0, m_pCurrentQR->GetTotalItemCount(), false);
                            SetHoleSize(m_pCurrentQR);
                            if (m_pCurrentQR->GetFilledItemCount() == m_pCurrentQR->GetTotalItemCount())
                                SetItemCount(m_pCurrentQR->GetFilledItemCount());
                            else
                                SetItemCount(m_pCurrentQR->GetFilledItemCount()+1); // For "Querying..."
                            ForceRedraw();

                            // keep querying for the remaining items.
                            // alternate between asking for items from the front and the back so that we fill in from the ends.
                            if (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount())
                            {
                                int iStartIndex = GetStartOfNextQueryBlock(m_pCurrentQR, QUERY_BLOCK_SIZE);
                                m_pCurrentQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                            }
                        }
                    }
                    break;
                case PLAYLIST:
                    if (m_pBrowseIML)
                    {
                        // if there's query already outstanding, kill it.
                        if (m_pCurrentQR)
                        {
                            CQueryResultManager::GetInstance()->RemoveQueryResult(m_pCurrentQR);
                            m_pCurrentQR = 0;
                        }

                        m_MenuItems.Clear();
                        SetItemCount(0);
                        m_iTopIndex = -1;
                        m_nGeneralQueryRetries = 0;
                        //m_iPlaylistTopIndex = -1;
                        // start the playlist query
                        m_pCurrentQR = m_pBrowseIML->InitialQueryPlaylists(CMK_ALL, QUERY_BLOCK_SIZE);
                        // set the qr callback
                        m_pCurrentQR->SetNewResultsCallback(QueryFuncCB, (void*)this);

                        // If this query isn't cached then show a alert screen.
                        if (m_pCurrentQR->GetViewID() == -1)
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_PLAYLISTS));
                            Add(CAlertScreen::GetInstance());
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Query %d\n", m_pCurrentQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pCurrentQR->GetQueryResultID();
                        }
                        else
                        {
                            // TODO: be smart about resizing
                            m_pCurrentQR->GetValues(m_MenuItems, 0, m_pCurrentQR->GetTotalItemCount(), false);
                            SetHoleSize(m_pCurrentQR);
                            if (m_pCurrentQR->GetFilledItemCount() == m_pCurrentQR->GetTotalItemCount())
                                SetItemCount(m_pCurrentQR->GetFilledItemCount()+1); // +1 for "Play Everything"
                            else
                                SetItemCount(m_pCurrentQR->GetFilledItemCount()+2); // +2 for "Play Everything" and "Querying..."
                            ForceRedraw();

                            // keep querying for the remaining items.
                            // alternate between asking for items from the front and the back so that we fill in from the ends.
                            if (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount())
                            {
                                int iStartIndex = GetStartOfNextQueryBlock(m_pCurrentQR, QUERY_BLOCK_SIZE);
                                m_pCurrentQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                            }
                        }
                    }
                    break;
                case TRACK:
                    if (m_pBrowseIML)
                    {
                        // if there's query already outstanding, kill it.
                        if (m_pMediaItemListQR)
                        {
                            CQueryResultManager::GetInstance()->RemoveQueryResult(m_pMediaItemListQR);
                            m_pMediaItemListQR = 0;
                        }
                        
                        m_miTracks.Clear();
                        SetItemCount(0);
                        m_iTopIndex = -1;
                        m_nMediaItemListRetries = 0;

                        // start the artist query
                        if (m_iAlbumKey != CMK_ALL)
                            m_pMediaItemListQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, m_iArtistKey, m_iAlbumKey, m_iGenreKey, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "ALO/MEN");
                        else
                            m_pMediaItemListQR = m_pBrowseIML->InitialQueryLibrary(CMK_ALL, m_iArtistKey, m_iAlbumKey, m_iGenreKey, CMK_ALL, NULL, QUERY_BLOCK_SIZE, "MEN");
                        
                        // set the qr callback
                        m_pMediaItemListQR->SetNewResultsCallback(MediaItemListQueryFuncCB, (void*)this);
                        
                        // If this query isn't cached then show a alert screen.
                        if (m_pMediaItemListQR->GetViewID() == -1)
                        {
                            CAlertScreen::GetInstance()->Config(this);
                            CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_TRACKS));
                            Add(CAlertScreen::GetInstance());
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Media Query %d\n", m_pMediaItemListQR->GetQueryResultID());
                            m_iCurrentQueryID = m_pMediaItemListQR->GetQueryResultID();
                        }
                        else
                        {
                            // TODO: be smart about resizing
                            m_pMediaItemListQR->GetValues(m_miTracks, 0, m_pMediaItemListQR->GetTotalItemCount(), false);
                            SetHoleSize(m_pMediaItemListQR);
                            if (m_pMediaItemListQR->GetFilledItemCount() == m_pMediaItemListQR->GetTotalItemCount())
                                SetItemCount(m_pMediaItemListQR->GetFilledItemCount());
                            else
                                SetItemCount(m_pMediaItemListQR->GetFilledItemCount()+1); // For "Querying..."
                            ForceRedraw();
                            
                            // keep querying for the remaining items.
                            // alternate between asking for items from the front and the back so that we fill in from the ends.
                            if (m_pMediaItemListQR->GetFilledItemCount() < m_pMediaItemListQR->GetTotalItemCount())
                            {
                                int iStartIndex = GetStartOfNextQueryBlock(m_pMediaItemListQR, QUERY_BLOCK_SIZE);
                                m_pMediaItemListQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                            }
                        }
                    }
                    break;
                case RADIO:
                    // if there's query already outstanding, kill it.
                    if (m_pRadioStationQR)
                    {
                        CQueryResultManager::GetInstance()->RemoveQueryResult(m_pRadioStationQR);
                        m_pRadioStationQR = 0;
                    }

                    m_miRadioStations.Clear();
                    SetItemCount(0);
                    m_iTopIndex = -1;
                    m_nRadioQueryRetries = 0;

                    // start the artist query
                    m_pRadioStationQR = m_pBrowseIML->InitialQueryRadioStations(QUERY_BLOCK_SIZE);
                    // set the qr callback
                    m_pRadioStationQR->SetNewResultsCallback(RadioStationQueryFuncCB, (void*)this);

                    // If this query isn't cached then show a alert screen.
                    if (m_pRadioStationQR->GetViewID() == -1)
                    {
                        CAlertScreen::GetInstance()->Config(this);
                        CAlertScreen::GetInstance()->SetTitleText(LS(SID_SEARCHING));
                        CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_STATIONS));
                        Add(CAlertScreen::GetInstance());
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Starting Radio Station Query %d\n", m_pRadioStationQR->GetQueryResultID());
                        m_iCurrentQueryID = m_pRadioStationQR->GetQueryResultID();
                    }
                    else
                    {
                        // TODO: be smart about resizing
                        m_pRadioStationQR->GetValues(m_miRadioStations, 0, m_pRadioStationQR->GetTotalItemCount(), false);
                        SetHoleSize(m_pRadioStationQR);
                        if (m_pRadioStationQR->GetFilledItemCount() == m_pRadioStationQR->GetTotalItemCount())
                            SetItemCount(m_pRadioStationQR->GetFilledItemCount());
                        else
                            SetItemCount(m_pRadioStationQR->GetFilledItemCount()+1); // For "Querying..."
                        ForceRedraw();

                        // keep querying for the remaining items.
                        // alternate between asking for items from the front and the back so that we fill in from the ends.
                        if (m_pRadioStationQR->GetFilledItemCount() < m_pRadioStationQR->GetTotalItemCount())
                        {
                            int iStartIndex = GetStartOfNextQueryBlock(m_pRadioStationQR, QUERY_BLOCK_SIZE);
                            m_pRadioStationQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
                        }
                    }
                    break;
#endif // NO_UPNP
		        default:
                    m_MenuItems.Clear();
                    m_miTracks.Clear();
                    m_miRadioStations.Clear();
                    SetItemCount(m_MenuItems.Size());
			        break;
            }
            break;
        default:
            break;
    }
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:SetConstraints-\n");
}


#ifndef NO_UPNP
bool
CLibraryMenuScreen::QueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:QueryFuncCB\n");
    return ((CLibraryMenuScreen*)pUserData)->QueryFunc(pQR, qs, iStartIndex, iItemCount);
}



bool
CLibraryMenuScreen::QueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:QueryFunc\n");
    // todo:  logic needed...   are the new results needed for this screen?   etc.

    if (pQR != m_pCurrentQR)
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "ERROR: Received Old Query Response for Query %d\n", pQR->GetQueryResultID());
        return false;
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:ResponseToQuery %d\n", pQR->GetQueryResultID());
    if (qs == QUERY_SUCCESSFUL)
    {
        if (m_bDropQueryResponses)
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Dropping Query Responses\n");
        }
        else
        {
            int  iMenuIndex = GetHighlightedIndex();
            bool bQueryingLineHighlighted = false;
            // Convert to absolute
            if (GetHoleSize())
            {
                if (iMenuIndex > m_iHoleStart)
                {
                    iMenuIndex += GetHoleSize() - 1;
                }
                else if (iMenuIndex == m_iHoleStart)
                {
                    bQueryingLineHighlighted = true;
                }
            }
            m_MenuItems.Clear();
            // use the items that we currently have in the querey result
            m_pCurrentQR->GetValues(m_MenuItems, 0, m_pCurrentQR->GetTotalItemCount(), false);
            SetHoleSize(m_pCurrentQR);
            int iItemCount;
            if (m_eBrowseMode == PLAYLIST)
                iItemCount = m_pCurrentQR->GetFilledItemCount(); // +1 for "Play Everything" Playlist
            else
                iItemCount = m_pCurrentQR->GetFilledItemCount();
            if (m_pCurrentQR->GetFilledItemCount() == m_pCurrentQR->GetTotalItemCount())
                SetItemCount(iItemCount);
            else
                SetItemCount(iItemCount+1); // For "Querying..."
            
            if (bQueryingLineHighlighted && GetHoleSize())
                SetHighlightedIndex(AdjustIndex(m_iHoleStart));
            else
                SetHighlightedIndex(AdjustIndex(iMenuIndex));
            
            if (m_iTopIndex + DISPLAY_LINES > iStartIndex) {
                ForceRedraw();
            }

            // keep querying for the remaining items.
            // alternate between asking for items from the front and the back so that we fill in from the ends.
            if (m_pCurrentQR->GetFilledItemCount() < m_pCurrentQR->GetTotalItemCount())
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Filled: %d\n     Total: %d\n", m_pCurrentQR->GetFilledItemCount(), m_pCurrentQR->GetTotalItemCount());
                int iStartIndex = GetStartOfNextQueryBlock(m_pCurrentQR, QUERY_BLOCK_SIZE);
                m_pCurrentQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
            }
            else
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:FinishedQuerying\n");
            }
        }
    }
    else if (qs == QUERY_TIMED_OUT)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Time out query result %d\n", pQR->GetQueryResultID());
        if (++m_nGeneralQueryRetries == QUERY_RETRY_COUNT)
        {
            DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Giving up on query %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            CancelPendingQueries();
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        }
        else
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "LIB:Retry %d on query %d from fml %d %w\n", m_nGeneralQueryRetries, pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            return true;
        }
    }
    else if (qs == QUERY_INVALID_PARAM)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Invalid param on query result %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());

        // This is the result of the fml dying a quiet death and then coming back to life.
        // Our view IDs are invalid since they had meaning only to the previous instance of the fml.
        // So mark this fml as unavailable and send out a request for advertisements so we can access this fml again.
        CancelPendingQueries();
        m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        DJUPnPControlPointRefresh(false);
    }
    if (m_iCurrentQueryID == pQR->GetQueryResultID())
        CAlertScreen::GetInstance()->HideScreen();

    return false;
}

bool
CLibraryMenuScreen::MediaItemListQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MediaItemListQueryFuncCB\n");
    return ((CLibraryMenuScreen*)pUserData)->MediaItemListQueryFunc(pQR, qs, iStartIndex, iItemCount);
}



bool
CLibraryMenuScreen::MediaItemListQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MediaItemListQueryFunc\n");
    // todo:  logic needed...   are the new results needed for this screen?   etc.

    if (pQR != m_pMediaItemListQR)
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "ERROR: Received Old Query Response for Query %d\n", pQR->GetQueryResultID());
        return false;
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:ResponseForMediaQuery %d\n", pQR->GetQueryResultID());
    if (qs == QUERY_SUCCESSFUL)
    {
        if (m_bDropMediaItemListQueryResponses)
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Dropping Query Responses\n");
        }
        else
        {
            int  iMenuIndex = GetHighlightedIndex();
            bool bQueryingLineHighlighted = false;
            // Convert to absolute
            if (GetHoleSize())
            {
                if (iMenuIndex > m_iHoleStart)
                {
                    iMenuIndex += GetHoleSize() - 1;
                }
                else if (iMenuIndex == m_iHoleStart)
                {
                    bQueryingLineHighlighted = true;
                }
            }
            m_miTracks.Clear();
            // TODO: be smart about resizing
            m_pMediaItemListQR->GetValues(m_miTracks, 0, m_pMediaItemListQR->GetTotalItemCount(), false);
            SetHoleSize(m_pMediaItemListQR);
            if (m_pMediaItemListQR->GetFilledItemCount() == m_pMediaItemListQR->GetTotalItemCount())
                SetItemCount(m_pMediaItemListQR->GetFilledItemCount());
            else
                SetItemCount(m_pMediaItemListQR->GetFilledItemCount()+1); // For "Querying..."
            
            if (bQueryingLineHighlighted && GetHoleSize())
                SetHighlightedIndex(AdjustIndex(m_iHoleStart));
            else
                SetHighlightedIndex(AdjustIndex(iMenuIndex));
            
            if (m_iTopIndex + DISPLAY_LINES > iStartIndex)
                ForceRedraw();

            // keep querying for the remaining items.
            // alternate between asking for items from the front and the back so that we fill in from the ends.
            if (m_pMediaItemListQR->GetFilledItemCount() < m_pMediaItemListQR->GetTotalItemCount())
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Filled: %d\n     Total: %d\n", m_pMediaItemListQR->GetFilledItemCount(), m_pMediaItemListQR->GetTotalItemCount());
                int iStartIndex = GetStartOfNextQueryBlock(m_pMediaItemListQR, QUERY_BLOCK_SIZE);
                m_pMediaItemListQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
            }
            else
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:FinishedQuerying\n");
            }
        }
    }
    else if (qs == QUERY_TIMED_OUT)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Time out on query result %d\n", pQR->GetQueryResultID());
        if (++m_nMediaItemListRetries == QUERY_RETRY_COUNT)
        {
            DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Giving up on query %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            CancelPendingQueries();
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        }
        else
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "LIB:Retry %d on query %d from fml %d %w\n", m_nMediaItemListRetries, pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            return true;
        }

    }
    else if (qs == QUERY_INVALID_PARAM)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Invalid param on query result %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
        // This is the result of the fml dying a quiet death and then coming back to life.
        // Our view IDs are invalid since they had meaning only to the previous instance of the fml.
        // So mark this fml as unavailable and send out a request for advertisements so we can access this fml again.
        CancelPendingQueries();
        m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        DJUPnPControlPointRefresh(false);
    }
    if (m_iCurrentQueryID == pQR->GetQueryResultID())
        CAlertScreen::GetInstance()->HideScreen();

    return false;
}



bool
CLibraryMenuScreen::MediaItemAppendQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MediaItemAppendQueryFuncCB\n");
    return ((CLibraryMenuScreen*)pUserData)->MediaItemAppendQueryFunc(pQR, qs, iStartIndex, iItemCount, true);
}

bool
CLibraryMenuScreen::MediaItemSelectQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MediaItemSelectQueryFuncCB\n");
    return ((CLibraryMenuScreen*)pUserData)->MediaItemAppendQueryFunc(pQR, qs, iStartIndex, iItemCount, false);
}

// This is the engine that drives appending media items to the playlist.
// There are two cases: cached and non-cached.
// Currently there is only one cached query: the top-level media item query.  If the
// user selects "Play Everything" then this is the query that will be put into play.
// After startup the first 20 items and the last 20 items of the top-level query are cached.
// When "Play Everything" is selected this function is called with iStartIndex equal to 0
// and iItemCount equal to 20.  It adds the first 20 items to the playlist and starts a query
// for the next 40 (QUERY_BLOCK_SIZE * 2) items in the playlist.
//
// After that we can pretend that the top-level cached query and the non-cached queries work
// the same.  A set of results come in, the new media items are added to the playlist, and
// the next batch of missing values are requested.  If there are no more missing values then
// the query is over.
//
// There's one more special case for the cached query.  Since the last 20 items are cached,
// then we have to check when the total item count equals the filled item count (i.e., there
// are no more missing values) and make sure we haven't forgotten to add the last cached values
// to the playlist.
//
// ProcessMediaItemResults returns true if the query is finished processing (or if the maximum
// number of fml tracks has been reached), false otherwise.
bool
CLibraryMenuScreen::ProcessMediaItemResults(int iStartIndex, int iItemCount, bool bAppend)
{
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    DBASSERT( DBG_LIBRARY_SCREEN, pPL, "Null playlist object\n" );

    // Show progress.
    if (bAppend) {
        CAlertScreen::GetInstance()->ResetProgressBar(m_pMediaItemAppendQR->GetFilledItemCount(), m_pMediaItemAppendQR->GetTotalItemCount());
        char cCount[32];
        sprintf(cCount, " %d ", m_pMediaItemAppendQR->GetTotalItemCount());
        TCHAR tcCount[32];
        CharToTcharN(tcCount, cCount, 32);

        TCHAR tcCaption[128];
        tcCaption[0] = 0;
        tstrcat(tcCaption, LS(SID_ADDING));
        tstrcat(tcCaption, tcCount);
        tstrcat(tcCaption, (m_pMediaItemAppendQR->GetTotalItemCount() > 1) ? LS(SID_TRACKS) : LS(SID_TRACK));
        CAlertScreen::GetInstance()->SetActionText(tcCaption);
    }
    

    // First, extract the latest media records from the query results.
    IMLMediaInfoVector miTracks;
    m_pMediaItemAppendQR->GetValues(miTracks, iStartIndex, iItemCount, false);

    bool bDone = false;

    // Are all query results in?
#ifdef MAX_PLAYLIST_TRACKS
    if (pPL->GetSize() + iItemCount < MAX_PLAYLIST_TRACKS)
    {
#endif  // MAX_PLAYLIST_TRACKS
        if (m_pMediaItemAppendQR->GetFilledItemCount() < m_pMediaItemAppendQR->GetTotalItemCount())
        {
            // Nope, make another query to get more of the results.
            // Get the filled ranges so we ask for the lowest unfilled index.
            SimpleVector<int> svRanges;
            m_pMediaItemAppendQR->GetFilledRanges(svRanges);

            int iQueryCount = QUERY_BLOCK_SIZE * 2;

            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Query from %d for %d of the %d items\n", svRanges[0] + svRanges[1], iQueryCount, m_pMediaItemAppendQR->GetTotalItemCount());
            m_iQueryItemsLeft = m_pMediaItemAppendQR->GetTotalItemCount() - svRanges[0] - svRanges[1];
            m_pMediaItemAppendQR->QueryValues(svRanges[0] + svRanges[1], iQueryCount);
        }
        else
        {
            // Yep.
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Received all query items\n");
            bDone = true;
            m_iQueryItemsLeft = 0;

            // If the tail end of the query result has been cached, then we also
            // have to request those items.
            if (m_pMediaItemAppendQR->GetTotalItemCount() > iStartIndex + iItemCount)
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Adding the last %d cached values (%d - %d) to the playlist\n", m_pMediaItemAppendQR->GetTotalItemCount() - (iStartIndex + iItemCount), iStartIndex + iItemCount, m_pMediaItemAppendQR->GetTotalItemCount() - 1);
                m_pMediaItemAppendQR->GetValues(miTracks, iStartIndex + iItemCount, m_pMediaItemAppendQR->GetTotalItemCount() - (iStartIndex + iItemCount), false);
            }

            if (m_iCurrentQueryID == m_pMediaItemAppendQR->GetQueryResultID())
                CAlertScreen::GetInstance()->HideScreen();
        }
    }
#ifdef MAX_PLAYLIST_TRACKS
    else
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Max playlist tracks reached: %d \n", MAX_PLAYLIST_TRACKS);
        bDone = true;
        m_iQueryItemsLeft = 0;

        if (m_iCurrentQueryID == m_pMediaItemAppendQR->GetQueryResultID())
            CAlertScreen::GetInstance()->HideScreen();
    }
#endif  // MAX_PLAYLIST_TRACKS

    // process the results we just received
    bool bSetSong = pPL->IsEmpty();
    MediaRecordList mrl;
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ProcessMediaItemResults: create %d media records\n", miTracks.Size());
    m_pIMLManager->CreateMediaRecords(mrl, miTracks, m_pMediaItemAppendQR->GetIML()->GetMediaBaseURL());
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ProcessMediaItemResults: add %d entries to current playlist\n", mrl.Size());
    pPL->AddEntries(mrl);

    // Save the playlist if all query results are in and we're not in append mode.
    if (bDone && !bAppend)
    {
        // dc- Properly handle the case of an empty playlist. this results in a query with 0 items, and was leaving
        //  the player screen in 'loading' forever
        if( m_pMediaItemAppendQR->GetTotalItemCount() == 0 ) {
            CPlayerScreen* pPS = CPlayerScreen::GetPlayerScreen();
            pPS->DisplayNoContentScreen();
            pPS->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
        }
#ifdef SAVE_PLAYLIST_ON_SELECT
        // Save the updated playlist.
        m_pDJPlayerState->SaveCurrentPlaylist();
#endif  // SAVE_PLAYLIST_ON_SELECT
        // Clear out any raw files that need to be deleted and replaced with encoded files.
        CRecordingManager::GetInstance()->ProcessEncodingUpdates();
    }

    // These results are from the initial query, so set the song to get things going.
    if (bSetSong)
    {
        // Set the first track in the current play mode.
        pPL->SetCurrentEntry(pPL->GetEntry(0, CPlayManager::GetInstance()->GetPlaylistMode()));
        if (SUCCEEDED(DJSetCurrentOrNext(true)))
        {
            // Start playback on select or append when the playlist is empty
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ProcessMediaItemResults: start playback\n");
            CPlayManager::GetInstance()->Play();
        }
    }

    // Force the quickbrowse screen to update
    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->RefreshPlaylist();

    return bDone;
}

bool
CLibraryMenuScreen::MediaItemAppendQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, bool bAppend)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:MediaItemAppendQueryFunc\n");
    // todo:  logic needed...   are the new results needed for this screen?   etc.
    
    if (pQR != m_pMediaItemAppendQR)
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "Received Old Query Response for media item append query %d\n", pQR->GetQueryResultID());
        return false;
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:ResponseToQuery %d\n", pQR->GetQueryResultID());

    if (qs == QUERY_SUCCESSFUL)
    {
        if (ProcessMediaItemResults(iStartIndex, iItemCount, bAppend))
        {
            // We're done, so clear the query pointer.
            CancelPendingQueries();
        }
    }
    else if (qs == QUERY_TIMED_OUT)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "Time out on query result %d\n", pQR->GetQueryResultID());
        if (++m_nMediaItemAppendRetries == QUERY_RETRY_COUNT)
        {
            DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Giving up on query %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            if (CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
            {
                ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->DisplayNoContentScreen();
                ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
            }
            if (m_iCurrentQueryID == m_pMediaItemAppendQR->GetQueryResultID())
                CAlertScreen::GetInstance()->HideScreen();
            CancelPendingQueries();
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        }
        else
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "LIB:Retry %d on query %d from fml %d %w\n", m_nMediaItemAppendRetries, pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            return true;
        }
    }
    else if (qs == QUERY_INVALID_PARAM)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Invalid param on query result %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
        if (CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
        {
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->DisplayNoContentScreen();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetMessageText(LS(SID_NO_VALID_TRACKS_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
        }
        if (m_iCurrentQueryID == pQR->GetQueryResultID())
            CAlertScreen::GetInstance()->HideScreen();

        // This is the result of the fml dying a quiet death and then coming back to life.
        // Our view IDs are invalid since they had meaning only to the previous instance of the fml.
        // So mark this fml as unavailable and send out a request for advertisements so we can access this fml again.
        CancelPendingQueries();
        m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        DJUPnPControlPointRefresh(false);

    }

    return false;
}



bool
CLibraryMenuScreen::RadioStationQueryFuncCB(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount, void* pUserData)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:RadioStationQueryFuncCB\n");
    return ((CLibraryMenuScreen*)pUserData)->RadioStationQueryFunc(pQR, qs, iStartIndex, iItemCount);
}



bool
CLibraryMenuScreen::RadioStationQueryFunc(CQueryResult* pQR, query_status_t qs, int iStartIndex, int iItemCount)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:QueryFunc\n");
    // todo:  logic needed...   are the new results needed for this screen?   etc.

    if (pQR != m_pRadioStationQR)
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "ERROR: Received Old Query Response for Query %d\n", pQR->GetQueryResultID());
        return false;
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:ResponseToQuery %d\n", pQR->GetQueryResultID());
    if (qs == QUERY_SUCCESSFUL)
    {
        if (m_bDropRadioStationQueryResponses)
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Dropping Query Responses\n");
        }
        else
        {
            int iMenuIndex = GetHighlightedIndex();
            bool bQueryingLineHighlighted = false;
            // Convert to absolute
            if (GetHoleSize())
            {
                if (iMenuIndex > m_iHoleStart)
                {
                    iMenuIndex += GetHoleSize() - 1;
                }
                else if (iMenuIndex == m_iHoleStart)
                {
                    bQueryingLineHighlighted = true;
                }
            }
            m_miRadioStations.Clear();
            // TODO: be smart about resizing
            m_pRadioStationQR->GetValues(m_miRadioStations, 0, m_pRadioStationQR->GetTotalItemCount(), false);
            SetHoleSize(m_pRadioStationQR);
            int iItemCount;
            if (m_eBrowseMode == PLAYLIST)
                iItemCount = m_pRadioStationQR->GetFilledItemCount(); // +1 for "Play Everything" Playlist
            else
                iItemCount = m_pRadioStationQR->GetFilledItemCount();
            if (m_pRadioStationQR->GetFilledItemCount() == m_pRadioStationQR->GetTotalItemCount())
                SetItemCount(iItemCount);
            else
                SetItemCount(iItemCount+1); // For "Querying..."
            
            if (bQueryingLineHighlighted && GetHoleSize())
                SetHighlightedIndex(AdjustIndex(m_iHoleStart));
            else
                SetHighlightedIndex(AdjustIndex(iMenuIndex));
            
            if (m_iTopIndex + DISPLAY_LINES > iStartIndex)
                ForceRedraw();

            // keep querying for the remaining items.
            // alternate between asking for items from the front and the back so that we fill in from the ends.
            if (m_pRadioStationQR->GetFilledItemCount() < m_pRadioStationQR->GetTotalItemCount())
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:Filled: %d\n     Total: %d\n", m_pRadioStationQR->GetFilledItemCount(), m_pRadioStationQR->GetTotalItemCount());
                int iStartIndex = GetStartOfNextQueryBlock(m_pRadioStationQR, QUERY_BLOCK_SIZE);
                m_pRadioStationQR->QueryValues(iStartIndex, QUERY_BLOCK_SIZE);
            }
            else
            {
                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:FinishedQuerying\n");
                if (m_iCurrentQueryID == pQR->GetQueryResultID())
                    CAlertScreen::GetInstance()->HideScreen();
            }
        }
    }
    else if (qs == QUERY_TIMED_OUT)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Time out query result %d\n", pQR->GetQueryResultID());
        if (++m_nRadioQueryRetries == QUERY_RETRY_COUNT)
        {
            DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Giving up on query %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            if (m_iCurrentQueryID == pQR->GetQueryResultID())
                CAlertScreen::GetInstance()->HideScreen();
            CancelPendingQueries();
            m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        }
        else
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "LIB:Retry %d on query %d from fml %d %w\n", m_nRadioQueryRetries, pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
            return true;
        }
    }
    else if (qs == QUERY_INVALID_PARAM)
    {
        DEBUG( DBG_LIBRARY_SCREEN, DBGLEV_WARNING, "LIB:Invalid param on query result %d from fml %d %w\n", pQR->GetQueryResultID(), m_pBrowseIML->GetInstanceID(), m_pBrowseIML->GetFriendlyName());
        if (m_iCurrentQueryID == pQR->GetQueryResultID())
            CAlertScreen::GetInstance()->HideScreen();

        // This is the result of the fml dying a quiet death and then coming back to life.
        // Our view IDs are invalid since they had meaning only to the previous instance of the fml.
        // So mark this fml as unavailable and send out a request for advertisements so we can access this fml again.
        CancelPendingQueries();
        m_pIMLManager->SetIMLUnavailable(m_pBrowseIML);
        DJUPnPControlPointRefresh(false);
    }

    return false;
}


#endif // NO_UPNP



void
CLibraryMenuScreen::DeleteItemInFocus()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:DeleteItemInFocus\n");
    if (m_cItems && m_eBrowseSource == CDJPlayerState::HD)
    {
    	DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "DelItem?\n");
        WORD wCaptionID;
        switch (m_eBrowseMode)
        {
        case GENRE:
            wCaptionID = SID_DELETE_THIS_GENRE_QM;
            break;
            
        case ARTIST:
            wCaptionID = SID_DELETE_THIS_ARTIST_QM;
            break;
            
        case ALBUM:
            wCaptionID = SID_DELETE_THIS_ALBUM_QM;
            break;
            
        case TRACK:
            wCaptionID = SID_DELETE_THIS_TRACK_QM;
            break;
            
        case PLAYLIST:
            // Don't delete "Play Everything"
            if (GetHighlightedIndex() == 0)
                return;
            wCaptionID = SID_DELETE_THIS_PLAYLIST_QM;
            break;
            
        case RADIO:
            wCaptionID = SID_DELETE_THIS_RADIO_STATION_QM;
            break;
            
        default:
            return;
        }
        CYesNoScreen::GetInstance()->Config(this, DeleteItemInFocusCallback);
        CYesNoScreen::GetInstance()->SetTitleText(LS(SID_DELETE));
        CYesNoScreen::GetInstance()->SetActionText(LS(wCaptionID));
        Add(CYesNoScreen::GetInstance());
    }
}


void
CLibraryMenuScreen::ResetAndRefresh()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ResetAndRefresh\n");
    ResetToTop();
    if (this == CInfoMenuScreen::GetInfoMenuScreen()->Parent())
        CInfoMenuScreen::GetInfoMenuScreen()->HideScreen();
    else
        Draw();
}


void
CLibraryMenuScreen::ResynchWithChanges()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:ResynchWithChanges\n");
    // see if the size has changed...
    int iSize = m_cItems;
    SetConstraints(m_iGenreKey, m_iArtistKey, m_iAlbumKey);
    // what index do we want to be looking at?
    if (iSize != m_cItems)
        while ((GetHighlightedIndex() >= m_cItems) && (m_iTopIndex > -1))
            m_iTopIndex--;
}


void
CLibraryMenuScreen::NotifyLostCurrentSource()
{
    // flag the currently showing list of items as unusable so we don't try to display them
    m_bInvalidLibrary = true;

    switch (m_eBrowseSource)
    {
    case CDJPlayerState::FML:
        // if the fml we're looking at got removed, then we need to let the user know and exit the screen.
        if (this == Presentation()->GetCurrentThing())
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH, LostCurrentSourceCallback);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_FML_NO_LONGER_AVAILABLE));
            Add(CAlertScreen::GetInstance());
        }
        SetBrowseIML(NULL);
        break;
    case CDJPlayerState::CD:
        // if the fml we're looking at got removed, then we need to let the user know and exit the screen.
        if (this == Presentation()->GetCurrentThing())
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH, LostCurrentSourceCallback);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_CD_NO_LONGER_AVAILABLE));
            Add(CAlertScreen::GetInstance());
        }
        break;
        break;
    case CDJPlayerState::HD:
    case CDJPlayerState::LINE_IN:
    default:
        // we shouldn't loose these
        break;
    }
}

void
CLibraryMenuScreen::LostCurrentSourceCB()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:LostCurrentSourceCB\n");
    CPlayerScreen* pPS = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    pPS->HideMenus();
    ((CSourceMenuScreen*)CSourceMenuScreen::GetSourceMenuScreen())->RefreshSource();
    pPS->Add(CSourceMenuScreen::GetSourceMenuScreen());
    Presentation()->MoveFocusTree(CSourceMenuScreen::GetSourceMenuScreen());
}

void
CLibraryMenuScreen::LostCurrentSourceCallback()
{
    if( s_pLibraryMenuScreen )
        s_pLibraryMenuScreen->LostCurrentSourceCB();    
}

// Removes a media record from the current playlist and idle coder.
void
CLibraryMenuScreen::RemoveMediaRecord(IMediaContentRecord* pRecord)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting track: %s\n", pRecord->GetURL());

    // Check the playlist and remove the entry if it's there.
    CDJPlaylist* pPlaylist = (CDJPlaylist*)CPlayManager::GetInstance()->GetPlaylist();

    // Hook into the inner workings of the DJ playlist to speed up traversal.
    const CDJPlaylist::PlaylistEntryList* playlist = pPlaylist->GetEntryList();
    CDJPlaylist::PlaylistIterator it = playlist->GetHead();

    while (it != playlist->GetEnd())
    {
        CDJPlaylist::PlaylistIterator itNext = it + 1;
        if ((*it)->GetContentRecord() == pRecord)
        {
            // If this is the current entry, then deconfigure the player and remember to set a new track.
            if (*it == pPlaylist->GetCurrentEntry())
            {
                CPlayerScreen::GetPlayerScreen()->RemoveCurrentPlaylistEntry();
            }
            else
            {
                pPlaylist->DeleteEntry(*it);
            }
            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsUnsaved();
        }
        it = itNext;
    }

    // Remove the file from the idle coder.
    if (CIdleCoder::GetInstance()->RemoveJob(pRecord->GetURL()))
    {
        SetRegistryDirty();
//        CIdleCoder::GetInstance()->SaveToRegistry();
//        CDJPlayerState::GetInstance()->SaveRegistry();
    }
}

static void DeleteMediaRecordProgressCB(int iProgress, void* pUserData)
{
    CAlertScreen::GetInstance()->UpdateProgressBar(iProgress + (int)pUserData);
}

void
CLibraryMenuScreen::RemoveMediaRecords(MediaRecordList& mrl, int iCurrentIndex)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting %d records\n", mrl.Size());
    int i = 0;

    // Delete the files from the drive.
    for (MediaRecordIterator it = mrl.GetHead(); it != mrl.GetEnd() && i < DELETE_CHUNK_SIZE; ++it, ++i)
    {
        RemoveMediaRecord(*it);
        // Delete the file from the disk.
        if (!pc_unlink(const_cast<char*>(FullFilenameFromURLInPlace((*it)->GetURL()))))
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:Unable to delete file %s\n", FullFilenameFromURLInPlace((*it)->GetURL()));
        }
        CAlertScreen::GetInstance()->UpdateProgressBar(iCurrentIndex + i);
    }

    ((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->DeleteMediaRecords(mrl, DELETE_CHUNK_SIZE, DeleteMediaRecordProgressCB, (void*)(i + iCurrentIndex));
}

bool
CLibraryMenuScreen::RemoveMediaRecords(content_delete_info_t* pDeleteInfo)
{
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
    int iDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    // Get a list of tracks to delete.
    MediaRecordList mrl;
    pCM->GetMediaRecords(mrl, pDeleteInfo->iArtistID, pDeleteInfo->iAlbumID, pDeleteInfo->iGenreID, iDSID);

    RemoveMediaRecords(mrl, pDeleteInfo->nDeleted);

    // If there are any media records left to delete then send a message to the UI to continue this process.
    if (!mrl.IsEmpty())
    {
        pDeleteInfo->nDeleted += (DELETE_CHUNK_SIZE * 3);
        CAlertScreen::GetInstance()->UpdateProgressBar(pDeleteInfo->nDeleted);
        CEventQueue::GetInstance()->PutEvent(EVENT_DELETE_CONTENT, (void*)pDeleteInfo);
        return false;
    }
    else
    {
        return true;
    }
}

bool
CLibraryMenuScreen::StartRemoveMediaRecords(int iGenreKey, int iArtistKey, int iAlbumKey)
{
    CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
    int iDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
    m_bCancelContentDeletion = false;

    // Get a list of tracks to delete.
    MediaRecordList mrl;
    pCM->GetMediaRecords(mrl, iArtistKey, iAlbumKey, iGenreKey, iDSID);

    // Tell the user how many tracks/records we're deleting
    {
        char cCount[32];
        sprintf(cCount, " %d ", mrl.Size());
        TCHAR tcCount[32];
        CharToTcharN(tcCount, cCount, 32);
        TCHAR tcCaption[128];
        tstrcpy(tcCaption, LS(SID_DELETING));
        tstrcat(tcCaption, tcCount);
        if (mrl.Size() == 1)
            tstrcat(tcCaption, LS(SID_TRACK_FROM_THE_HD));
        else
            tstrcat(tcCaption, LS(SID_TRACKS_FROM_THE_HD));
        CAlertScreen::GetInstance()->SetMessageText(tcCaption);
    }

    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting %d records total\n", mrl.Size());
    CAlertScreen::GetInstance()->ResetProgressBar(0, mrl.Size() * 3);

    RemoveMediaRecords(mrl, 0);

    // If there are any media records left to delete then send a message to the UI to continue this process.
    if (!mrl.IsEmpty())
    {
        content_delete_info_t* pDeleteInfo = new content_delete_info_t;
        pDeleteInfo->iGenreID = iGenreKey;
        pDeleteInfo->iArtistID = iArtistKey;
        pDeleteInfo->iAlbumID = iAlbumKey;
        pDeleteInfo->nDeleted = DELETE_CHUNK_SIZE * 3;
        CEventQueue::GetInstance()->PutEvent(EVENT_DELETE_CONTENT, (void*)pDeleteInfo);
        return false;
    }
    else
    {
        // Save the content manager database if we're not playing audio.
        CommitUpdatesIfSafe();
        return true;
    }
}

void
CLibraryMenuScreen::DeleteItemInFocusCB(bool bDelete)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:DeleteItemInFocusCB\n");
    bool bHideAlert = true;
    if (bDelete)
    {
        // add the alert screen
        CAlertScreen::GetInstance()->Config(this, 0);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_DELETE));
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_PLEASE_WAIT));
        CAlertScreen::GetInstance()->SetCancellable(DeletionCancelCallback);
        Add(CAlertScreen::GetInstance());

        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:DelFocusItem\n");
        CDJContentManager* pCM = (CDJContentManager*) CPlayManager::GetInstance()->GetContentManager();
        int iDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

        // Adjust thread priority so playback doesn't stutter.
        int nPrio = GetMainThreadPriority();
        SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

        switch(m_eBrowseMode)
        {

        case GENRE:
        {
            int iMenuIndex = GetHighlightedIndex();
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting genre %w\n", m_MenuItems[iMenuIndex].szValue);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_DELETING_GENRE));
            CAlertScreen::GetInstance()->SetMessageText(LS(SID_LOOKING_FOR_TRACKS));

            bHideAlert = StartRemoveMediaRecords(m_MenuItems[iMenuIndex].iKey, m_iArtistKey, m_iAlbumKey);

            // Requery, since pointers in the database may be invalid now.
            m_MenuItems.Clear();
            pCM->GetGenres(m_MenuItems, m_iArtistKey, m_iAlbumKey, iDSID);
            if (m_MenuItems.Size() > 1)
                m_MenuItems.QSort(QCompareKeyValueRecord);
            SetItemCount(m_MenuItems.Size());

            if (m_MenuItems.IsEmpty())
            {
                GotoPreviousMenu();
            }
            else if (iMenuIndex >= m_MenuItems.Size())
            {
                // select last item
                m_iGenreTopIndex = m_MenuItems.Size() - 1;
                --m_iTopIndex;
            }
            else
                m_iGenreTopIndex = iMenuIndex;

            break;
        }
            
        case ARTIST:
        {
            int iMenuIndex = GetHighlightedIndex();
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting artist %w\n", m_MenuItems[iMenuIndex].szValue);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_DELETING_ARTIST));
            CAlertScreen::GetInstance()->SetMessageText(LS(SID_LOOKING_FOR_TRACKS));

            bHideAlert = StartRemoveMediaRecords(m_iGenreKey, m_MenuItems[iMenuIndex].iKey, m_iAlbumKey);

            // Requery, since pointers in the database may be invalid now.
            m_MenuItems.Clear();
            pCM->GetArtists(m_MenuItems, m_iAlbumKey, m_iGenreKey, iDSID);
            if (m_MenuItems.Size() > 1)
                m_MenuItems.QSort(QCompareKeyValueRecord);
            SetItemCount(m_MenuItems.Size());

            if (m_MenuItems.IsEmpty())
            {
                GotoPreviousMenu();
            }
            else if (iMenuIndex >= m_MenuItems.Size())
            {
                // select last item
                m_iArtistTopIndex = m_MenuItems.Size() - 1;
                --m_iTopIndex;
            }
            else
                m_iArtistTopIndex = iMenuIndex;

            break;
        }
            
        case ALBUM:
        {
            int iMenuIndex = GetHighlightedIndex();
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting album %w\n", m_MenuItems[iMenuIndex].szValue);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_DELETING_ALBUM));
            CAlertScreen::GetInstance()->SetMessageText(LS(SID_LOOKING_FOR_TRACKS));

            bHideAlert = StartRemoveMediaRecords(m_iGenreKey, m_iArtistKey, m_MenuItems[iMenuIndex].iKey);

            // Requery, since pointers in the database may be invalid now.
            m_MenuItems.Clear();
            pCM->GetAlbums(m_MenuItems, m_iArtistKey, m_iGenreKey, iDSID);
            if (m_MenuItems.Size() > 1)
                m_MenuItems.QSort(QCompareKeyValueRecord);
            SetItemCount(m_MenuItems.Size());

            if (m_MenuItems.IsEmpty())
            {
                GotoPreviousMenu();
            }
            else if (iMenuIndex >= m_MenuItems.Size())
            {
                // select last item
                m_iAlbumTopIndex = m_MenuItems.Size() - 1;
                --m_iTopIndex;
            }
            else
                m_iAlbumTopIndex = iMenuIndex;

            break;
        }
            
        case TRACK:
        {
            int iMenuIndex = GetHighlightedIndex();
            
            // extract URL
            int i = 0;
            for (MediaRecordIterator itTrack = m_mrlTracks.GetHead(); itTrack != m_mrlTracks.GetEnd(); ++itTrack)
            {
                if (iMenuIndex == i)
                {	
                    // Remove the entry from the content manager.
                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:RemEntry\n");
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_DELETING_TRACK));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_DELETING_TRACK_FROM_THE_HD));

                    // Reset the draw list
                    IMediaContentRecord* pCR = m_mrlTracks.Remove(itTrack);
                    SetItemCount(m_mrlTracks.Size());

                    RemoveMediaRecord(pCR);

                    // Delete the file from the disk.
                    if (!pc_unlink(const_cast<char*>(FullFilenameFromURLInPlace(pCR->GetURL()))))
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:Unable to delete file %s\n", FullFilenameFromURLInPlace(pCR->GetURL()));

                    // Remove the record from the content manager.
                    CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);
                    CPlayManager::GetInstance()->GetContentManager()->DeleteMediaRecord(pCR);

                    // Save the content manager database if we're not playing audio.
                    CommitUpdatesIfSafe();

                    m_iTrackTopIndex = -1;
                    if (m_mrlTracks.IsEmpty())
                    {
                        GotoPreviousMenu();
                    }
                    else if (iMenuIndex >= m_mrlTracks.Size())
                    {
                        // select last item
                        m_iTrackTopIndex = m_mrlTracks.Size() - 1;
                        --m_iTopIndex;
                    }
                    else
                        m_iTrackTopIndex = iMenuIndex;

                    // Find the new current place in the menu system.
                    m_iIterIndex = 0;
                    for (m_itTrack = m_mrlTracks.GetHead(); (m_itTrack != m_mrlTracks.GetEnd()) && (m_iIterIndex < m_iTrackTopIndex); ++m_itTrack)
                        ++m_iIterIndex;
                    
                    break;
                    
                }
                i++;
            }
            break;
        }
            
        case RADIO:
            break;

        case PLAYLIST:
        {
            // get current highlighted item index
            int iMenuIndex = s_pLibraryMenuScreen->GetHighlightedIndex();

            // extract URL
            int i = 0;
            for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
            {
                if ((iMenuIndex - 1) == i)
                {	
                    // Delete the file from the disk.
                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Deleting playlist %s\n", (*itPlaylist)->GetURL());
                    CAlertScreen::GetInstance()->SetActionText(LS(SID_DELETING_PLAYLIST));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_DELETING_PLAYLIST_FROM_THE_HD));

                    if (!pc_unlink(const_cast<char*>(FullFilenameFromURLInPlace((*itPlaylist)->GetURL()))))
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:Unable to delete playlist file %s\n", FullFilenameFromURLInPlace((*itPlaylist)->GetURL()));
                    }

                    // Remove the entry from the content manager.
                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:RemEntry\n");
                    CPlayManager::GetInstance()->GetContentManager()->DeletePlaylistRecord((*itPlaylist)->GetID());

                    // Reset the draw list.
                    m_prlPlaylistMenuItems.Clear();
                    CPlayManager::GetInstance()->GetContentManager()->GetPlaylistRecordsByDataSourceID(m_prlPlaylistMenuItems, iDSID);
                    SetItemCount(m_prlPlaylistMenuItems.Size() + 1);  // 1 for "Play Everything"

                    if (iMenuIndex >= (m_prlPlaylistMenuItems.Size() + 1))
                    {
                        // select last item
                        m_iPlaylistTopIndex =  m_prlPlaylistMenuItems.Size();
                        --m_iTopIndex;
                    }
                    else
                        m_iPlaylistTopIndex = iMenuIndex - 1;

                    break;
                    
                }
                i++;
            }
            break;
        }
            
        default:
            break;
            
        }

        // Reset the thread priority.
        SetMainThreadPriority(nPrio);

        // Switch the Current Source to be HD whenever we delete something off the HD
        // dc- per tg #1469 don't perform this switch when listening to audio CDs
        if (CDJPlayerState::HD != m_pDJPlayerState->GetSource()
            && CDJPlayerState::CD != m_pDJPlayerState->GetSource() )
        {
            if (m_pDJPlayerState->SetSource(CDJPlayerState::HD, true, false))
                CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
            else
                CPlayerScreen::GetPlayerScreen()->RefreshCurrentTrackMetadata();

            CPlayerScreen::GetPlayerScreen()->SetEventHandlerMode(ePSPlaybackEvents);
        }
        else
        {
            // synch the player screen in case we've deleted an item in playlist
            // and the current tracks playlist number has changed
            if (CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
                CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
            else
                CPlayerScreen::GetPlayerScreen()->RefreshCurrentTrackMetadata();
        }
    }
    else
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:CnclDel\n");
    }

    if (bHideAlert)
    {
        CAlertScreen::GetInstance()->HideScreen();
        Draw();
    }
}


void
CLibraryMenuScreen::DeleteItemInFocusCallback(bool bDelete)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:DeleteItemInFocusCallback\n");
    // let's be anal, shall we?
    if (!s_pLibraryMenuScreen)
        return;

    s_pLibraryMenuScreen->DeleteItemInFocusCB(bDelete);
}


void
CLibraryMenuScreen::EditItemInFocus()
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:EditItemInFocus\n");
    if (m_cItems && m_eBrowseSource == CDJPlayerState::HD)
    {
        switch (m_eBrowseMode)
        {
        case GENRE:
        case ARTIST:
        case ALBUM:
        case TRACK:
            break;
        case PLAYLIST:
            // Index 0 is the Play Everything item, which should be a constant, so do not edit.
            if (GetHighlightedIndex() == 0)
                return;
            break;
        default:
            return;
        }

        CEditScreen::GetInstance()->Config(this, EditItemInFocusCallback, MenuItemCaption(GetHighlightedIndex()));
        Add(CEditScreen::GetInstance());
    }
}


void
CLibraryMenuScreen::EditItemInFocusCallback(bool bSave)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:EditItemInFocusCallback\n");
    if (!s_pLibraryMenuScreen)
        return;

    s_pLibraryMenuScreen->EditItemInFocusCB(bSave);
}


void
CLibraryMenuScreen::EditItemInFocusCB(bool bSave)
{
    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "lib:EditItemInFocusCB\n");

    bool bCommitChanges = false;

    if (bSave)
    {
        const TCHAR* szNewString = CEditScreen::GetInstance()->GetDataString();
        if (tstrlen(szNewString) == 0)
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Empty String, Aborting Edit\n");
            return;
        }

		// Remember that we've started an update -- this will trigger a content rescan if the device reboots.
		CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);

        CAlertScreen::GetInstance()->Config(this, 100);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_SAVE));
        CAlertScreen::GetInstance()->SetActionText(LS(SID_PREPARING_TO_SAVE_CHANGES));
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_COUNTING_FILES));
        Add(CAlertScreen::GetInstance());
        
        IQueryableContentManager* pQCM           = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
        CDJPlayerState*           pDJPlayerState = CDJPlayerState::GetInstance();
        int                       iDSID          = pDJPlayerState->GetFatDataSource()->GetInstanceID();

        MediaRecordList mrlRecords;
        
        int iAttributeID;
        int iMenuIndex = GetHighlightedIndex();
        int iMenuKey   = m_MenuItems[iMenuIndex].iKey;
        int iNewKey    = -1; // Keep track of where the modified entry gets re-sorted to, so we can keep focus on it.
        
        switch(m_eBrowseMode)
        {

        case GENRE:
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Renaming genre [%w] to [%w]\n", m_MenuItems[iMenuIndex].szValue, szNewString);

            pQCM->GetMediaRecords(mrlRecords, CMK_ALL, CMK_ALL, iMenuKey, iDSID);
            iAttributeID = MDA_GENRE;
            break;
        }
            
        case ARTIST:
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Renaming artist [%w] to [%w]\n", m_MenuItems[iMenuIndex].szValue, szNewString);

            pQCM->GetMediaRecords(mrlRecords, iMenuKey, CMK_ALL, CMK_ALL, iDSID);
            iAttributeID = MDA_ARTIST;
            break;
        }
            
        case ALBUM:
        {
            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Renaming album [%w] to [%w]\n", m_MenuItems[iMenuIndex].szValue, szNewString);

            pQCM->GetMediaRecords(mrlRecords, CMK_ALL, iMenuKey, CMK_ALL, iDSID);
            iAttributeID = MDA_ALBUM;
            break;
        }
            
        case TRACK:
        {
            // extract URL
            int i = 0;
            for (MediaRecordIterator itTrack = m_mrlTracks.GetHead(); itTrack != m_mrlTracks.GetEnd(); ++itTrack)
            {
                if (iMenuIndex == i)
                {	
                    void* pData = 0;
                    (*itTrack)->GetAttribute(MDA_TITLE,&pData);
                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Renaming track title [%w] to [%w]\n", (TCHAR*)pData, szNewString);

                    mrlRecords.PushBack((*itTrack));
                    iAttributeID = MDA_TITLE;

                    break;
                }
                i++;
            }
            break;
        }
            
        case PLAYLIST:
        {
            // extract URL
            int i           = 0;
            for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
            {
                if ((iMenuIndex - 1) == i)
                {
                    // get a pointer to the front of the playlist name (get it from the filename)
                    const char* temp = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                    
                    // strip extension (-4)
                    int len = strlen(temp) - 4;
                    TCHAR* szOrigPl = (TCHAR*)malloc((len+1) * sizeof(TCHAR));
                    CharToTcharN(szOrigPl, temp, len);

                    // if the name hasn't changed, don't do anything.
                    if (tstrcmp(szOrigPl, szNewString) == 0)
                    {
                        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:playlist name didn\'t change\n"); // i told you not to look!!!
                    }
                    else
                    {
                        // create a new URL
                        // TODO:  Find some constants for playlist file and max path lengths
                        char szNewURL[EMAXPATH];
                        char szNewPlaylistName[128];
                        TcharToCharN(szNewPlaylistName, szNewString, 127);
//                        sprintf(szNewURL,"file://A:\\%s.DJP",szNewPlaylistName);
                        CDJPlayerState::GetInstance()->GetFatDataSource()->GetContentRootURLPrefix(szNewURL, EMAXPATH);

                        if (strlen(szNewURL) + 1 /* / */ +  strlen(szNewPlaylistName) + 4 /* .DJP */ + 1 > EMAXPATH)
                        {
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:PlaylistNameTooLong: %s\n", szNewString);
                            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                            CAlertScreen::GetInstance()->SetActionText(LS(SID_BAD_PLAYLIST_NAME));
                            Add(CAlertScreen::GetInstance());
                        }
                        else
                        {
                            strcat(szNewURL, "/");
                            strcat(szNewURL, szNewPlaylistName);
                            strcat(szNewURL, ".DJP");
                            // attempt to rename the playlist file via pc_move
                            DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:Rename [%s] to [%s]\n", (char *)(*itPlaylist)->GetURL(), szNewURL);
                            if (pc_mv(const_cast<char*>(FullFilenameFromURLInPlace((char *)(*itPlaylist)->GetURL())), const_cast<char*>(FullFilenameFromURLInPlace(szNewURL))) == YES)
                            {
                                // The alert screen here is pretty hokey, but make the intent is to make the UI flow
                                // be the same for playlists as it is for genre, artist, etc.
                                char cCount[32];
                                sprintf(cCount, " 1 ");
                                TCHAR tcCount[32];
                                CharToTcharN(tcCount, cCount, 32);
                                
                                TCHAR tcCaption[128];
                                tcCaption[0] = 0;
                                tstrcat(tcCaption, LS(SID_CHANGING));
                                tstrcat(tcCaption, tcCount);
                                tstrcat(tcCaption, LS(SID_TRACK));
                                CAlertScreen::GetInstance()->SetActionText(tcCaption);
                                CAlertScreen::GetInstance()->ResetProgressBar(0, 1);

                                tcCaption[0] = 0;
                                tstrcat(tcCaption, LS(SID_CHANGED));
                                tstrcat(tcCaption, tcCount);
                                tstrcat(tcCaption, LS(SID_OF));
                                tstrcat(tcCaption, tcCount);
                                CAlertScreen::GetInstance()->SetMessageText(tcCaption);
                                CAlertScreen::GetInstance()->UpdateProgressBar(1);
                                
		                        // create a new playlist record
		                        playlist_record_t prNew;
		                        prNew.bVerified = true;
		                        prNew.iDataSourceID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
                                const int iFormat = CPlaylistFormatManager::GetInstance()->FindPlaylistFormat("dpl");
		                        prNew.iPlaylistFormatID = iFormat;
		                        prNew.szURL = szNewURL;

                                // add new playlist entry (URL) into metakit
                                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:AddEntry\n");
                                CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);
                                IPlaylistContentRecord* pPCR = CPlayManager::GetInstance()->GetContentManager()->AddPlaylistRecord(prNew);
                                if (pPCR != NULL)
                                {
                                    // Remove the entry from the content manager.
                                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:RemEntry\n");
                                    CPlayManager::GetInstance()->GetContentManager()->DeletePlaylistRecord((*itPlaylist)->GetID());
                                    bCommitChanges = true;
                                    
                                    iNewKey = pPCR->GetID();
                                }
                                else
                                {
                                    // TODO:  moved playlist but can't add it to metakit
                                    DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:AddPlaylistEntryFailed\n", szOrigPl, szNewString);
                                }
                                // Reset the draw list.
                                m_prlPlaylistMenuItems.Clear();
                                CPlayManager::GetInstance()->GetContentManager()->GetPlaylistRecordsByDataSourceID(m_prlPlaylistMenuItems, iDSID);
                                SetItemCount(m_prlPlaylistMenuItems.Size() + 1);  // 1 for "Play Everything"
                            }
                            else
                            {
                                // TODO: pc_move failed.  notify that we couldn't rename the playlist
                                DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_ERROR, "lib:MovePlaylistFailed\n", szOrigPl, szNewString);
                                CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                                CAlertScreen::GetInstance()->SetActionText(LS(SID_BAD_PLAYLIST_NAME));
                                Add(CAlertScreen::GetInstance());
                            }
                        }
                    }
                    free(szOrigPl);
                    break;
                }
                i++;
            }
            break;
        }
            
        case RADIO:
            break;

        default:
            break;
            
        }

        if (m_eBrowseMode == GENRE ||
            m_eBrowseMode == ARTIST ||
            m_eBrowseMode == ALBUM ||
            m_eBrowseMode == TRACK)
        {
            if (!mrlRecords.IsEmpty())
            {
                char cTotalCount[32];
                sprintf(cTotalCount, " %d ", mrlRecords.Size());
                TCHAR tcTotalCount[32];
                CharToTcharN(tcTotalCount, cTotalCount, 32);
                TCHAR tcCaption[128];
                tcCaption[0] = 0;
                tstrcat(tcCaption, LS(SID_CHANGING));
                tstrcat(tcCaption, tcTotalCount);
                if (mrlRecords.Size() > 1)
                    tstrcat(tcCaption, LS(SID_TRACKS));
                else
                    tstrcat(tcCaption, LS(SID_TRACK));
                CAlertScreen::GetInstance()->SetActionText(tcCaption);
                CAlertScreen::GetInstance()->ResetProgressBar(0, mrlRecords.Size());
                
                MediaRecordIterator itTrack = mrlRecords.GetHead();
                for (int c = 0; c < mrlRecords.Size(); ++c)
                {
                    char cCount[32];
                    sprintf(cCount, " %d ", c);
                    TCHAR tcCount[32];
                    CharToTcharN(tcCount, cCount, 32);
                    TCHAR tcCaption[128];
                    tcCaption[0] = 0;
                    tstrcat(tcCaption, LS(SID_CHANGED));
                    tstrcat(tcCaption, tcCount);
                    tstrcat(tcCaption, LS(SID_OF));
                    tstrcat(tcCaption, tcTotalCount);
                    CAlertScreen::GetInstance()->SetMessageText(tcCaption);
                    CAlertScreen::GetInstance()->UpdateProgressBar(c);
                    
                    IMediaContentRecord* pContentRecord = *(itTrack);
                
                    if (SUCCEEDED(pContentRecord->SetAttribute(iAttributeID, (void*)(szNewString))))
                    {
                        bCommitChanges = true;
                        // todo:  check to see if updating the actual file fails.  this could easily happen
                        //  if we're trying to edit the currently playing song
#ifdef DDOMOD_DJ_BUFFERING
                        CBuffering::GetInstance()->PauseHDAccess();
#endif
                        CMetadataFileTag::GetInstance()->UpdateTag(pContentRecord->GetURL(), pContentRecord);
#ifdef DDOMOD_DJ_BUFFERING
                        CBuffering::GetInstance()->ResumeHDAccess();
#endif
                        iNewKey = pContentRecord->GetID();  // This is only valid for tracks, it will be adjusted later for other fields
                    }
                    else
                    {
                        DEBUGP(DBG_LIBRARY_SCREEN, DBGLEV_TRACE, "info:CommitChanges ERROR: couldn't commit a change\n");
                    }
                    ++itTrack;
                }
                CAlertScreen::GetInstance()->UpdateProgressBar(mrlRecords.Size());

                switch (m_eBrowseMode) {
                    case GENRE: {
                        iNewKey = pQCM->GetGenreKey(szNewString);
                        break;
                    }
                    case ARTIST: {
                        iNewKey = pQCM->GetArtistKey(szNewString);
                        break;
                    }
                    case ALBUM: {
                        iNewKey = pQCM->GetAlbumKey(szNewString);
                        break;
                    }
                }
            }            
        }
        
        
        // Save the content manager DB.
        if (bCommitChanges)
        {
            TCHAR tcCaption[128];
            tcCaption[0] = 0;
            tstrcat(tcCaption, LS(SID_SAVING_INFO_TO));
            TCHAR tcSpace[] = { ' ',0 };
            tstrcat(tcCaption, tcSpace);
            tstrcat(tcCaption, LS(SID_HD));
            CAlertScreen::GetInstance()->SetActionText(tcCaption);
            CAlertScreen::GetInstance()->SetMessageText(LS(SID_PLEASE_WAIT));
            
            ((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
        }

        switch (m_eBrowseMode)
        {
            case GENRE:
            {
                SetConstraints(iMenuKey);
                break;
            }
            case ARTIST:
            {
                SetConstraints(m_iGenreKey, iMenuKey);
                break;
            }
            case ALBUM:
            {
                SetConstraints(m_iGenreKey, m_iArtistKey, iMenuKey);
                break;
            }
            case TRACK:
            {
                SetConstraints(m_iGenreKey, m_iArtistKey, m_iAlbumKey);
                break;
            }
        }

        // Update the selected item now that it may have moved around
        switch (m_eBrowseMode)
        {
            case GENRE:
            case ARTIST:
            case ALBUM:
            {
                for (int i = 0; i < m_MenuItems.Size(); ++i)
                {
                    if (m_MenuItems[i].iKey == iNewKey)
                    {
                        m_iTopIndex = i - 1;
                        break;
                    }
                }
                break;
            }                
            case TRACK:
            {
                int i = 0;
                for (MediaRecordIterator itTrack = m_mrlTracks.GetHead();
                     itTrack != m_mrlTracks.GetEnd(); ++itTrack)
                {
                    if ((*itTrack)->GetID() == iNewKey)
                    {
                        m_iTopIndex = i - 1;
                        break;
                    }
                    ++i;
                }
                break;
            }
            case PLAYLIST:
            {
                int i = 0;
                for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                     itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
                {
                    if ((*itPlaylist)->GetID() == iNewKey)
                    {
                        m_iTopIndex = i;
                        break;
                    }
                    ++i;
                }
                break;
            }
        }
        
        // synch the player screen in case we've edited the currently playing track
        CPlayerScreen::GetPlayerScreen()->RefreshCurrentTrackMetadata();
    }
    else
    {
        DEBUGP( DBG_LIBRARY_SCREEN, DBGLEV_INFO, "lib:CancelEdit\n");
    }

    Draw();
}

void CLibraryMenuScreen::SetHoleSize(CRadioStationQueryResult* pQR)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    if (pQR->GetFilledItemCount() != pQR->GetTotalItemCount())
    {
        SimpleVector<int> svRanges;
        pQR->GetFilledRanges(svRanges);

        if (svRanges.Size() == 0)
        {
            m_iHoleStart = 0;
            m_iHoleEnd   = pQR->GetTotalItemCount();
        }
        else
        {
            DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");
            
            DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning");
            m_iHoleStart = svRanges[1];
            
            if (svRanges.Size() == 4)
            {
                DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end");
                m_iHoleEnd = svRanges[2];
            }
            else
            {
                m_iHoleEnd = pQR->GetTotalItemCount();
            }
        }
    }
    else
    {
        m_iHoleStart = m_iHoleEnd = -1;
    }
}

void CLibraryMenuScreen::SetHoleSize(CGeneralQueryResult* pQR)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    if (pQR->GetFilledItemCount() != pQR->GetTotalItemCount())
    {
        SimpleVector<int> svRanges;
        pQR->GetFilledRanges(svRanges);

        if (svRanges.Size() == 0)
        {
            m_iHoleStart = 0;
            m_iHoleEnd   = pQR->GetTotalItemCount();
        }
        else
        {
            DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");

            DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning (svRanges[0] = %d)\n", svRanges[0]);
            m_iHoleStart = svRanges[1];
            
            if (svRanges.Size() == 4)
            {
                DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end (svRanges[2] = %d, svRanges[3] = %d, total count = %d)\n", svRanges[2], svRanges[3], pQR->GetTotalItemCount());
                m_iHoleEnd = svRanges[2];
            }
            else
            {
                m_iHoleEnd = pQR->GetTotalItemCount();
            }
        }
    }
    else
    {
        m_iHoleStart = m_iHoleEnd = -1;
    }
}

void CLibraryMenuScreen::SetHoleSize(CMediaQueryResult* pQR)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    if (pQR->GetFilledItemCount() != pQR->GetTotalItemCount())
    {
        SimpleVector<int> svRanges;
        pQR->GetFilledRanges(svRanges);

        if (svRanges.Size() == 0)
        {
            m_iHoleStart = 0;
            m_iHoleEnd   = pQR->GetTotalItemCount();
        }
        else
        {
            DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");
            
            DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning");
            m_iHoleStart = svRanges[1];
            
            if (svRanges.Size() == 4)
            {
                DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end");
                m_iHoleEnd = svRanges[2];
            }
            else
            {
                m_iHoleEnd = pQR->GetTotalItemCount();
            }
        }
    }
    else
    {
        m_iHoleStart = m_iHoleEnd = -1;
    }
}

int CLibraryMenuScreen::GetStartOfNextQueryBlock(CGeneralQueryResult* pQR, int iBlockSize)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    DBASSERT(DBG_LIBRARY_SCREEN, (pQR->GetFilledItemCount() != pQR->GetTotalItemCount()), "No more querying to do");

    SimpleVector<int> svRanges;
    pQR->GetFilledRanges(svRanges);
    DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");

    DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning");
    if (svRanges.Size() == 4)
    {
        DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end");
        if (svRanges[1] <= svRanges[3])
            return svRanges[0] + svRanges[1];
        else
            return svRanges[2] - iBlockSize;
    }
    else
    {
        int iLastBlockStart = pQR->GetTotalItemCount() - iBlockSize;
        if (iLastBlockStart < 0)
            iLastBlockStart = 0;
        return iLastBlockStart;
    }
}

int CLibraryMenuScreen::GetStartOfNextQueryBlock(CMediaQueryResult* pQR, int iBlockSize)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    DBASSERT(DBG_LIBRARY_SCREEN, (pQR->GetFilledItemCount() != pQR->GetTotalItemCount()), "No more querying to do");

    SimpleVector<int> svRanges;
    pQR->GetFilledRanges(svRanges);
    DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");

    DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning");
    if (svRanges.Size() == 4)
    {
        DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end");
        if (svRanges[1] <= svRanges[3])
            return svRanges[0] + svRanges[1];
        else
            return svRanges[2] - iBlockSize;
    }
    else
    {
        int iLastBlockStart = pQR->GetTotalItemCount() - iBlockSize;
        if (iLastBlockStart < 0)
            iLastBlockStart = 0;
        return iLastBlockStart;
    }
}

int CLibraryMenuScreen::GetStartOfNextQueryBlock(CRadioStationQueryResult* pQR, int iBlockSize)
{
    DBASSERT(DBG_LIBRARY_SCREEN, pQR, "pQR is not NULL");
    DBASSERT(DBG_LIBRARY_SCREEN, (pQR->GetFilledItemCount() != pQR->GetTotalItemCount()), "No more querying to do");

    SimpleVector<int> svRanges;
    pQR->GetFilledRanges(svRanges);
    DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges.Size() == 2) || (svRanges.Size() == 4)), "More than one hole");

    DBASSERT(DBG_LIBRARY_SCREEN, svRanges[0] == 0, "First block not at beginning");
    if (svRanges.Size() == 4)
    {
        DBASSERT(DBG_LIBRARY_SCREEN, ((svRanges[2] + svRanges[3]) == pQR->GetTotalItemCount()), "Second block not at end");
        if (svRanges[1] <= svRanges[3])
            return svRanges[0] + svRanges[1];
        else
            return svRanges[2] - iBlockSize;
    }
    else
    {
        int iLastBlockStart = pQR->GetTotalItemCount() - iBlockSize;
        if (iLastBlockStart < 0)
            iLastBlockStart = 0;
        return iLastBlockStart;
    }
}

const TCHAR* CLibraryMenuScreen::BuildQueryingString(const TCHAR* tcPrefix)
{
    char cRange[128];
    sprintf(cRange, " %d - %d", m_iHoleStart, m_iHoleEnd - 1);
    TCHAR tcRange[128];
    CharToTcharN(tcRange, cRange, 128);

    static TCHAR tcCaption[128]; // This will be returned, so must make it static, not auto
    tcCaption[0] = 0;
    tstrcat(tcCaption, tcPrefix);
    tstrcat(tcCaption, tcRange);

    return tcCaption;
}

int CLibraryMenuScreen::AdjustIndex(int iIndex)
{
    // Update iMenuIndex to reflect a hole if one exists.
    int iItemCount;
    switch (m_eBrowseMode) {
        case TRACK: {
            iItemCount = m_miTracks.Size();
            break;
        }
        case RADIO: {
            iItemCount = m_miRadioStations.Size();
            break;
        }
        default: {
            iItemCount = m_MenuItems.Size();
            break;
        }
    }
    if (iItemCount == 0) {
        // This will get adjusted correctly later when there are some entries
        return iIndex;
    }
    else if (m_iHoleStart != -1)
    {
        if (m_iDesiredIndex != -1)
        {
            if ((m_iDesiredIndex < m_iHoleStart) || (m_iDesiredIndex > m_iHoleEnd))
            {
                iIndex = m_iDesiredIndex;
                m_iDesiredIndex = -1;
            }
        }
        
        if (iIndex >= m_iHoleEnd)
        {
            iIndex -= (GetHoleSize() - 1);
        }
        else if (iIndex >= m_iHoleStart)
        {
            iIndex = m_iHoleStart;
        }
    }
    
    return iIndex;
}
