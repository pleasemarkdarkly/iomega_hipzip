//
// SystemToolsMenuScreen.cpp: implementation of CSystemToolsMenuScreen class
// danb@fullplaymedia.com 09/26/2001
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <main/ui/SystemToolsMenuScreen.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/LibraryMenuScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Strings.hpp>
#include <main/ui/YesNoScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/Messages.h>

#include <main/main/AppSettings.h>
#include <main/main/DJPlayerState.h>
#include <main/main/FatHelper.h>
#include <main/main/ProgressWatcher.h>
#include <main/main/Recording.h>
#include <main/main/UpdateManager.h>
#include <_version.h>

#include <main/cddb/CDDBHelper.h>
#include <extras/cddb/gn_errors.h>

#include <core/playmanager/PlayManager.h>
#include <main/content/djcontentmanager/DJContentManager.h>

#include <util/debug/debug.h>
#include <util/upnp/genlib/mystring.h>

#include <devs/audio/dai.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/netdatasource/NetDataSource.h>

#include <cyg/compress/zlib.h>
#include <cyg/infra/diag.h>

#include <string.h>

#include <util/debug/debug.h>          // debugging hooks
DEBUG_MODULE_S( DBG_SOURCE_MENU_SCREEN, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( DBG_SOURCE_MENU_SCREEN );

// Comment out the following to remove support for tftp based HD updates
#define SUPPORT_LOCAL_UPDATES

// the global reference for this class
CSystemToolsMenuScreen* CSystemToolsMenuScreen::s_pSystemToolsMenuScreen = 0;

static MenuItemRec s_menuItems[] =
{
//  { pSelected, pSelectable, bHasSubmenu, wCaptionSID, pScreen }
#ifdef SUPPORT_LOCAL_UPDATES
    { false, false, false, SID_UPDATE_FIRMWARE_LOCALLY, NULL },
#endif
    { false, false, false, SID_UPDATE_FIRMWARE, NULL },
    { false, false, false, SID_UPDATE_FIRMWARE_ONLINE, NULL },
    { false, false, false, SID_UPDATE_CDDB, NULL },
    { false, false, false, SID_UPDATE_CDDB_ONLINE, NULL },
    { false, false, false, SID_SCAN_HD_FOR_MUSIC, NULL },
//  { false, false, false, SID_SCAN_HD_FOR_ERRORS, NULL },
//  { false, false, false, SID_DEFRAGMENT_HD, NULL },
};


// This is a singleton class.
CScreen*
CSystemToolsMenuScreen::GetSystemToolsMenuScreen()
{
	if (!s_pSystemToolsMenuScreen) {
		s_pSystemToolsMenuScreen = new CSystemToolsMenuScreen(NULL);
	}
	return s_pSystemToolsMenuScreen;
}


CSystemToolsMenuScreen::CSystemToolsMenuScreen(CScreen* pParent)
	: CMenuScreen(pParent, SID_SYSTEM_TOOLS, s_menuItems, sizeof(s_menuItems) / sizeof(MenuItemRec))
{
    // Configure the update manager to love us
    CUpdateManager::GetInstance()->Configure( FileProgressCallback, FlashProgressCallback );
    m_bUpdatingCDDB = false;
    m_bDoingOnlineUpdate = false;
    m_bHideMenuWhenScanDone = false;
}

CSystemToolsMenuScreen::~CSystemToolsMenuScreen()
{
}

SIGNED
CSystemToolsMenuScreen::Message(const PegMessage &Mesg)
{
    char szNumber[32];
    TCHAR tszNumber[32];
    TCHAR tszMessage[128];

	switch (Mesg.wType)
	{
    case IOPM_CONTENT_UPDATE:
        if (Mesg.iData == CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID())
        {
            DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "stms: IOPM_CONTENT_UPDATE [%d]\n", Mesg.iUserData[1]);
            tstrcpy(tszMessage, LS(SID_TRACKS_FOUND_COLON));
            sprintf(szNumber, " %d", Mesg.iUserData[1]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            CAlertScreen::GetInstance()->SetMessageText(tszMessage);
        }
        break;

    case IOPM_CONTENT_UPDATE_END:
        if (Mesg.iData == CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID())
        {
            DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "stms: IOPM_CONTENT_UPDATE [%d]\n", Mesg.iUserData[0]);
            tstrcpy(tszMessage, LS(SID_COMPLETED));
            sprintf(szNumber, " 0 ");
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_OF));
            sprintf(szNumber, " %d", Mesg.iUserData[0]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            CAlertScreen::GetInstance()->SetMessageText(tszMessage);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_SCANNING_FOR_TRACK_INFO));
        }
        break;

    case IOPM_METADATA_UPDATE:
        if (Mesg.iData == CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID())
        {
            DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "stms: IOPM_METADATA_UPDATE [%d] of [%d]\n", Mesg.iUserData[0], Mesg.iUserData[1]);
            tstrcpy(tszMessage, LS(SID_COMPLETED));
            sprintf(szNumber, " %d ", Mesg.iUserData[0]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_OF));
            sprintf(szNumber, " %d", Mesg.iUserData[1]);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            CAlertScreen::GetInstance()->SetMessageText(tszMessage);
            CAlertScreen::GetInstance()->SetActionText(LS(SID_SCANNING_FOR_TRACK_INFO));
            CAlertScreen::GetInstance()->ResetProgressBar(Mesg.iUserData[0], Mesg.iUserData[1]);
        }
        break;

    case IOPM_METADATA_UPDATE_END:
        if (Mesg.iData == CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID())
        {
            DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "stms: IOPM_METADATA_UPDATE_END\n");
            if (Presentation()->GetCurrentThing() == CAlertScreen::GetInstance())
            {
                // Commit the database.
                ((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();

                if (!m_bHideMenuWhenScanDone)
                {
                    CAlertScreen::GetInstance()->HideScreen();

                    CAlertScreen::GetInstance()->Config(this, 400, HDScanDoneCallback);
                    CAlertScreen::GetInstance()->SetTitleText(LS(SID_SCAN_HD_FOR_MUSIC));
                    CAlertScreen::GetInstance()->SetMessageText(LS(SID_RETURNING_TO_MENU));
                    if (Mesg.iUserData[0] > 0)
                    {
                        tstrcpy(tszMessage, LS(SID_SCANNED));
                        sprintf(szNumber, " %d ", Mesg.iUserData[0]);
                        CharToTcharN(tszNumber, szNumber, 31);
                        tstrcat(tszMessage, tszNumber);
                        tstrcat(tszMessage, LS(SID_TRACKS));
                        CAlertScreen::GetInstance()->SetActionText(tszMessage);
                    }
                    else
                        CAlertScreen::GetInstance()->SetActionText(LS(SID_NO_MUSIC_FOUND_ON_HD));
                    Add(CAlertScreen::GetInstance());
                }
            }
            if (m_bHideMenuWhenScanDone)
            {
                CPlayerScreen::GetPlayerScreen()->HideMenus();
            }
            m_bHideMenuWhenScanDone = false;
        }
        break;

    case IOPM_CDDB_UPDATE_DOWNLOADING:
    {
        m_bUpdatedCDDB = true;
        tstrcpy(tszMessage, LS(SID_DOWNLOADING_CDDB_FILE));
        sprintf(szNumber, " %d ", Mesg.iData);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_OF));
        sprintf(szNumber, " %ld", Mesg.lData);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        CAlertScreen::GetInstance()->Config(this);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
        CAlertScreen::GetInstance()->SetActionText(LS(SID_UPDATING_CDDB));
        CAlertScreen::GetInstance()->SetMessageText(tszMessage);
        break;
    }

    case IOPM_CDDB_UPDATE_PROCESSING:
    {
        tstrcpy(tszMessage, LS(SID_PROCESSING_CDDB_FILE));
        sprintf(szNumber, " %d ", Mesg.iData);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_OF));
        sprintf(szNumber, " %ld", Mesg.lData);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        CAlertScreen::GetInstance()->Config(this);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
        CAlertScreen::GetInstance()->SetActionText(LS(SID_UPDATING_CDDB));
        CAlertScreen::GetInstance()->SetMessageText(tszMessage);
        break;
    }

    case IOPM_CDDB_UPDATE_ERROR:
    {
        if (Presentation()->GetCurrentThing() == CAlertScreen::GetInstance())
            CAlertScreen::GetInstance()->HideScreen();
        CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
        CAlertScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
        CAlertScreen::GetInstance()->SetActionText(LS(SID_CDDB_UPDATE_FAILED));
        // TODO: is this error message overkill?  needed?
        CAlertScreen::GetInstance()->SetMessageText(gnerr_get_error_message(Mesg.iData));
        Add(CAlertScreen::GetInstance());
        break;
    }

    case IOPM_CDDB_UPDATE_END:
        if (Presentation()->GetCurrentThing() == CAlertScreen::GetInstance())
            CAlertScreen::GetInstance()->HideScreen();
        else
            Remove(CAlertScreen::GetInstance());

        // If no updates were processed, then tell the user.
        if (!m_bUpdatedCDDB)
        {
            CAlertScreen::GetInstance()->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
            CAlertScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
            CAlertScreen::GetInstance()->SetActionText(LS(SID_NO_UPDATES_FOUND));

            unsigned short usUpdateLevel, usDataRevision;
            if (CDDBGetUpdateLevel(usUpdateLevel, usDataRevision) == SUCCESS)
            {
                tstrcpy(tszMessage, LS(SID_CURRENT_UPDATE_LEVEL));
                sprintf(szNumber, "%d", usUpdateLevel);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                CAlertScreen::GetInstance()->SetMessageText(tszMessage);
            }

            Add(CAlertScreen::GetInstance());
        }
        break;

    default:
        break;
	}

    return CMenuScreen::Message(Mesg);
}


