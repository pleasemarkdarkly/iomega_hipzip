//........................................................................................
//........................................................................................
//.. File Name: FolderMenuScreen.cpp														..
//.. Date: 09/28/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CFolderMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@fullplaymedia.com								..
//.. Modification date: 09/28/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Fullplay Media Systems Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Fullplay Media Systems.        ..
//.. Contact Information: www.fullplaymedia.com												..
//........................................................................................
//........................................................................................
#include <main/ui/FolderMenuScreen.h>

#include <main/main/FatHelper.h>
#include <main/main/PlaylistConstraint.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>
#include <main/main/AppSettings.h>     // PLAYLIST_STRING_SIZE

#include <codec/codecmanager/CodecManager.h>
#include <core/playmanager/PlayManager.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <playlist/common/Playlist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <util/datastructures/SimpleList.h>
#include <fs/fat/sdapi.h>

#include <ctype.h>      /* isspace */
#include <stdlib.h>     /* malloc */
#include <stdio.h>      /* sprintf */

#include <util/debug/debug.h>
DEBUG_MODULE_S(FOLDER_MENUSCREEN, DBGLEV_DEFAULT);
DEBUG_USE_MODULE(FOLDER_MENUSCREEN);

CFolderMenuScreen* CFolderMenuScreen::s_pFolderMenuScreen = 0;

// This is a singleton class.
CScreen*
CFolderMenuScreen::GetFolderMenuScreen()
{
	if (!s_pFolderMenuScreen) {
		s_pFolderMenuScreen = new CFolderMenuScreen();
	}
	return s_pFolderMenuScreen;
}


CFolderMenuScreen::CFolderMenuScreen()
  : CDynamicMenuScreen(NULL, SID_SET),
    m_nFolderDepth(0),
    m_prfCurrDir(0),
    m_itrDirEntry(NULL),
    m_nitrDirEntry(0)
{
	SetMenuTitle(LS(SID_FOLDERS));
    m_lstParentScrollSettings.Clear();
}

CFolderMenuScreen::~CFolderMenuScreen()
{
}


bool 
CFolderMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    if (iMenuIndex < m_cSubFolders)
        return true;
    return false;
}

void CFolderMenuScreen::SyncItrToIndex(int nIndex)
{
    // init the itr if necessary
    if (m_itrDirEntry == NULL)
    {
        m_nitrDirEntry = 0;
        IFileNameNode* node = **m_prfCurrDir;
        FileNodeList* lstChildren = node->Children();
        m_itrDirEntry = lstChildren->GetHead();
    }
    //walk the itr to the correct location
    while (m_nitrDirEntry < nIndex)
    {
        ++m_nitrDirEntry;
        ++m_itrDirEntry;
    }
    while (m_nitrDirEntry > nIndex)
    {
        --m_nitrDirEntry;
        --m_itrDirEntry;
    }
}

const TCHAR* 
CFolderMenuScreen::MenuItemCaption(int iMenuIndex)
{
    SyncItrToIndex(iMenuIndex);
    // copy the long name into a temporary wide string
    static TCHAR temp[PLAYLIST_STRING_SIZE];
    CharToTchar(temp,(*m_itrDirEntry)->LongName());
    // and return it.
    return temp;
}

PegBitmap* 
CFolderMenuScreen::MenuItemBitmap(int iMenuIndex)
{
    return &gbEmptyBitmap;
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CFolderMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    // deal with empty folders
    if (m_cItems == 0)
        return;
    SyncItrToIndex(iMenuIndex);
	// use the current selection to set the current query info 
	// as the playlist criteria and exit to the player screen
    //FolderEntry* fe = m_vFolderEntries[iMenuIndex];
    bool bExit = false;
    if(iMenuIndex < m_cSubFolders)
	{
		// clear the current playlist, and start a recursive addition rooted at this entry.
        CreateRecursiveFolderPlaylist((*m_itrDirEntry));
        bExit = true;            
	}
	else
	{
        char* ext = ExtensionFromFilename((*m_itrDirEntry)->Name());
        CPlaylistConstraint* pPC = CPlaylistConstraint::GetInstance();
        bool bCrntStillInList;
        if (IsPlayableExtension(ext))
        {
            pPC->Constrain();
            IFileNameRef* pRef = new CStoreFileNameRef (*m_itrDirEntry);
            IMediaContentRecord* pRec = CPlayManager::GetInstance()->GetContentManager()->GetMediaRecord(pRef);
            delete pRef;
            pPC->SetTrack(pRec);
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
            bExit = true;            
        }
        else if (IsPlaylistExtension(ext))
        {
            pPC->Constrain();
            IFileNameRef* pRef = new CStoreFileNameRef (*m_itrDirEntry);
            pPC->SetPlaylistFileNameRef(pRef);                                // constraint object will duplicate reference
            delete pRef;
            pPC->UpdatePlaylist(&bCrntStillInList);
            if (!bCrntStillInList)
                pPC->SyncPlayerToPlaylist();
            CPlayManager::GetInstance()->Play();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchControlSymbol();
            bExit = true;            
        }
	}
    if (bExit)
    {
        Minimize();
        ResetToRoot();
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
    }
}

// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CFolderMenuScreen::GotoSubMenu(int iMenuIndex)
{
    // exit if there is no sub menu to go to.
    if (!MenuItemHasSubMenu(iMenuIndex))
        return;
    m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
    // deal with empty folders
    if (m_cItems == 0)
        return;
    EnterFolder(iMenuIndex);
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CFolderMenuScreen::GotoPreviousMenu()
{
    m_aryPrevMenuArrows[m_iLineIndex + m_iTitleOffset]->SetIcon(&gbEmptyBitmap);
    if (m_nFolderDepth != 0)
        ReturnToParentFolder();                                        
    else
    {
        Minimize();
        ResetToRoot();
    	((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->HideMenus();
    }
}

void
CFolderMenuScreen::PopulateFolderContents()
{
    m_cSubFolders = 0;
    m_nitrDirEntry = 0;
    m_itrDirEntry = NULL;

    ((CDirFileNameNode*)(**m_prfCurrDir))->SortByIsDir();
    m_itrDirEntry = (**m_prfCurrDir)->Children()->GetHead();
    while (m_itrDirEntry != (**m_prfCurrDir)->Children()->GetEnd() && (*m_itrDirEntry)->IsDir())
    {
        ++m_itrDirEntry;
        ++m_cSubFolders;
    }
    m_itrDirEntry = (**m_prfCurrDir)->Children()->GetHead();
    
    SetItemCount((*m_prfCurrDir).Children().Size());
}

// move the screen focus into a subfolder of the current folder.
bool
CFolderMenuScreen::EnterFolder(int iFolderIndex)
{
    SyncItrToIndex(iFolderIndex);

    delete m_prfCurrDir;
    m_prfCurrDir = (CStoreFileNameRef*)(*m_itrDirEntry)->GetRef();

    ++m_nFolderDepth;

    FolderScrollSettings* fss = new FolderScrollSettings;
    fss->lineindex = m_iLineIndex;
    fss->topindex= m_iTopIndex;
    m_lstParentScrollSettings.PushBack(fss);
    
    PopulateFolderContents();
    ResetToTop();
    Draw();
    return true;
}

// move the screen focus up into the immediate parent folder.
void
CFolderMenuScreen::ReturnToParentFolder()
{
    if (m_nFolderDepth == 0)
        return;
    // update any files added or removed from the current playlist.
    --m_nFolderDepth;

    CStoreFileNameRef* parent = (CStoreFileNameRef*)(**m_prfCurrDir)->Parent()->GetRef();
    delete m_prfCurrDir;
    m_prfCurrDir = parent;
    // set m_iTopIndex to a suitable item so that the child folder is visible, and
    // set m_iLineIndex to the correct child folder that we just came out of.
    PopulateFolderContents();
    FolderScrollSettings* fss = m_lstParentScrollSettings.PopBack();
    m_iLineIndex = fss->lineindex;
    m_iTopIndex = fss->topindex;
    delete fss;
    Draw();
}

bool CFolderMenuScreen::IsPlayableExtension(char* szFileExtension)
{
    return (CCodecManager::GetInstance()->FindCodecID(szFileExtension) != 0);
}

void CFolderMenuScreen::Init()
{
    ResetToRoot();
    PopulateFolderContents();
}

// reset the current path/URL to the root directory
void CFolderMenuScreen::ResetToRoot()
{
    delete m_prfCurrDir;
    m_prfCurrDir = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->GetRoot();
}

// take care of cleanup that occurs directory to directory
void CFolderMenuScreen::Minimize()
{
    m_cItems = 0;
    delete m_prfCurrDir;
    m_prfCurrDir = 0;
    m_cSubFolders = 0;
    m_nitrDirEntry = 0;
    m_itrDirEntry = 0;
    while (m_lstParentScrollSettings.Size() > 0)
    {
        FolderScrollSettings* fss = m_lstParentScrollSettings.PopFront();
        delete fss;
    }
}

void CFolderMenuScreen::CreateRecursiveFolderPlaylist(IFileNameNode* listroot)
{
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    int iCurrentEntryId = 0;
    if (IPlaylistEntry* entry = pPL->GetCurrentEntry())
        iCurrentEntryId = entry->GetContentRecord()->GetID();
    pPL->Clear();

    FileNodeList lstDirs;
    lstDirs.PushBack(listroot);
    IFileNameNode* dir;
    MediaRecordList lstTracks;

    while (lstDirs.Size() > 0)
    {
        dir = lstDirs.PopFront();
        for (FileNodeListItr child = dir->Children()->GetHead(); child != dir->Children()->GetEnd(); ++child)
        {
            if ((*child)->IsDir())
                lstDirs.PushBack(*child);
            else
                if (IsPlayableExtension(ExtensionFromFilename((*child)->Name())))
                    lstTracks.PushBack((IMediaContentRecord*)(*child)->GetContentRecord());
        }
    }  
    pPL->AddEntries(lstTracks);
    lstTracks.Clear();
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->UpdateConstraintDots(true, false, false, false);
    
    TCHAR temp[PLAYLIST_STRING_SIZE];
    CharToTchar(temp,listroot->LongName());
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetSetText(temp);
    bool bCrntStillInList;
    CPlaylistConstraint::GetInstance()->ResyncCurrentPlaylistEntryById(iCurrentEntryId,&bCrntStillInList);
    if (!bCrntStillInList)
        CPlaylistConstraint::GetInstance()->SyncPlayerToPlaylist();
    CPlayManager::GetInstance()->Play();
}