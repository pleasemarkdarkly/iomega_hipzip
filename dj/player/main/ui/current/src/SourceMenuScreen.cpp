//
// SourceMenuScreen.cpp: Browse available content and input sources via db queries
// danb@fullplaymedia.com 09/20/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/SourceMenuScreen.h>
#include <main/ui/PlayerScreen.h>

#include <main/ui/Bitmaps.h>
#include <main/ui/Keys.h>
#include <main/ui/Messages.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <core/events/SystemEvents.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <main/main/DJPlayerState.h>
#include <main/main/DJEvents.h>

#ifdef DDOMOD_DATASOURCE_FAT
#include <datasource/fatdatasource/FatDataSource.h>
#define USE_HD 1
#else
#define USE_HD 0
#endif

#ifdef DDOMOD_DATASOURCE_CD
#include <datasource/cddatasource/CDDataSource.h>
#define USE_CD 1
#else
#define USE_CD 0
#endif

#ifdef DDOMOD_DATASOURCE_LINEIN
#include <datasource/lineindatasource/LineInDataSource.h>
#define USE_LINEIN 1
#else
#define USE_LINEIN 0
#endif

#ifndef NO_UPNP
#include <main/iml/iml/IML.h>
#include <main/iml/manager/IMLManager.h>
#endif  // NO_UPNP

#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_SOURCE_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DBG_SOURCE_SCREEN );

CSourceMenuScreen* CSourceMenuScreen::s_pSourceMenuScreen = 0;

// This is a singleton class.
CScreen*
CSourceMenuScreen::GetSourceMenuScreen()
{
	if (!s_pSourceMenuScreen) {
		s_pSourceMenuScreen = new CSourceMenuScreen();
	}
	return s_pSourceMenuScreen;
}

CSourceMenuScreen::CSourceMenuScreen()
	: CDynamicMenuScreen(&(CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen), SID_MUSIC_SOURCES),
    m_iCurrentSourceIndex(0),
      m_pDJPlayerState(0),
      m_iScanningIMLs(0),
      m_bAllIMLsCached(false),
      m_bNetworkUp(false)
{
#ifndef NO_UPNP
    // this shouldn't fail, or we're all in BIG trouble.  :)
    m_pIMLManager = CIMLManager::GetInstance();
#endif // NO_UPNP
    m_pDJPlayerState = CDJPlayerState::GetInstance();

    m_pLMS = (CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen();
    m_pLEMS = (CLibraryEntryMenuScreen*)CLibraryEntryMenuScreen::GetLibraryEntryMenuScreen();
}

CSourceMenuScreen::~CSourceMenuScreen()
{
}

// Hides any visible menu screens.
void
CSourceMenuScreen::HideScreen()
{
    DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_TRACE, "sms:HideScreen\n");
    m_pLMS->SetBrowseSource(m_pDJPlayerState->GetSource());	
    m_pLMS->SetBrowseIML(m_pDJPlayerState->GetCurrentIML());
	CDynamicMenuScreen::HideScreen();
    RefreshSource();
}