// Called when the user hits the select button.
// Acts based upon the currently highlighted menu item.
void
CSystemToolsMenuScreen::ProcessMenuOption(int iMenuIndex)
{
    m_bDoingOnlineUpdate = false;
    CAlertScreen* pAS = CAlertScreen::GetInstance();

    // switch on the sid instead of the index so the menu can be shuffled
    // without rearranging this code
	switch(s_menuItems[iMenuIndex].wCaptionSID)
	{
#ifdef SUPPORT_LOCAL_UPDATES
        case SID_UPDATE_FIRMWARE_LOCALLY:
        {
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            Add(pAS);

            UpdateImage( LOCAL_CONFIG_PATH );
            break;
        }
#endif
		case SID_UPDATE_FIRMWARE:		// Update Firmware (from CD)
        {
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            pAS->SetActionText(LS(SID_CHECKING_FOR_UPDATES));
            pAS->SetMessageText(LS(SID_PLEASE_WAIT));
            Add(pAS);

			UpdateImage( RESTORE_CD_CONFIG_PATH );
            break;
        }
        case SID_UPDATE_FIRMWARE_ONLINE:
        {
            // Don't attempt an update if the network isn't up.
            // TODO: this doesn't catch every case of network disconnection.
            if (!CDJPlayerState::GetInstance()->GetNetDataSource()->IsInitialized())
            {
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE_ONLINE));
                pAS->SetActionText(LS(SID_FIRMWARE_UPDATE_FAILED));
                pAS->SetMessageText(LS(SID_NETWORK_DISCONNECTED));
                Add(pAS);
                break;
            }
            else
            {
                pAS->Config(this);
                pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE_ONLINE));
                pAS->SetActionText(LS(SID_CHECKING_FOR_UPDATES));
                pAS->SetMessageText(LS(SID_PLEASE_WAIT));
                Add(pAS);
            }

            m_bDoingOnlineUpdate = true;
            UpdateImage( ONLINE_CONFIG_PATH );
            break;
        }
		case SID_SCAN_HD_FOR_MUSIC:		// Scan HD For Music
        {
            DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "Scan the HD for Music\n");
            StartHDScan(false);
            break;
        }

		case SID_UPDATE_CDDB:		// Update CDDB
        {
            // Get a list of update files from CD
            const char* szUpdateURL = RESTORE_CD_CONFIG_PATH;
            CUpdateManager* pUpdate = CUpdateManager::GetInstance();
            update_t updates = pUpdate->CheckForUpdates( szUpdateURL );
            
            if( !(updates & UM_UPDATE_CDDBUPDA) ) {
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_CDDB));
                pAS->SetActionText(LS(SID_NO_UPDATES_FOUND));
                if (updates == UM_UPDATE_UNAVAIL)
                    pAS->SetMessageText(LS(SID_PLEASE_INSERT_RESTORE_CD));
                Add(pAS);
                break;
            }

            // Get the list of updates
            SimpleList<cddb_update_info_t> cddbfiles;
            int count = pUpdate->GetCDDBUpdateList( szUpdateURL, cddbfiles );

            char szNumber[32];
            TCHAR tszNumber[32];
            TCHAR tszMessage[128];

            if( count < 0 ) {
                // error
            } else if( count == 0 ) {
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_CDDB));
                pAS->SetActionText(LS(SID_NO_UPDATES_FOUND));

                unsigned short usUpdateLevel, usDataRevision;
                if (CDDBGetUpdateLevel(usUpdateLevel, usDataRevision) == SUCCESS)
                {
                    tstrcpy(tszMessage, LS(SID_CURRENT_UPDATE_LEVEL));
                    sprintf(szNumber, "%d", usUpdateLevel);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    pAS->SetMessageText(tszMessage);
                }

                Add(pAS);
                break;
            }

            m_bUpdatingCDDB = true;
            
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_CDDB));
            pAS->SetActionText(LS(SID_UPDATING_CDDB));
            pAS->SetMessageText(LS(SID_CHECKING_FOR_UPDATES));

            // Stop ripping and playback and set the message text.
            StopRippingAndPlayback();

            Add(pAS);

            int i = 1;
            char inurl[512];   // eek
            int inidx=0, outidx=0;
            char outurl[512];  // eek

            // Determine the source of the file
            strcpy(inurl, szUpdateURL);
            for( int z = strlen(inurl); z; z-- ) {
                if( inurl[z] == '/' ) {
                    inurl[z] = 0;
                    inidx = z;
                    break;
                }
            }
            
            strcpy(outurl, ROOT_URL_PATH);
            outidx = strlen(outurl);
            
            for (SimpleListIterator<cddb_update_info_t> itFiles = cddbfiles.GetHead();			
                itFiles != cddbfiles.GetEnd(); ++itFiles)				
            {
                tstrcpy(tszMessage, LS(SID_COPYING_CDDB_FILE));
                sprintf(szNumber, " %d ", i);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_OF));
                sprintf(szNumber, " %d ", cddbfiles.Size());
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);

