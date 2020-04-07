//........................................................................................
//........................................................................................
//.. File Name: BrowseFoldersMenuScreen.cpp														..
//.. Date: 09/05/2001																	..
//.. Author(s): Daniel Bolstad															..
//.. Description of content: implementation of CBrowseFoldersMenuScreen class	 				..
//.. Usage: Controls main menu															..
//.. Last Modified By: Daniel Bolstad  danb@iobjects.com								..
//.. Modification date: 09/05/2001														..
//........................................................................................
//.. Copyright:(c) 1995-2002 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
#include <main/demos/ssi_neo/ui/BrowseFoldersMenuScreen.h>

#include <main/demos/ssi_neo/ui/Bitmaps.h>
#include <main/demos/ssi_neo/ui/Keys.h>
#include <main/demos/ssi_neo/ui/Strings.hpp>
#include <main/demos/ssi_neo/ui/UI.h>
#include <main/demos/ssi_neo/main/FatHelper.h>

#include <codec/codecmanager/CodecManager.h>
#include <core/playmanager/PlayManager.h>
#include <playlist/common/Playlist.h>
#include <ctype.h>      /* isspace */
#include <stdlib.h>     /* malloc */

CBrowseFoldersMenuScreen* CBrowseFoldersMenuScreen::s_pBrowseFoldersMenuScreen = 0;

void PopPathSegment(char* szPath);
void PushURLSegment(char** pszURL, char* szSubFolder);
bool PushPathSegment(char* szPath, char* szSubFolder);

// todo: this structure is for testing only...
// the real info should be from the content manager
/*
static DynamicMenuItemRec s_menuItems[] =
{
	{ &gbEmptyBitmap, SID_FOLDERS, true, false, false },
	{ &gbEmptyBitmap, SID_FOLDERS, true, false, false },
	{ &gbEmptyBitmap, SID_FOLDERS, true, false, false },
	{ &gbEmptyBitmap, SID_PLAY_ALL, false, false, true },
	{ &gbEmptyBitmap, SID_TRACKS, false, false, false },
	{ &gbEmptyBitmap, SID_TRACKS, false, false, false },
	{ &gbEmptyBitmap, SID_TRACKS, false, false, false }
};
*/

// This is a singleton class.
CScreen*
CBrowseFoldersMenuScreen::GetBrowseFoldersMenuScreen()
{
	if (!s_pBrowseFoldersMenuScreen) {
		s_pBrowseFoldersMenuScreen = new CBrowseFoldersMenuScreen();
	}
	return s_pBrowseFoldersMenuScreen;
}


CBrowseFoldersMenuScreen::CBrowseFoldersMenuScreen()
  : CDynamicMenuScreen(NULL, SID_BROWSE_FOLDERS), m_nFolderDepth(0)
{
	// todo: this is for testing
	//SetItemCount(sizeof(s_menuItems) / sizeof(DynamicMenuItemRec));
    strcpy (m_szCurrentPath,"A:\\");
    m_szCurrentURL = 0;
    PushURLSegment(&m_szCurrentURL,"file://A:");
}

CBrowseFoldersMenuScreen::~CBrowseFoldersMenuScreen()
{
}


bool 
CBrowseFoldersMenuScreen::MenuItemIsChecked(int iMenuIndex)
{
//	return s_menuItems[iMenuIndex].bIsChecked;
    return m_vFolderEntries[iMenuIndex]->bSelected;
}

bool 
CBrowseFoldersMenuScreen::MenuItemHasSubMenu(int iMenuIndex)
{
    if (iMenuIndex < m_cSubFolders)
        // the entry is a folder
        return true;
    else if (iMenuIndex == m_vFolderEntries.Size())
        // last item in the screen... // (epg,10/3/2001): TODO: WTF is this for?  possibly never invoked.
        return true;
    // normal files don't have submenus.
    return false;
    //return s_menuItems[iMenuIndex].bHasSubmenu;
}


const TCHAR* 
CBrowseFoldersMenuScreen::MenuItemCaption(int iMenuIndex)
{
    static TCHAR temp[PLAYLIST_STRING_SIZE];
    CharToTchar(temp,m_vFolderEntries[iMenuIndex]->szCaption);
    return temp;
    /*	
    if(s_menuItems[iMenuIndex].bSelectAll) // Play All
		return LS(SID_PLAY_ALL);
	else
		return LS(s_menuItems[iMenuIndex].wCaptionSID);
    */
}