SIGNED
CSourceMenuScreen::Message(const PegMessage &Mesg)
{
    CIMLManager* pIMLManager = CIMLManager::GetInstance();
    
	switch (Mesg.wType)
	{
    /*
	case PM_KEY:

		switch (Mesg.iData)
		{
        case IR_KEY_MENU:
		case KEY_MENU:
            GotoPreviousMenu();
            return 0;

        // todo: take this out at some point so we can browse a music source without having to select it.
        // override the base class and act like a select
        case IR_KEY_NEXT:
            ProcessMenuOption(GetHighlightedIndex());
            return 0;
		}
        break;
    */
    case IOPM_NETWORK_UP:
        m_bNetworkUp = true;
        Draw();
        break;
    
    case IOPM_NETWORK_DOWN:
        m_bNetworkUp = false;
        Draw();
        break;
            
    case IOPM_IML_FOUND:
        m_bNetworkUp     = true;
        m_iScanningIMLs  = pIMLManager->GetIMLCount() - pIMLManager->GetCachedIMLCount();
        m_bAllIMLsCached = (m_iScanningIMLs == 0);
        SetItemCount(m_pIMLManager->GetCachedIMLCount() + (USE_HD + USE_CD + USE_LINEIN) +
            (m_bAllIMLsCached ? 0 : 1));
        Draw();
        break;
        
    case IOPM_IML_REMOVED:
    {
        m_iScanningIMLs  = pIMLManager->GetIMLCount() - pIMLManager->GetCachedIMLCount();
        m_bAllIMLsCached = (pIMLManager->GetIMLCount() > 0) && (m_iScanningIMLs == 0);
        SetItemCount(m_pIMLManager->GetCachedIMLCount() + (USE_HD + USE_CD + USE_LINEIN) +
            (m_bAllIMLsCached ? 0 : 1));
        bool bRestoreFML = false;
        if (m_pLMS->GetBrowseIML() == (CIML*)Mesg.pData)
        {
            // Remember the fml's udn at the end of this call.
            // This is done because RefreshSource() calls SetSource(), and SetSource() clears the UDN.
            bRestoreFML = true;
            m_pLEMS->NotifyLostCurrentSource();
            m_pLMS->NotifyLostCurrentSource();
        }
        if (m_pDJPlayerState->GetCurrentIML() == (CIML*)Mesg.pData)
        {
            m_pDJPlayerState->SetCurrentIML(NULL);
            m_iCurrentSourceIndex = -1; // no source should be selected b/c we lost the current FML
            RefreshSource();
            ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchStatusMessage();
        }
        else
        {
            while(GetHighlightedIndex() >= m_cItems)
                --m_iTopIndex;
        }
        Draw();

        if (bRestoreFML)
        {
            m_pDJPlayerState->SetIMLToRestore(((CIML*)Mesg.pData)->GetUDN());
            DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_INFO, "sms:Lost current fml source, UDN: %s\n", m_pDJPlayerState->GetIMLToRestore());
        }

        break;
    }

    case IOPM_IML_AVAILABLE:
        {
            m_bNetworkUp     = true;
            m_iScanningIMLs  = pIMLManager->GetIMLCount() - pIMLManager->GetCachedIMLCount();
            m_bAllIMLsCached = (pIMLManager->GetIMLCount() > 0) && (m_iScanningIMLs == 0);
            bool bLastIndex  = GetHighlightedIndex() == (m_cItems - 1);
            SetItemCount(m_pIMLManager->GetCachedIMLCount() + (USE_HD + USE_CD + USE_LINEIN) +
                (m_bAllIMLsCached ? 0 : 1));
            if (bLastIndex)
                SetHighlightedIndex(m_cItems - 1);

            // If this fml was the previous current source and the user hasn't changed sources
            // then restore it as the current source.
            if (m_pDJPlayerState->GetIMLToRestore() &&
                !strcmp(m_pDJPlayerState->GetIMLToRestore(), ((CIML*)Mesg.pData)->GetUDN()))
            {
                DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_INFO, "sms:Restoring current source, UDN: %s\n", m_pDJPlayerState->GetIMLToRestore());
                CDJPlayerState::GetInstance()->SetSource(CDJPlayerState::FML, true, false);
                m_pDJPlayerState->SetCurrentIML((CIML*)Mesg.pData);
                RefreshSource();
                ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->SynchStatusMessage();
            }

            Draw();
            break;
        }
	}
	return CDynamicMenuScreen::Message(Mesg);
}