//                pAS->SetActionText(LS(SID_COPYING_CDDB_FILE));
                pAS->SetMessageText(tszMessage);

                strcat( &(inurl[inidx]), (*itFiles).path );
                strcat( &(outurl[outidx]), (*itFiles).path );
                
                if( pUpdate->CopyFile( inurl, outurl ) == 0 )
                {
                    tstrcpy(tszMessage, LS(SID_PROCESSING_CDDB_FILE));
                    sprintf(szNumber, " %d ", i);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);
                    tstrcat(tszMessage, LS(SID_OF));
                    sprintf(szNumber, " %d ", cddbfiles.Size());
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);

//                    pAS->SetActionText(LS(SID_PROCESSING_CDDB_FILE));
                    pAS->SetMessageText(tszMessage);
                    pAS->ResetProgressBar(0, 0);

                    CCDDBQueryManager::GetInstance()->UpdateLocal(FilenameFromURLInPlace(outurl));
                }
                else
                {
                    tstrcpy(tszMessage, LS(SID_ERROR_COPYING_CDDB));
                    sprintf(szNumber, " %d ", i);
                    CharToTcharN(tszNumber, szNumber, 31);
                    tstrcat(tszMessage, tszNumber);

                    pAS->SetActionText(LS(SID_ERROR));
                    pAS->SetMessageText(tszMessage);
                }
                inurl[inidx] = outurl[outidx] = 0;
                ++i;
            }
            pAS->HideScreen();

            m_bUpdatingCDDB = false;
			break;
        }

		case SID_UPDATE_CDDB_ONLINE:		// Update CDDB online
        {
            // Don't attempt an update if the network isn't up.
            // TODO: this doesn't catch every case of network disconnection.
            if (!CDJPlayerState::GetInstance()->GetNetDataSource()->IsInitialized())
            {
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
                pAS->SetActionText(LS(SID_CDDB_UPDATE_FAILED));
                pAS->SetMessageText(LS(SID_NETWORK_DISCONNECTED));
                Add(pAS);
                break;
            }

            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_CDDB_ONLINE));
            pAS->SetActionText(LS(SID_UPDATING_CDDB));
            pAS->SetMessageText(LS(SID_CHECKING_FOR_UPDATES));

            // Stop ripping and playback and set the message text.
            StopRippingAndPlayback();

            Add(pAS);

            m_bUpdatedCDDB = false;
            CCDDBQueryManager::GetInstance()->UpdateOnlineAsynch();

			break;
        }
        
	};
}

