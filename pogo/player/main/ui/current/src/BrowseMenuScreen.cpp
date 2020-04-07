//........................................................................................
//........................................................................................
//.. File Name: BrowseMenuScreen.cpp														..
//.. Date: 09/20/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/20/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/BrowseMenuScreen.h>
#include <main/main/PlaylistConstraint.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>
#include <main/main/FatHelper.h>

#include <core/playmanager/PlayManager.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_BROWSE_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_BROWSE_SCREEN);

CBrowseMenuScreen* CBrowseMenuScreen::s_pBrowseMenuScreen = 0;

struct tMediaRecordMetadataMapping {
    IMediaContentRecord* record;
    TCHAR* string;
};

// This is a singleton class.
CScreen*
CBrowseMenuScreen::GetBrowseMenuScreen()
{
	if (!s_pBrowseMenuScreen) {
		s_pBrowseMenuScreen = new CBrowseMenuScreen();
	}
	return s_pBrowseMenuScreen;
}

CBrowseMenuScreen::CBrowseMenuScreen()
	: CDynamicMenuScreen(NULL, SID_ALBUMS),
    m_iArtistKey(CMK_ALL),
    m_iAlbumKey(CMK_ALL),
    m_iGenreKey(CMK_ALL),
	m_eBrowseMode(ARTIST),
	m_eMenuEnterMode(ARTIST)
{

}

CBrowseMenuScreen::~CBrowseMenuScreen()
{
}

bool 
CBrowseMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    switch(m_eBrowseMode)
	{
		case ARTIST:
            return true;
		case ALBUM:
            return true;
		case TRACK:
            return false;
		default:
            // question reality.
            // hmm...
            // reality is our name, given in faith, for what we know.
            // how can that be questioned?
            return false;
	}
	
}

const TCHAR* 
CBrowseMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if (m_eBrowseMode == TRACK)
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
        void* pData = 0;
        // look up the metadata title
        if (SUCCEEDED((*m_itTrack)->GetAttribute(MDA_TITLE,&pData)))
            return (TCHAR*) pData;
        // fall back on the filename if necessary
        else
        {
            DBASSERT( DBG_BROWSE_SCREEN, false, "Track w/o Title\n");
/*
            static TCHAR filename[PLAYLIST_STRING_SIZE];
            const char* longname = (*m_itTrack)->GetFileNameRef()->LongName();
            CharToTchar(filename,longname);
            int idx = tstrlen(filename) - 1;
            while (filename[idx] != (TCHAR) '.')
                --idx;
            filename[idx] = 0;
            return filename;
 */
        }
    }
    else
        return m_MenuItems[iMenuIndex].szValue;
}

