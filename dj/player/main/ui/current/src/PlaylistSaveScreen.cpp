//
// PlaylistSaveScreen.cpp: contains the implementation of the CPlaylistSaveScreen class
// temancl@fullplaymedia.com 12/18/01
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/MainMenuScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/PlaylistSaveScreen.h>
#include <main/ui/PlaylistConstraint.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/EditScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/YesNoScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Fonts.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <main/main/AppSettings.h>
#include <main/main/DJHelper.h>
#include <main/main/DJPlayerState.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <core/playmanager/PlayManager.h>
#include <main/main/FatHelper.h>

#include <main/content/djcontentmanager/DJContentManager.h>

#include <stdio.h>              /* sprintf */

extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

//#include <util/debug/debug.h>          // debugging hooks
//DEBUG_MODULE_S( DBG_QUICKBROWSE, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
//DEBUG_USE_MODULE( DBG_QUICKBROWSE );

CPlaylistSaveScreen* CPlaylistSaveScreen::s_pPlaylistSaveScreen = 0;


// This is a singleton class.
CPlaylistSaveScreen*
CPlaylistSaveScreen::GetPlaylistSaveScreen()
{
    if (!s_pPlaylistSaveScreen) {
        s_pPlaylistSaveScreen = new CPlaylistSaveScreen();
    }
    return s_pPlaylistSaveScreen;
}

CPlaylistSaveScreen::CPlaylistSaveScreen()
    : CDynamicMenuScreen(NULL, SID_SAVE_CURRENT_PLAYLIST),
      m_iPlaylistCount(0),
      m_iPlaylistEntryIndex(0),
      m_iPlaylistTopIndex(0),
      m_iPlaylistLineIndex(0),
      m_bSavingPlaylist(false)
{
    SetItemCount(0);
}

CPlaylistSaveScreen::~CPlaylistSaveScreen()
{
}

SIGNED
CPlaylistSaveScreen::Message(const PegMessage &Mesg)
{
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        case KEY_SELECT:
        case IR_KEY_SELECT:
        case IR_KEY_SAVE:
            if (!m_bSavingPlaylist)
                ProcessMenuOption(GetHighlightedIndex());
            return 0;

        // drop these keys
        case IR_KEY_GENRE:
        case IR_KEY_ARTIST:
        case IR_KEY_ALBUM:
        case IR_KEY_PLAYLIST:
        case IR_KEY_RADIO:
            return 0;
            
        default:
            break;
        }
        break;
        
    default:
        break;
    }
    
    return CDynamicMenuScreen::Message(Mesg);
}

const TCHAR* 
CPlaylistSaveScreen::MenuItemCaption(int iMenuIndex)
{
    //DEBUGP( DBG_QUICKBROWSE, DBGLEV_TRACE, "CPlaylistSaveScreen::MenuItemCaption(%d)\n", iMenuIndex);
    
    if(iMenuIndex == 0)
        return LS(SID_SAVE_CURRENT_PLAYLIST_AS_NEW);
    else
    {
        int i = 0;
        for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead(); itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
        {
            if((iMenuIndex - 1) == i)
            {
                const char* temp = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                
                // strip extension
                static TCHAR tcTemp[128];
                int len = strlen(temp);
                len = (len < 127) ? len - 4 : 127;
                return CharToTcharN(tcTemp, temp, len);
            }
            i++;
        }
    }
    
    // just in case
    return LS(SID_EMPTY_STRING);
}

// called to make this screen do the necessary queries to get ready to be shown
// 0 is create new playlist
// 1 is playlist[0], etc