void
CSystemToolsMenuScreen::UpdateImage( const char* szSourceURL )
{
    CUpdateManager* pUpdate = CUpdateManager::GetInstance();
    CAlertScreen* pAS = CAlertScreen::GetInstance();

    bool bUpdateAvailable = false;
    update_t updates = pUpdate->CheckForUpdates( szSourceURL );
    if( (updates & (UM_UPDATE_SYSTEM|UM_UPDATE_FIRMWARE)) ) {
        bUpdateAvailable = true;
    }

    if( bUpdateAvailable )
	{
        m_szSourceURL = szSourceURL;

		CYesNoScreen::GetInstance()->Config(this, WriteImageCallback);
        if( m_bDoingOnlineUpdate )
            CYesNoScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_FIRMWARE_ONLINE));
        else
            CYesNoScreen::GetInstance()->SetTitleText(LS(SID_UPDATE_FIRMWARE));
        CYesNoScreen::GetInstance()->SetActionText(LS(SID_INSTALL_UPDATE_QM));
		Add(CYesNoScreen::GetInstance());
	}
	else if( (updates & UM_ERROR) && m_bDoingOnlineUpdate ) {
        // Generalize online update errors into 'the server was not available' - probably correct
        pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
        pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
        pAS->SetActionText(LS(SID_UPDATE_SERVER_NOT_FOUND));
        Add(pAS);
		DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "Update server not available\n");
    }
    else
	{
        pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
        pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
        pAS->SetActionText(LS(SID_NO_UPDATES_FOUND));
//        pAS->SetMessageText(LS(SID_NO_NEW_UPDATES_TRY_AGAIN_LATER));

	    TCHAR pszVersionNum[128] = {0};
        tstrcpy(pszVersionNum, LS(SID_FIRMWARE_VERSION));
        tstrncat(pszVersionNum, LS(SID_COLON_SPACE), 128-1);
        TCHAR tcVersion[128];
        CharToTcharN(tcVersion, DDO_VERSION_STR, 128-1);
        tstrncat(pszVersionNum, tcVersion, 128-1);
        pAS->SetMessageText(pszVersionNum);

        Add(pAS);
		DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "No updates found\n");
	}
}