PegBitmap* 
CBrowseMenuScreen::MenuItemBitmap(int iMenuIndex)
{
    return &gbEmptyBitmap;
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	// use the current selection to set the current query info 
	// as the playlist criteria and exit to the player screen
    CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
    bool bCrntStillInList = false;
    switch(m_eBrowseMode)
	{
		case ARTIST:
        {
            pPC->Constrain(m_MenuItems[iMenuIndex].iKey,CMK_ALL,m_iGenreKey);
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
            break;
        }
		case ALBUM:
        {
            pPC->Constrain(m_iArtistKey, m_MenuItems[iMenuIndex].iKey);
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
			break;
        }
		case TRACK:
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
            pPC->Constrain();
            pPC->SetTrack((*m_itTrack));
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
			break;
		default:
			break;
	}
	((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CBrowseMenuScreen::GotoSubMenu(int iMenuIndex)
{
    if (m_cItems == 0)
        return;
    // exit if there is no sub menu to go to.
    if (!MenuItemHasSubMenu(iMenuIndex))
        return;
    // reconfigure this screen to show the right query
	switch(m_eBrowseMode)
	{
		case ARTIST:
        {
            m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
            m_iArtistTopIndex = m_iTopIndex;
            m_iArtistLineIndex = m_iLineIndex;
            ResetToTop();
			SetBrowseMode(ALBUM, false);
            SetConstraints(m_iGenreKey, m_MenuItems[iMenuIndex].iKey, CMK_ALL);
			break;
        }
		case ALBUM:
        {
            m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
            m_iAlbumTopIndex = m_iTopIndex;
            m_iAlbumLineIndex = m_iLineIndex;
            ResetToTop();
			SetBrowseMode(TRACK, false);
            SetConstraints(m_iGenreKey, m_iArtistKey, m_MenuItems[iMenuIndex].iKey);
			break;
        }
		case TRACK:
			break;
		default:
			break;
	}
	Draw();
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CBrowseMenuScreen::GotoPreviousMenu()
{
	// reconfigure this screen to show the right query
	if (m_eBrowseMode == m_eMenuEnterMode)
	{
		Parent()->Add(m_pParent);
		Parent()->Remove(this);
		Presentation()->MoveFocusTree(m_pParent);
		return;
	}

	switch(m_eBrowseMode)
	{
		case ALBUM:
        {
            m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
            SetBrowseMode(ARTIST, false);
            SetConstraints(m_iGenreKey);
            m_iTopIndex = m_iArtistTopIndex;
            m_iLineIndex = m_iArtistLineIndex;
            ForceRedraw();
			break;
        }
		case TRACK:
        {
            m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
			SetBrowseMode(ALBUM, false);
            SetConstraints(m_iGenreKey, m_iArtistKey);
            m_iTopIndex = m_iAlbumTopIndex;
            m_iLineIndex = m_iAlbumLineIndex;
            ForceRedraw();
			break;
        }
		default:
			break;
	}

	Draw();
}

// allow other screens to configure this screens browse mode
void
CBrowseMenuScreen::SetBrowseMode(eBrowseMode eMode, bool bMenuEnterMode)
{
	PegRect newRect = m_pScreenTitle->mReal;

	m_eBrowseMode = eMode;
	if(bMenuEnterMode)
		m_eMenuEnterMode = eMode;

	switch(m_eBrowseMode)
	{
		case ARTIST:
			m_wScreenTitleSID = SID_ARTIST;
			newRect.wTop = mReal.wTop + 10;
			newRect.wBottom = mReal.wTop + 19;
			break;

		case ALBUM:
			m_wScreenTitleSID = SID_ALBUM;
			newRect.wTop = mReal.wTop + 20;
			newRect.wBottom = mReal.wTop + 29;
			break;

		case TRACK:
			m_wScreenTitleSID = SID_TRACK;
			newRect.wTop = mReal.wTop + 30;
			newRect.wBottom = mReal.wTop + 39;
			break;

		default:
			break;
	}

	m_pScreenTitle->Resize(newRect);
}


int CompareContentKeyValues(const void* a, const void* b)
{
    return tstrcmp(((cm_key_value_record_t*)a)->szValue,((cm_key_value_record_t*)b)->szValue);
}

void SortContentKeyValueVector(ContentKeyValueVector& keyValues)
{
    keyValues.QSort(CompareContentKeyValues);
}

bool MediaRecordTrackNameLessThan(void* left, void* right)
{
    void *pDataLeft;
    void *pDataRight;
    ((IMediaContentRecord*)left)->GetAttribute(MDA_TITLE, &pDataLeft);
    ((IMediaContentRecord*)right)->GetAttribute(MDA_TITLE, &pDataRight);
    return (tstrcmp((TCHAR*)pDataLeft,(TCHAR*)pDataRight) <= 0);
}

void CBrowseMenuScreen::SetConstraints(int iGenreKey, int iArtistKey, int iAlbumKey)
{
    m_iGenreKey = iGenreKey;
    m_iArtistKey = iArtistKey;
    m_iAlbumKey = iAlbumKey;
    IQueryableContentManager* pQCM = (IQueryableContentManager*) CPlayManager::GetInstance()->GetContentManager();
    switch (m_eBrowseMode)
    {
		case ARTIST:
            m_MenuItems.Clear();
            pQCM->GetArtists(m_MenuItems, m_iAlbumKey, m_iGenreKey);
            if (m_MenuItems.Size() > 1)
                SortContentKeyValueVector(m_MenuItems);
            SetItemCount(m_MenuItems.Size());
            break;

		case ALBUM:
            m_MenuItems.Clear();
            pQCM->GetAlbums(m_MenuItems, m_iArtistKey, m_iGenreKey);
            if (m_MenuItems.Size() > 1)
                SortContentKeyValueVector(m_MenuItems);
            SetItemCount(m_MenuItems.Size());
			break;

		case TRACK:
        {
            m_mrlTracks.Clear();
            pQCM->GetMediaRecords(m_mrlTracks,iArtistKey, iAlbumKey, iGenreKey);
            SortMediaRecordListByTitle(&m_mrlTracks);

            m_iIterIndex = 0;
            m_itTrack = m_mrlTracks.GetHead();
            SetItemCount(m_mrlTracks.Size());
			break;
        }
		default:
			break;
    }
}

void CBrowseMenuScreen::Minimize()
{
    m_iGenreKey = CMK_ALL;
    m_iArtistKey = CMK_ALL;
    m_iAlbumKey = CMK_ALL;
    m_eBrowseMode = (eBrowseMode)ARTIST;
    m_MenuItems.Clear();
    m_mrlTracks.Clear();
    SetItemCount(0);
    // clear out position memory members
    m_iArtistTopIndex = 0;
    m_iArtistLineIndex = 0;
    m_iAlbumTopIndex = 0;
    m_iAlbumLineIndex = 0;
    m_iTopIndex = 0;
    m_iLineIndex = 0;
}

void CBrowseMenuScreen::SortMediaRecordListByTitle(ContentRecordSortList* mrlTracks)
{
    int nRecords = mrlTracks->Size();
    if (nRecords < 2)
        return;
    tMediaRecordMetadataMapping * map = new tMediaRecordMetadataMapping[nRecords+1];
    InitMappingToListOrder(map, mrlTracks);
    PopulateMapWithTitleMetadata(map, nRecords);
    SortMap(map,nRecords);
    ReorderListByMap(map, mrlTracks);
    delete [] map;
}

void CBrowseMenuScreen::InitMappingToListOrder(tMediaRecordMetadataMapping* map, ContentRecordSortList* mrlTracks)
{
    int i = 0;
    for (ContentRecordIterator itTrack = mrlTracks->GetHead(); itTrack != mrlTracks->GetEnd(); ++itTrack, ++i)
    {
        map[i].record = *itTrack;
    }
}

void CBrowseMenuScreen::PopulateMapWithTitleMetadata(tMediaRecordMetadataMapping* map, int nRecords)
{
    for (int i = 0; i < nRecords; ++i)
    {
        if (FAILED(map[i].record->GetAttribute(MDA_TITLE, (void**)&(map[i].string))))
        {
            DEBUGP( DBG_BROWSE_SCREEN, DBGLEV_INFO, "couldn't retrieve mrl title!\n"); 
        }
    }
}

int CompareMappings(const void* a, const void* b)
{
    return tstrcmp(((tMediaRecordMetadataMapping*)a)->string, ((tMediaRecordMetadataMapping*)b)->string);
}

void CBrowseMenuScreen::SortMap(tMediaRecordMetadataMapping* map, int nRecords)
{
    qsort( (void*) map, nRecords, sizeof (tMediaRecordMetadataMapping), CompareMappings);
}

void CBrowseMenuScreen::ReorderListByMap(tMediaRecordMetadataMapping* map, ContentRecordSortList* mrlTracks)
{
    int nRecords = mrlTracks->Size();
    mrlTracks->Clear();
    for (int i = 0; i < nRecords; ++i)
        mrlTracks->PushBack(map[i].record);
}