void
CPlaylistSaveScreen::SetupList(int iPlaylist)
{
    m_prlPlaylistMenuItems.Clear();
    CPlayManager::GetInstance()->GetContentManager()->GetPlaylistRecordsByDataSourceID(m_prlPlaylistMenuItems, CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID());
    SetItemCount(m_prlPlaylistMenuItems.Size() + 1);  // 1 for "Create new"
    
    if(iPlaylist > 0)
        m_iTopIndex = iPlaylist - 1;
    
    m_iPlaylistLineIndex = iPlaylist;
    
    DrawMenu();
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CPlaylistSaveScreen::ProcessMenuOption(int iMenuIndex)
{
    if(iMenuIndex == 0)
        CreateNewPlaylist();
    else
        OverwritePlaylist(iMenuIndex);
}

// Create a new playlist
void
CPlaylistSaveScreen::CreateNewPlaylist()
{
    // the playlist ID to start at
    unsigned int uiID = 1;
    
    // do we have a playlist to save?
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (!pCurrentPlaylist)
        return;

    TCHAR tcname[EMAXPATH];
    char name[EMAXPATH];

    CDJContentManager* pCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();
    int iFatDSID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    // make a URL
    CDJPlayerState::GetInstance()->GetFatDataSource()->GetContentRootURLPrefix(name, EMAXPATH);
    strcat(name, "/");
    char* szEnd = name + strlen(name);

    do
    {
        // create a unique URL
        sprintf(szEnd, "Playlist %d.djp", uiID);

        // check with the content manager to make sure a playlist with a matching URL doesn't exist
        if (pCM->GetPlaylistRecord(name, iFatDSID))
            ++uiID;
        else
            break;

    } while (1);

    // create a unique name
    sprintf(name,"Playlist %d",uiID);
    
    CharToTchar(tcname, name);
        
    // put it in the edit screen to see if the user wants to edit it.
    CEditScreen::GetInstance()->Config(this, EditNewPlaylistNameCallback, tcname);
    Add(CEditScreen::GetInstance());
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CPlaylistSaveScreen::OverwritePlaylist(int iMenuIndex)
{
    // do we have a playlist to save?
    IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (!pCurrentPlaylist)
        return;
    
    // put up an overlay
    m_bSavingPlaylist = true;
    CAlertScreen::GetInstance()->Config(this, 100, FinishedSavingPlaylistCallback);
    CAlertScreen::GetInstance()->SetTitleText(LS(SID_SAVE_PLAYLIST));
    CAlertScreen::GetInstance()->SetActionText(LS(SID_SAVING_PLAYLIST));
    CAlertScreen::GetInstance()->SetMessageText(LS(SID_PLEASE_WAIT));
    CAlertScreen::GetInstance()->ResetProgressBar(0, 3);
    Add(CAlertScreen::GetInstance());

    const int iFormat = CPlaylistFormatManager::GetInstance()->FindPlaylistFormat("djp");
    
    char* szExisting = 0;
    // get (iMenuIndex - 1) URL
    int i = 0;
    for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
    itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
    {
        if((iMenuIndex - 1) == i)
        {
            
            szExisting = (char *)(*itPlaylist)->GetURL();				
            break;
        }
        
        i++;
    }
    CAlertScreen::GetInstance()->UpdateProgressBar(1);

    char filename[128];
    strncpy(filename, FilenameFromURLInPlace(szExisting), 127);
    // strip extension
    int len = strlen(filename);
    len = (len < 127) ? len - 4 : 127;
    filename[len] = 0;

    // construct our system text message
    char szName[128];
    TCHAR tszName[128];
    TCHAR tszMessage[256];
    tstrcpy(tszMessage, LS(SID_SAVED_CURRENT_PLAYLIST_AS));
    sprintf(szName, " %s ", filename);
    CharToTcharN(tszName, szName, 255);
    tstrcat(tszMessage, tszName);
    tstrcat(tszMessage, LS(SID_TO_HD));
    ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetMessageText(tszMessage, CSystemMessageString::REALTIME_INFO);

    // tell the quickbrowse screen the name of the playlist
    CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(filename);

    // Adjust thread priority so playback doesn't stutter.
    int nPrio = GetMainThreadPriority();
    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

    // call save playlist
    CPlaylistFormatManager::GetInstance()->SavePlaylist(iFormat,szExisting,pCurrentPlaylist);
    CAlertScreen::GetInstance()->UpdateProgressBar(2);
    
    // save the content manager database if we're not playing back
    CommitUpdatesIfSafe();

    SetMainThreadPriority(nPrio);

    CAlertScreen::GetInstance()->UpdateProgressBar(3);
}


void
CPlaylistSaveScreen::FinishedSavingPlaylistCB()
{
    // exit to the previous screen
    GotoPreviousMenu();
    m_bSavingPlaylist = false;
}

void
CPlaylistSaveScreen::FinishedSavingPlaylistCallback()
{
    if( s_pPlaylistSaveScreen )
        s_pPlaylistSaveScreen->FinishedSavingPlaylistCB();    
}

void
CPlaylistSaveScreen::EditNewPlaylistNameCB(bool bSave, bool bOverwrite)
{
    if (bSave)
    {
        // do we still have a playlist to save?
        IPlaylist* pCurrentPlaylist = CPlayManager::GetInstance()->GetPlaylist();
        if (!pCurrentPlaylist)
            return;

        // check name
        const TCHAR ForwardSlash[]  = {'/',0};
        const TCHAR BackwardSlash[] = {'\\',0};
        
        bool InvalidName  = false;
        InvalidName       = tstrcmp(LS(SID_PLAY_EVERYTHING), CEditScreen::GetInstance()->GetDataString()) == 0;
        InvalidName      |= tstrstr(CEditScreen::GetInstance()->GetDataString(), ForwardSlash) != 0;
        InvalidName      |= tstrstr(CEditScreen::GetInstance()->GetDataString(), BackwardSlash) != 0;
        
        if (InvalidName)
        {
            CAlertScreen::GetInstance()->Config(this, 100);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_BAD_PLAYLIST_NAME));
            Add(CAlertScreen::GetInstance());
            return;
        }

        

        // check name against overwriting
        if (!bOverwrite)
        {
            // convert new playlist name to char for comparison purposes
            int len               = tstrlen(CEditScreen::GetInstance()->GetDataString());
            char* NewPlaylistName = new char[len + 1];
            TcharToCharN(NewPlaylistName, CEditScreen::GetInstance()->GetDataString(), len);

            for (PlaylistRecordIterator itPlaylist = m_prlPlaylistMenuItems.GetHead();
                 itPlaylist != m_prlPlaylistMenuItems.GetEnd(); ++itPlaylist)
            {
                const char* Filename = FilenameFromURLInPlace((*itPlaylist)->GetURL());
                
                // strip extension
                int   len      = strlen(Filename);
                char* ListName = new char[len + 1];
                strcpy(ListName, Filename);
                ListName[len - 4] = 0;
                
                if (strcmp(NewPlaylistName, ListName) == 0)
                {
                    // Ask if user wants to overwrite name
                    CEditScreen::GetInstance()->SetHideScreen(false);
                    CYesNoScreen::GetInstance()->Config(CEditScreen::GetInstance(), OverwritePlaylistNameCallback);
                    CYesNoScreen::GetInstance()->SetTitleText(LS(SID_SAVE_PLAYLIST));
                    CYesNoScreen::GetInstance()->SetActionText(LS(SID_OVERWRITE_PLAYLIST));
                    Add(CYesNoScreen::GetInstance());

                    delete [] ListName;
                    delete [] NewPlaylistName;
                    return;
                }
                
                delete [] ListName;
            }
            
            delete [] NewPlaylistName;
        }
        
        // put up an overlay
        m_bSavingPlaylist = true;
        CAlertScreen::GetInstance()->Config(this, 100, FinishedSavingPlaylistCallback);
        CAlertScreen::GetInstance()->SetActionText(LS(SID_CREATING_PLAYLIST));
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_PLEASE_WAIT));
        CAlertScreen::GetInstance()->ResetProgressBar(0, 3);
        Add(CAlertScreen::GetInstance());

        const int iFormat = CPlaylistFormatManager::GetInstance()->FindPlaylistFormat("djp");
        char szUrl[EMAXPATH];
        char name[EMAXPATH];
    
        // make a URL
        CDJPlayerState::GetInstance()->GetFatDataSource()->GetContentRootURLPrefix(szUrl, EMAXPATH);
            
        // send the new name to the quickbrowse screen
        CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(CEditScreen::GetInstance()->GetDataString());

        TcharToCharN(name, CEditScreen::GetInstance()->GetDataString(),EMAXPATH-1);
        strcat(name, ".DJP");
        strcat(szUrl, "/");
        strcat(szUrl, name);

        char filename[128];
        strncpy(filename, FilenameFromURLInPlace(szUrl), 127);
        // strip extension
        int len = strlen(filename);
        len = (len < 127) ? len - 4 : 127;
        filename[len] = 0;
    
        // construct our system text message
        char szName[128];
        TCHAR tszName[128];
        TCHAR tszMessage[256];
        tstrcpy(tszMessage, LS(SID_SAVED_CURRENT_PLAYLIST_AS));
        sprintf(szName, " %s ", filename);
        CharToTcharN(tszName, szName, 255);
        tstrcat(tszMessage, tszName);
        tstrcat(tszMessage, LS(SID_TO_HD));
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SetMessageText(tszMessage, CSystemMessageString::REALTIME_INFO);

        // Adjust thread priority so playback doesn't stutter.
        int nPrio = GetMainThreadPriority();
        SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);

        // save to disk
        CPlaylistFormatManager::GetInstance()->SavePlaylist(iFormat,szUrl,pCurrentPlaylist);
        CAlertScreen::GetInstance()->UpdateProgressBar(1);
    
        // if that worked, add playlist to content manager
        playlist_record_t prNew;
        prNew.bVerified         = true;
        prNew.iDataSourceID     = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();
        prNew.iPlaylistFormatID = iFormat;
        prNew.szURL             = szUrl;		
        
        // add to playlist content manager
        CPlayManager::GetInstance()->GetContentManager()->AddPlaylistRecord(prNew);
        CAlertScreen::GetInstance()->UpdateProgressBar(2);
 
        // save the content manager database if we're not playing back
        CommitUpdatesIfSafe();

        SetMainThreadPriority(nPrio);

        CLibraryMenuScreen* pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
        pLMS->ResynchWithChanges();
        
        CAlertScreen::GetInstance()->UpdateProgressBar(3);
    }
}

void
CPlaylistSaveScreen::EditNewPlaylistNameCallback(bool bSave)
{
    if( s_pPlaylistSaveScreen )
        s_pPlaylistSaveScreen->EditNewPlaylistNameCB(bSave);    
}

void
CPlaylistSaveScreen::OverwritePlaylistNameCallback(bool bOverwrite)
{
    if (s_pPlaylistSaveScreen && bOverwrite) {
        s_pPlaylistSaveScreen->EditNewPlaylistNameCB(true, bOverwrite);
        CEditScreen::GetInstance()->HideScreen();
    }
}