PegBitmap* 
CBrowseFoldersMenuScreen::MenuItemBitmap(int iMenuIndex)
{
	//return s_menuItems[iMenuIndex].pBitmap;
    return &gbEmptyBitmap;
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CBrowseFoldersMenuScreen::ProcessMenuOption(int iMenuIndex)
{
	int i = 0;

	switch (iMenuIndex)
	{
		case -1:	// Exit
            if (m_nFolderDepth != 0)
            {
                ReturnToParentFolder();                                        
                ForceRedraw();
            }
            else
            {
			    Parent()->Add(m_pParent);
			    Parent()->Remove(this);
			    Presentation()->MoveFocusTree(m_pParent);
			    ResetToTop();
            }
			break;

		default:
			if(iMenuIndex == m_cSubFolders) // Play All
			{
				for(i = iMenuIndex + 1; i < m_vFolderEntries.Size() - 1; i++)
					m_vFolderEntries[i]->bSelected = true;
				ForceRedraw();
				break;
			}

			if(iMenuIndex < m_cSubFolders)
			{
				// go into the subdirectory and redraw the screen
                EnterFolder(iMenuIndex);
			}
			else
			{
				// add or remove the file from the current playlist
                if (IsPlayableExtension(m_vFolderEntries[iMenuIndex]->szCaption))
                    m_vFolderEntries[iMenuIndex]->bSelected = !m_vFolderEntries[iMenuIndex]->bSelected;
				/*
                if(s_menuItems[iMenuIndex].bIsChecked)
				{
					s_menuItems[iMenuIndex].bIsChecked = false;
				}
				else
				{
					s_menuItems[iMenuIndex].bIsChecked = true;
				}
                */
			}
			ForceRedraw();
			break;
	};
}

// iterate over entries in the current dir and update the playlist if any have been selected or deselected.
void
CBrowseFoldersMenuScreen::UpdatePlaylistMembership()
{
    for (int i = m_cSubFolders + 1; i < m_vFolderEntries.Size(); ++i)
    {
        if (m_vFolderEntries[i]->bSelected != (m_vFolderEntries[i]->pPlaylistEntry!=NULL))
        {
            if (m_vFolderEntries[i]->bSelected)
            {
                // (epg,10/3/2001): TODO: don't I need the full URL here, not just the LFN, as it appears I'm passing in?   hmmmmmmmm.
                IMediaContentRecord* pMR = CPlayManager::GetInstance()->GetContentManager()->GetMediaRecord(m_vFolderEntries[i]->szCaption);
                CPlayManager::GetInstance()->GetPlaylist()->AddEntry( pMR );
            }
            else
            {
                // the only method I can find to remove a pl entry is to iterate over the list until we find the matching file...
                // instead of that, I'll store a pointer to the entry during check pl membership so it is quicker here.  there
                // I'm already iterating anyway, and all I have to do is store a pointer instead of the boolean membership flag.
                CPlayManager::GetInstance()->GetPlaylist()->DeleteEntry(m_vFolderEntries[i]->pPlaylistEntry);
            }
        }
    }
}

// iterate over entries in the current dir and check for membership in the current playlist.

void
CBrowseFoldersMenuScreen::CheckPlaylistMembership()
{
    IPlaylist* pPL = CPlayManager::GetInstance()->GetPlaylist();
    // make a copy of the current pl that only contains entries in this dir
    SimpleList<StringEntryPair*> lstPlaylistEntries;
    lstPlaylistEntries.Clear();
    int plindex = 0;
    IPlaylistEntry* pEntry = pPL->GetEntry(plindex);
    char urlpath[PLAYLIST_STRING_SIZE];
    strcpy (urlpath,"file://");
    strcat (urlpath,m_szCurrentPath);
    while (pEntry)
    {
        unsigned int len1 = PathlenFromURL(pEntry->GetContentRecord()->GetURL());
        if ((len1 == strlen(urlpath)) && !strncmp(urlpath,pEntry->GetContentRecord()->GetURL(),len1))
        //if (!strncmp(this->urlpath,pEntry->GetContentRecord()->GetURL(),strlen(urlpath)))
        {
            // this entry is in the current dir, so add it to the list.
            StringEntryPair *pair = new StringEntryPair;
            pair->entry = pEntry;
            pair->name = FilenameFromURL(pEntry->GetContentRecord()->GetURL());
            lstPlaylistEntries.PushBack(pair);
        }
        pEntry = pPL->GetEntry(++plindex);
    }
    // now we can do the monster loop only on entries in the current dir, and only string compare the actual filenames, no URLs needed.
    for (int i = m_cSubFolders + 1; i < m_vFolderEntries.Size(); ++i)
    {
        m_vFolderEntries[i]->pPlaylistEntry = 0;
        for (SimpleListIterator<StringEntryPair*> it = lstPlaylistEntries.GetHead(); it != lstPlaylistEntries.GetEnd(); ++it)
        {
            if (!strcmp((*it)->name,m_vFolderEntries[i]->szCaption))
            {
                m_vFolderEntries[i]->pPlaylistEntry = (*it)->entry;
                m_vFolderEntries[i]->bSelected = true;
                break;
            }
        }
    }
}

void PushURLSegment(char** pszURL, char* szSubFolder)
{
    static unsigned int sAlloc = 0;
    if (!*pszURL)
    {
        sAlloc += 1024;
        *pszURL = (char*)malloc(sAlloc);
        *pszURL[0] = 0;
    }
    if (strlen(*pszURL) + strlen(szSubFolder) + 2 > sAlloc)
    {
        while (strlen(*pszURL) + strlen(szSubFolder) + 2 > sAlloc)
            sAlloc += 1024;
        char* temp = (char*)malloc(sAlloc);
        strcpy (temp,*pszURL);
        delete [] *pszURL;
        *pszURL = temp;
    }
    strcat (*pszURL,szSubFolder);
    strcat (*pszURL,"\\");

}

void
CBrowseFoldersMenuScreen::PopulateFolderContents()
{
    Minimize();
    m_cSubFolders = 0;
    // grab the folders first
    FolderEntry* pEntry = NULL;
    DSTAT statobj;

    char mask[PLAYLIST_STRING_SIZE];
    strcpy (mask,m_szCurrentPath);
    strcat (mask,"*.*");
    if (pc_gfirst(&statobj,mask))
    {
        do 
        {
            if( statobj.fname[0] == '.' )
                continue;
            if (statobj.fattribute & ADIRENT)
            {
                TrimInPlace(statobj.fext);
                TrimInPlace(statobj.fname);
                pEntry = new FolderEntry;
                pEntry->bSelected = false;
                pEntry->pPlaylistEntry = NULL;
                pEntry->szCaption = MakeLongFilename(statobj);
                //pEntry->szCaption = new char[strlen( statobj.fname ) + 1/*.*/ + strlen( statobj.fext ) + 1];
                strcpy(pEntry->szFilename,statobj.fname);
                if (strlen(statobj.fext))
                {
                    strcat(pEntry->szFilename,".");
                    strcat(pEntry->szFilename,statobj.fext);
                }
                m_vFolderEntries.PushBack(pEntry);
                ++m_cSubFolders;
            }
        } while (pc_gnext(&statobj));
        pc_gdone(&statobj);
    }

    // add an entry for "Play All"
    pEntry = new FolderEntry;
    pEntry->bSelected = false;
    pEntry->pPlaylistEntry = NULL;
    pEntry->szCaption = new char[tstrlen(LS(SID_PLAY_ALL))+1];
    TcharToChar(pEntry->szCaption,LS(SID_PLAY_ALL));
    m_vFolderEntries.PushBack(pEntry);

    // next grab the normal files
    if (pc_gfirst(&statobj,mask))
    {
        do 
        {
            if (!(statobj.fattribute & ADIRENT))
            {
                TrimInPlace(statobj.fext);
                TrimInPlace(statobj.fname);
                pEntry = new FolderEntry;
                pEntry->bSelected = false;
                pEntry->pPlaylistEntry = NULL;
                pEntry->szCaption = MakeLongFilename(statobj);
                //pEntry->szCaption = new char[strlen( statobj.fname ) + 1/*.*/ + strlen( statobj.fext ) + 1];
                strcpy(pEntry->szFilename,statobj.fname);
                if (strlen(statobj.fext))
                {
                    strcat(pEntry->szFilename,".");
                    strcat(pEntry->szFilename,statobj.fext);
                }
                m_vFolderEntries.PushBack(pEntry);
            }
        } while (pc_gnext(&statobj));
        pc_gdone(&statobj);
    }
    
    // see which files are already in the current playlist
    CheckPlaylistMembership();
    SetItemCount(m_vFolderEntries.Size());
}

// move the screen focus into a subfolder of the current folder.
bool
CBrowseFoldersMenuScreen::EnterFolder(int iFolderIndex)
{
    if (!PushPathSegment(m_szCurrentPath,m_vFolderEntries[iFolderIndex]->szFilename))
        return false;
    PushURLSegment(&m_szCurrentURL,m_vFolderEntries[iFolderIndex]->szCaption);
    // update any files added or removed from the current playlist.
    UpdatePlaylistMembership();
    ++m_nFolderDepth;
    PopulateFolderContents();
    return true;
}

// move the screen focus up into the immediate parent folder.
void
CBrowseFoldersMenuScreen::ReturnToParentFolder()
{
    if (m_nFolderDepth == 0)
        return;
    // update any files added or removed from the current playlist.
    UpdatePlaylistMembership();
    --m_nFolderDepth;
    PopPathSegment(m_szCurrentPath);
    PopPathSegment(m_szCurrentURL);
    // set m_iTopIndex to a suitable item so that the child folder is visible, and
    // set m_iLineIndex to the correct child folder that we just came out of.
    PopulateFolderContents();
}

bool CBrowseFoldersMenuScreen::IsPlayableExtension(char* szExtension)
{
    return (CCodecManager::GetInstance()->FindCodecID(szExtension) != 0);
}

void CBrowseFoldersMenuScreen::Init()
{
    PopulateFolderContents();
}
void CBrowseFoldersMenuScreen::Minimize()
{
    for (int d = 0; d < m_vFolderEntries.Size(); ++d)
    {
        free (m_vFolderEntries[d]->szCaption);
        delete (m_vFolderEntries[d]);
    }
    m_vFolderEntries.Clear();
}