void
CSystemToolsMenuScreen::WriteImageCB(bool bProgram)
{
    CAlertScreen* pAS = CAlertScreen::GetInstance();

    if( bProgram )
    {
        const char* szSourceURL = m_szSourceURL;
        const char szDownloadURL[] = DOWNLOAD_CONFIG_PATH;

		// stop the player, deconfigure
		CRecordingManager::GetInstance()->StopRipping();
        if (CPlayManager::GetInstance()->GetPlayState() != CMediaPlayer::NOT_CONFIGURED)
			CPlayManager::GetInstance()->Deconfigure();
        
        CUpdateManager* pUpdate = CUpdateManager::GetInstance();
        update_t updates = pUpdate->CheckForUpdates( m_szSourceURL );

        if (updates && m_bDoingOnlineUpdate) {
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE_ONLINE));
            pAS->SetActionText(LS(SID_DOWNLOADING_UPDATES));
            pAS->SetMessageText(LS(SID_PLEASE_WAIT));
            Add(pAS);
            Presentation()->MoveFocusTree(pAS);

            // copy all the files locally if we're doing an online update
            update_t downloaded_updates = pUpdate->CopyUpdatesFromSource( m_szSourceURL, DOWNLOAD_URL_PATH, updates );

            // check to see if we downloaded everything we wanted
            if ( updates != downloaded_updates ) {
                DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "Update Failed To Download All The Files\n");
                // report the download failure to the user
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE_ONLINE));
                pAS->SetActionText(LS(SID_FIRMWARE_UPDATE_FAILED));
                pAS->SetMessageText(LS(SID_FAILED_TO_DOWNLOAD_UPDATE_FILES));

                // wipe the update directory
                pUpdate->DeleteUpdatesAtSource( szDownloadURL );

                return ;
            }

            // swap the source to the newly downloaded files
            szSourceURL = szDownloadURL;
        }
        
        // If we are doing a 'system' update, which involves copying files, then show that
        if( (updates & UM_UPDATE_SYSTEM) ) {
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            pAS->SetActionText(LS(SID_UPDATING_SYSTEM_FILES));
            pAS->SetMessageText(LS(SID_PLEASE_WAIT));
            Add(pAS);
            Presentation()->MoveFocusTree(pAS);

            if( pUpdate->UpdateFromSource( szSourceURL, UM_UPDATE_SYSTEM ) < 0 ) {
                // TODO fixup the UI portions here
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
                pAS->SetActionText(LS(SID_FIRMWARE_UPDATE_FAILED));
                pAS->SetMessageText(LS(SID_CORRUPT_FIRMWARE));
                DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "Update Failed To Copy Files\n");
                return ;
            }
        }

        if( (updates & UM_UPDATE_FIRMWARE) ) {
            // If we are doing a 'firmware' update, which burns the flash, then show that
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            pAS->SetActionText(LS(SID_UPDATING_FIRMWARE));
            pAS->SetMessageText(LS(SID_PLEASE_WAIT));
            Add(pAS);
            Presentation()->MoveFocusTree(pAS);

            DAIDisable();
            
            if( pUpdate->UpdateFromSource( szSourceURL, UM_UPDATE_FIRMWARE ) < 0 ) {
                pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
                pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
                pAS->SetActionText(LS(SID_FIRMWARE_UPDATE_FAILED));
                pAS->SetMessageText(LS(SID_CORRUPT_FIRMWARE));
                DEBUGP( DBG_SOURCE_MENU_SCREEN, DBGLEV_INFO, "Update Failed, Corrupt Image\n");

                // clean up the temp download directory if we're doing an online update
                if (m_bDoingOnlineUpdate)
                    pUpdate->DeleteUpdatesAtSource( szDownloadURL );
#ifdef SUPPORT_LOCAL_UPDATES
                else
                    pUpdate->DeleteUpdates();
#endif

                // Bail out here, hopefully everything is still intact
                DAIEnable();
                return ;
            }

            // Now that we are updated, get ready to reset
            //pAS->HideScreen();
            pAS->Config(this);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            pAS->SetActionText(LS(SID_RESETTING));
            pAS->SetMessageText(LS(SID_PLEASE_WAIT));
            Add(pAS);
		
            // clean up the temp download directory if we're doing an online update
            if (m_bDoingOnlineUpdate)
                pUpdate->DeleteUpdatesAtSource( szDownloadURL );
#ifdef SUPPORT_LOCAL_UPDATES
            else
                pUpdate->DeleteUpdates();
#endif

            // a little delay, so the user can see what is happening
            cyg_thread_delay(100);

            // commit databases and reset! (call does not return)
            CDJPlayerState::GetInstance()->SafeReset();
        }

        // clean up the temp download directory if we're doing an online update
        if (m_bDoingOnlineUpdate)
            pUpdate->DeleteUpdatesAtSource( szDownloadURL );

        // we're done.  tell the user.
        if (updates)
        {
            pAS->Config(this, AS_DEFAULT_TIMEOUT_LENGTH);
            pAS->SetTitleText(LS(SID_UPDATE_FIRMWARE));
            pAS->SetActionText(LS(SID_COMPLETED));
            Add(pAS);
        }
    }
    else
    {
        pAS->HideScreen();
    }
}