void
CSourceMenuScreen::RefreshSource(bool bSynchToBrowseSource)
{
    DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_TRACE, "sms:RefreshSource\n");
    SetItemCount(m_pIMLManager->GetCachedIMLCount() + (USE_HD + USE_CD + USE_LINEIN) +
        (m_bAllIMLsCached ? 0 : 1));

    CDJPlayerState::ESourceMode eSource;
    CIML* pIML;
    if (bSynchToBrowseSource)
    {
        eSource = m_pLMS->GetBrowseSource();
        pIML = m_pLMS->GetBrowseIML();
    }
    else
    {
        eSource = m_pDJPlayerState->GetSource();
        pIML = m_pDJPlayerState->GetCurrentIML();
    }

    // make sure we're synched with what we're browsing
    switch(eSource)
    {
        case CDJPlayerState::HD:
            m_iTopIndex = -1;
            break;
        case CDJPlayerState::CD:
            m_iTopIndex = -1 + USE_HD;
            break;
        case CDJPlayerState::LINE_IN:
            m_iTopIndex = -1 + USE_HD + USE_LINEIN;
            break;
        case CDJPlayerState::FML:
            // we need to make sure what we think the current fml is is actually at
            // the index we think it's at.
            if(pIML)
            {
                // point at the correct fml
                for(int i = 0; i < m_pIMLManager->GetCachedIMLCount(); i++)
                {
                    if(pIML == m_pIMLManager->GetCachedIMLByIndex(i))
                    {
                        m_iTopIndex = i + (USE_HD + USE_CD + USE_LINEIN) - 1;
                    }
                }
            }
            break;
    }

    // make sure we've got the right source checked as current
    switch(m_pDJPlayerState->GetSource())
    {
        case CDJPlayerState::HD:
            m_iCurrentSourceIndex = 0;
            break;
        case CDJPlayerState::CD:
            m_iCurrentSourceIndex = 0 + USE_HD;
            break;
        case CDJPlayerState::LINE_IN:
            m_iCurrentSourceIndex = 0 + (USE_HD + USE_CD);
            break;
        case CDJPlayerState::FML:
            // we need to make sure what we think the current fml is is actually at
            // the index we think it's at.
            if(m_pDJPlayerState->GetCurrentIML())
            {
                // point at the correct fml
                for(int i = 0; i < m_pIMLManager->GetCachedIMLCount(); i++)
                {
                    if(m_pDJPlayerState->GetCurrentIML() == m_pIMLManager->GetCachedIMLByIndex(i))
                    {
                        m_iCurrentSourceIndex = i + (USE_HD + USE_CD + USE_LINEIN);
                        return;
                    }
                }
            }
            // had a problem finding the index of the current iml
            // revert to HD/CD/LINEIN
            DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_ERROR, "Lost current FML.  Reverting to HD Source Mode\n");
            if (m_pDJPlayerState->SetSource(CDJPlayerState::HD, false))
            {
                CPlayerScreen* pPS = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
                pPS->ClearTrack();
                pPS->SetEventHandlerMode(ePSPlaybackEvents);
            }
            m_pLMS->SetBrowseSource(CDJPlayerState::HD);
            m_iCurrentSourceIndex = 0;
            m_iTopIndex = -1;
            break;
    }
}