// Progress for file copies
void
CSystemToolsMenuScreen::FileProgressCallback( const char* szFilename, int iCurrent, int iTotal )
{
    s_pSystemToolsMenuScreen->FileProgressCB( szFilename, iCurrent, iTotal );
}
void
CSystemToolsMenuScreen::FileProgressCB( const char* szFilename, int iCurrent, int iTotal )
{
    if( iCurrent == 0 && !m_bUpdatingCDDB ) {
        // dont update the system message text on cddb updates
        CAlertScreen::GetInstance()->SetMessageText( FullFilenameFromURLInPlace(szFilename) );
    }
    CAlertScreen::GetInstance()->ResetProgressBar( iCurrent, iTotal );
}
// Progress for flash burns
void
CSystemToolsMenuScreen::FlashProgressCallback( int iCurrent, int iTotal )
{
    s_pSystemToolsMenuScreen->FlashProgressCB( iCurrent, iTotal );
}
void
CSystemToolsMenuScreen::FlashProgressCB( int iCurrent, int iTotal )
{
    CAlertScreen::GetInstance()->ResetProgressBar( iCurrent, iTotal );
}

void
CSystemToolsMenuScreen::WriteImageCallback(bool bProgram)
{
	if(!s_pSystemToolsMenuScreen)
		return;

	s_pSystemToolsMenuScreen->WriteImageCB(bProgram);
}