const TCHAR* 
CSourceMenuScreen::MenuItemCaption(int iMenuIndex)
{
    if ( iMenuIndex == 0 )
        return LS(SID_HARD_DISK);
    else if ( iMenuIndex == 0 + USE_HD )
        return LS(SID_CD_DRIVE);
#if USE_LINEIN == 1
    else if ( iMenuIndex == 0 + (USE_HD + USE_CD) )
        return LS(SID_LINE_INPUT);
#endif
#ifndef NO_UPNP
    else 
    {
        // get the names of the imls currently available
        CIML* pIML = m_pIMLManager->GetCachedIMLByIndex(iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
        if(pIML)
            return pIML->GetFriendlyName();
        else
            return BuildScanningString();
    }
#else  // NO_UPNP
    else
        return LS(SID_EMPTY_STRING);
#endif  // NO_UPNP
}

bool
CSourceMenuScreen::MenuItemSelected(int iMenuIndex)
{
    return (iMenuIndex == m_iCurrentSourceIndex);
}


// Called when the user hits the next button.
// Acts based upon the currently highlighted menu item.
void
CSourceMenuScreen::GotoSubMenu(int iMenuIndex)
{
    CDynamicMenuScreen::GotoSubMenu(iMenuIndex);

    CPlayerScreen* pPS = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();

    // switch the input sources
    if (iMenuIndex == 0) // HD
        m_pLMS->SetBrowseSource(CDJPlayerState::HD);
    else if (iMenuIndex == 0 + USE_HD) // CD
    {
        // Only browse if we have a fully scanned data CD.
        // Otherwise go to the player screen.
        if (m_pDJPlayerState->GetCDState() == CDJPlayerState::AUDIO)
        {
            pPS->SetEventHandlerMode(ePSPlaybackEvents);
            if (m_pDJPlayerState->SetSource(CDJPlayerState::CD))
            {
//                pPS->ClearTrack();
                // Start playback on select.
                if (!CPlayManager::GetInstance()->GetPlaylist()->IsEmpty())
                    CPlayManager::GetInstance()->Play();
            }
            //pPS->SetMessageText(LS(SID_CANT_BROWSE_AUDIO_CD));
            pPS->HideMenus();
            return;
        }
        else if (m_pDJPlayerState->GetCDState() == CDJPlayerState::NONE)
        {
            if (m_pDJPlayerState->GetCDDataSource()->IsTrayOpen())
            {
                // fake an EJECT button press to close the tray
                // (danb) i did it this way b/c there are a lot of state variables
                // that need to be used/set/checked when ejecting and inserting a cd
                CEventQueue::GetInstance()->PutEvent( EVENT_KEY_PRESS, (void*)KEY_CD_EJECT );
            }
            else if (m_pDJPlayerState->GetSource() == CDJPlayerState::CD)
                pPS->DisplayInsertCDScreen();
            else
                pPS->SetMessageText(LS(SID_PLEASE_INSERT_CD));
            pPS->HideMenus();
            return;
        }
        if ((m_pDJPlayerState->GetCDState() == CDJPlayerState::DATA) &&
            m_pDJPlayerState->IsScanningCD())
        {
            pPS->SetMessageText(LS(SID_CANT_BROWSE_WHILE_SCANNING_DATA_CD));
            pPS->HideMenus();
            return;
        }

        m_pLMS->SetBrowseSource(CDJPlayerState::CD);
    }
#if USE_LINEIN == 1
    else if (iMenuIndex == 0 + USE_HD + USE_CD)
    {
        if (m_pDJPlayerState->GetSource() != CDJPlayerState::LINE_IN)
        {
            if (m_pDJPlayerState->SetSource(CDJPlayerState::LINE_IN))
            {
                pPS->ClearTrack();
            }
            pPS->SetEventHandlerMode(ePSLineInEvents);
            m_iCurrentSourceIndex = iMenuIndex;
        }
        pPS->HideMenus();
        return;
    }
#endif
#ifndef NO_UPNP
    else
    {
        // This check for availability is redundant with that in ProcessMenuOption, but next and select
        // may be changed back to do seperate things, so I will leave this check in here so a bug doesn't creep
        // in later.
        CIML* pIML = m_pIMLManager->GetCachedIMLByIndex(iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
        if (pIML && m_pIMLManager->IsAvailable(pIML))
        {
            m_pLMS->SetBrowseIML(pIML);
            if (!m_pLMS->GetBrowseIML())
            {
                DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_ERROR, "Couldn't Find IML Library: %d\n\n", iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
                // maybe put up an alert screen?
                return;
            }
            else
            {
                m_pLMS->SetBrowseSource(CDJPlayerState::FML);
            }
        }
        else
        {
            // IML unavailable
            DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_ERROR, "IML Library %d Unavailable\n\n", iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
            m_pIMLManager->SetIMLUnavailable(pIML);
            return;
        }
    }
#endif  // NO_UPNP

    m_pLEMS->SetBrowseMode(CLibraryMenuScreen::ALBUM);
    pPS->Add(m_pLEMS);
    Presentation()->MoveFocusTree(m_pLEMS);
}

// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CSourceMenuScreen::ProcessMenuOption(int iMenuIndex)
{

// take out the parts that make the source switch and move that to the library menu screen for when you 
// do an <add> or <select> operation
#if 0

    CPlayerScreen* pPS = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();

    // switch the input sources
    if (iMenuIndex == 0)
    {
        if (m_pDJPlayerState->SetSource(CDJPlayerState::HD))
        {
            pPS->ClearTrack();
        }
        pPS->SetEventHandlerMode(ePSPlaybackEvents);
        m_iCurrentSourceIndex = iMenuIndex;
    }
    else if (iMenuIndex == 0 + USE_HD)
    {
        // dc- set the event handler mode prior to calling djps->setsource, since setting the CD
        //     source creates a new playlist, and setting the event handler mode (for some reason)
        //     clears the playlist.
        pPS->SetEventHandlerMode(ePSPlaybackEvents);
        if (m_pDJPlayerState->SetSource(CDJPlayerState::CD))
        {
            pPS->ClearTrack();
        }
        m_iCurrentSourceIndex = iMenuIndex;
        // Only browse if we have a fully scanned data CD.
        // Otherwise go to the player screen.
        if (m_pDJPlayerState->GetCDState() == CDJPlayerState::AUDIO)
        {
            pPS->SetMessageText(LS(SID_CANT_BROWSE_AUDIO_CD));
            pPS->HideMenus();
            return;
        }
        else if (m_pDJPlayerState->GetCDState() == CDJPlayerState::NONE)
        {
            pPS->DisplayInsertCDScreen();
            pPS->HideMenus();
            return;
        }
        if ((m_pDJPlayerState->GetCDState() == CDJPlayerState::DATA) &&
            m_pDJPlayerState->IsScanningCD())
        {
            pPS->SetMessageText(LS(SID_CANT_BROWSE_WHILE_SCANNING_DATA_CD));
            pPS->HideMenus();
            return;
        }
    }
#if USE_LINEIN == 1
    else if (iMenuIndex == 0 + USE_HD + USE_CD)
    {
        if (m_pDJPlayerState->GetSource() != CDJPlayerState::LINE_IN)
        {
            if (m_pDJPlayerState->SetSource(CDJPlayerState::LINE_IN))
            {
                pPS->ClearTrack();
            }
            pPS->SetEventHandlerMode(ePSLineInEvents);
            m_iCurrentSourceIndex = iMenuIndex;
        }
        pPS->HideMenus();
        return;
    }
#endif
#ifndef NO_UPNP
    else
    {
        CIML* pIML = m_pIMLManager->GetCachedIMLByIndex(iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
        if (pIML && m_pIMLManager->IsAvailable(pIML))
        {
            m_pDJPlayerState->SetCurrentIML(pIML);
            if (!m_pDJPlayerState->GetCurrentIML())
            {
                if (m_pDJPlayerState->SetSource(CDJPlayerState::HD))
                {
                    pPS->ClearTrack();
                }
                pPS->SetEventHandlerMode(ePSPlaybackEvents);
                m_iCurrentSourceIndex = 0;
                DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_ERROR, "Couldn't Connect to IML: %d\nReverting to HD Source Mode\n", iMenuIndex - (USE_HD + USE_CD + USE_LINEIN));
            }
            else
            {
                if (m_pDJPlayerState->SetSource(CDJPlayerState::FML))
                {
                    pPS->ClearTrack();
                }
                pPS->SetEventHandlerMode(ePSPlaybackEvents);
                m_iCurrentSourceIndex = iMenuIndex;
                char imlString[128];
                DEBUGP( DBG_SOURCE_SCREEN, DBGLEV_INFO, "Using IML: %s\n", TcharToCharN(imlString, m_pDJPlayerState->GetCurrentIML()->GetFriendlyName(), 128));
            }
        }
        else
        {
            m_pIMLManager->SetIMLUnavailable(pIML);
            // Return now, don't try to go to the SubMenu if the IML is gone.
            return;
        }
    }
#endif  // NO_UPNP

#endif // if 0

    GotoSubMenu(iMenuIndex);
}

// Called when the user hits the previous button.
// Acts based upon the currently highlighted menu item.
void
CSourceMenuScreen::GotoPreviousMenu()
{
    m_pLMS->SetBrowseSource(m_pDJPlayerState->GetSource());	
    m_pLMS->SetBrowseIML(m_pDJPlayerState->GetCurrentIML());
	CDynamicMenuScreen::GotoPreviousMenu();
    RefreshSource();
}

const TCHAR*
CSourceMenuScreen::BuildScanningString()
{
    CIMLManager* pIMLManager = CIMLManager::GetInstance();
    CNetDataSource* pNetDS = CDJPlayerState::GetInstance()->GetNetDataSource();

    if (!pNetDS->IsInitialized())
    {
        return LS(SID_NETWORK_DISCONNECTED);
    }
    else if (pIMLManager->GetIMLCount() == 0)
    {
        return LS(SID_NO_FULLPLAY_MEDIA_LIBRARIES);
    }
    else
    {
        static TCHAR tcCaption[128]; // This will be returned, so must make it static, not auto
        tcCaption[0] = 0;
        tstrcat(tcCaption, LS(SID_SCANNING));
        
        char cCount[32];
        sprintf(cCount, " %d ", m_iScanningIMLs);
        TCHAR tcCount[32];
        CharToTcharN(tcCount, cCount, 128);
        tstrcat(tcCaption, tcCount);
        
        tstrcat(tcCaption, m_iScanningIMLs > 1 ? LS(SID_MEDIA_LIBRARIES) : LS(SID_MEDIA_LIBRARY));
        
        return tcCaption;
    }
}

    
    