// Stops ripping or playback and prints the appropriate message in the message text in the alert screen.
void
CSystemToolsMenuScreen::StopRippingAndPlayback()
{
    // Are we ripping?
    if (CRecordingManager::GetInstance()->IsRipping())
    {
        // Stop ripping.
        CPlayerScreen::GetPlayerScreen()->StopRipping();
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_STOPPING_FAST_RECORDING));
    }
    // Are we playing?
    else if ((CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING) ||
        (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED))
    {
        // Stop playing.
        CPlayerScreen::GetPlayerScreen()->StopPlayback();
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_STOPPING_PLAYBACK));
    }
//    else
//        CAlertScreen::GetInstance()->SetMessageText(LS(SID_EMPTY_STRING));
}

void
CSystemToolsMenuScreen::HDScanDoneCB()
{
    CPlayManager* pPlayManager = CPlayManager::GetInstance();
    IPlaylist* pCurrentPlaylist = pPlayManager->GetPlaylist();

    if ((!pCurrentPlaylist) || (pCurrentPlaylist && pCurrentPlaylist->GetSize() == 0))
    {
        if( CDJPlayerState::GetInstance()->GetSource() != CDJPlayerState::CD ) {
            CPlayerScreen::GetPlayerScreen()->DisplaySelectTracksScreen();
        }
    }
    
    GotoPreviousMenu();
}


void
CSystemToolsMenuScreen::HDScanDoneCallback()
{
	if(!s_pSystemToolsMenuScreen)
		return;

	s_pSystemToolsMenuScreen->HDScanDoneCB();
}

void
CSystemToolsMenuScreen::StartHDScan(bool bHideMenuWhenDone)
{
    m_bHideMenuWhenScanDone = bHideMenuWhenDone;

    CAlertScreen::GetInstance()->Config(this);
    CAlertScreen::GetInstance()->SetTitleText(LS(SID_SCAN_HD_FOR_MUSIC));
    CAlertScreen::GetInstance()->SetActionText(LS(SID_LOOKING_FOR_HD_TRACKS));
    // Set the message text based on what we're going to do.
    if (CRecordingManager::GetInstance()->IsRipping())
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_STOPPING_FAST_RECORDING));
    else if ((CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING) ||
        (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED))
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_STOPPING_PLAYBACK));
    else
        CAlertScreen::GetInstance()->SetMessageText(LS(SID_EMPTY_STRING));

    Add(CAlertScreen::GetInstance());

    CPlayManager* pPlayManager = CPlayManager::GetInstance();

	// if there is a current playlist, and we're not on a CD, clear it
    bool bCleared = false;
	IPlaylist* pCurrentPlaylist = pPlayManager->GetPlaylist();
	if (pCurrentPlaylist)
    {
        ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->CancelPendingQueries();
        if( CDJPlayerState::GetInstance()->GetSource() != CDJPlayerState::CD )
        {
            pCurrentPlaylist->Clear();
            bCleared = true;
        }
    }

    // talk to the player screen & tell it to stop doin' stuff, yo
    if (CRecordingManager::GetInstance()->IsRipping())
        CPlayerScreen::GetPlayerScreen()->StopRipping();
    else if ((CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING) ||
            (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::STOPPED))
        CPlayerScreen::GetPlayerScreen()->StopPlayback();

    if( bCleared )
    {
        pPlayManager->Deconfigure();
    }
    
    if (!bHideMenuWhenDone)
    {
        // Start the rescan.
        CDJPlayerState::GetInstance()->SetScanningHD(true);
        CProgressWatcher::GetInstance()->SetTask(TASK_REFRESHING_CONTENT);
        CFatDataSource* pFatDS = CDJPlayerState::GetInstance()->GetFatDataSource();
        pPlayManager->GetContentManager()->DeleteRecordsFromDataSource(pFatDS->GetInstanceID());
        pPlayManager->RefreshContent( pFatDS->GetInstanceID() );
    }
}

///////////////////////////////////////////////////
// End updater functions
///////////////////////////////////////////////////
