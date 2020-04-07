//
// RestoreScreen.cpp: contains the implementation of the CRestoreScreen class
// danb@fullplaymedia.com 05/01/2002
// Copyright (c) Fullplay Media (TM). All rights reserved
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_cache.h>   // SafeReset
#include <cyg/hal/hal_edb7xxx.h> // SafeReset
#include <cyg/hal/hal_intr.h>
#include <cyg/fileio/fileio.h>    // mount, unmount

#include <main/ui/RestoreScreen.h>
#include <main/ui/AlertScreen.h>
#include <main/ui/Bitmaps.h>
#include <main/ui/Fonts.h>
#include <main/ui/Keys.h>
#include <main/ui/Strings.hpp>
#include <main/ui/UI.h>

#include <main/main/DJEvents.h>
#include <main/main/RestoreEvents.h>
#include <main/main/SpaceMgr.h>

#include <extras/cddb/gn_memory.h>
#include <extras/cddb/gn_system.h>

#include <fs/utils/format_fat32/format.h>
#include <fs/utils/chkdsk_fat32/chkdsk.h>
#include <util/datastructures/SimpleList.h>
#include <devs/audio/dai.h>

#include <fs/fat/sdapi.h>
#include <main/main/FatHelper.h>   // FullFilenameFromURLInPlace
#include <main/main/UpdateManager.h>

#include <datasource/datasourcemanager/DataSourceManager.h>   // add hooks for needed data sources
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/fatdatasource/FatDataSource.h>

#include <util/debug/debug.h>          // debugging hooks

#include <errno.h>

DEBUG_MODULE_S( DBG_RESTORE_SCREEN, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE );
DEBUG_USE_MODULE( DBG_RESTORE_SCREEN );


//extern CPlayScreen* g_pMainWindow;
CRestoreScreen* CRestoreScreen::s_pRestoreScreen = 0;

// This is a singleton class.
CRestoreScreen*
CRestoreScreen::GetInstance()
{
	if (!s_pRestoreScreen) {
		s_pRestoreScreen = new CRestoreScreen(NULL);
	}
	return s_pRestoreScreen;
}

CRestoreScreen::CRestoreScreen(CScreen* pParent)
  : CScreen(pParent)
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "rs:Ctor\n");

    m_bFormat = m_bFormatStatus = false;
    m_bCheckDisk = m_bCheckDiskStatus = false;
    m_bUpdateCDDB = m_bUpdateCDDBStatus = false;
    m_bUpdateSoftware = m_bUpdateSoftwareStatus = false;
    m_bDeleteLocalContent = m_bDeleteLocalContentStatus = false;
    m_bClearCDCache = m_bClearCDCacheStatus = false;
    m_bClearLocalContentDatabase = m_bClearLocalContentDatabaseStatus = false;
    m_bClearSavedSettings = m_bClearSavedSettingsStatus = false;
    m_bResetCDDB = m_bResetCDDBStatus = false;
    m_bRestoreCDDB = false;
    
	BuildScreen();

    ResetProgressBar();

    m_iTryCount = 0;
}

CRestoreScreen::~CRestoreScreen()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "rs:Dtor\n");
}

void
CRestoreScreen::Draw()
{
    BeginDraw();
    CScreen::Draw();
    DrawProgressBar();
    EndDraw();
}

void
CRestoreScreen::ForceRedraw()
{
	Invalidate(mReal);
	Draw();
}

SIGNED
CRestoreScreen::Message(const PegMessage &Mesg)
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "CRestoreScreen::Message\n");
    switch (Mesg.wType)
    {
    case PM_KEY:
        
        switch (Mesg.iData)
        {
        case IR_KEY_SELECT:
        case KEY_SELECT:
            break;
        case IR_KEY_EXIT:
        case KEY_EXIT:
            DoReset();
            break;
        default:
            break;
        }
        break;
        
    default:
        break;
    }
    return CScreen::Message(Mesg);
}

void
CRestoreScreen::SetMessageText(const char* szText, CSystemMessageString::SysMsgType iMessageType)
{
    //KillTimer(PS_TIMER_MESSAGE_TEXT_TTL);
    //if(iTimeoutLength > 0)
    //    SetTimer(PS_TIMER_MESSAGE_TEXT_TTL, iTimeoutLength, 0);
    m_pMessageTextString->SystemMessage(szText, iMessageType);
    Screen()->Invalidate(m_pMessageTextString->mReal);
    //SynchTextScrolling();
    Draw();
}

void
CRestoreScreen::SetMessageText(const TCHAR* szText, CSystemMessageString::SysMsgType iMessageType)
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "rs:SetMessageText [%w]\n", szText);
    //KillTimer(PS_TIMER_MESSAGE_TEXT_TTL);
    //if(iTimeoutLength > 0)
    //    SetTimer(PS_TIMER_MESSAGE_TEXT_TTL, iTimeoutLength, 0);
    m_pMessageTextString->SystemMessage(szText, iMessageType);
    Screen()->Invalidate(m_pMessageTextString->mReal);
    //SynchTextScrolling();
    Draw();
}

void
CRestoreScreen::ResetProgressBar(int iProgress, int iTotal)
{
    m_iProgressBarTotal = iTotal;
    UpdateProgressBar(iProgress);
}

void
CRestoreScreen::UpdateProgressBar(int iProgress)
{
    // invalidate that part of the screen
    m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom);
    Invalidate(m_ProgressBarRect);
    
    if (iProgress <= 0 || m_iProgressBarTotal <= 0)
        // we don't want to show the progress bar
        m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wLeft - 1, mReal.wBottom);
    else
    {
        int iBarWidth = (int)((double)iProgress * ((double)(mReal.wRight - mReal.wLeft) / (double)m_iProgressBarTotal));
        m_ProgressBarRect.Set(mReal.wLeft, mReal.wBottom - 14, iBarWidth, mReal.wBottom);
    }
    
    // only redraw if this screen has input focus
    if (Presentation()->GetCurrentThing() == this)
        Draw();
}

void
CRestoreScreen::DrawProgressBar()
{
    if ((Presentation()->GetCurrentThing() == this) && (m_ProgressBarRect.Width() > 0))
        Screen()->InvertRect(this, m_ProgressBarRect);
}

void
CRestoreScreen::SetTitleText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pScreenTitle->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pScreenTitle->DataGet(), m_pScreenTitle->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pScreenTitle->mReal.wTop, m_pScreenTitle->mReal.wRight, m_pScreenTitle->mReal.wBottom);
    m_pScreenTitle->Resize(ChildRect);
    
    Screen()->Invalidate(m_pScreenTitle->mReal);
    Draw();
}

void
CRestoreScreen::SetActionText(const TCHAR* szText)
{
    PegRect ChildRect;
    int iTextLength = 0, iCaptionLength = 0;
    
    m_pActionTextString->DataSet(szText);
    // center the string
    iTextLength = Screen()->TextWidth(m_pActionTextString->DataGet(), m_pActionTextString->GetFont());
    iCaptionLength = mReal.wRight - mReal.wLeft;
    if(iTextLength < iCaptionLength)
        ChildRect.Set(((iCaptionLength - iTextLength) / 2), m_pActionTextString->mReal.wTop, m_pActionTextString->mReal.wRight, m_pActionTextString->mReal.wBottom);
    else
        ChildRect.Set(mReal.wLeft, m_pActionTextString->mReal.wTop, m_pActionTextString->mReal.wRight, m_pActionTextString->mReal.wBottom);
    m_pActionTextString->Resize(ChildRect);
    Screen()->Invalidate(m_pActionTextString->mReal);
    Draw();
}

void
CRestoreScreen::BuildScreen()
{
    PegRect ChildRect;
    mReal.Set(0, 0, UI_X_SIZE-1, UI_Y_SIZE-1);
    InitClient();
    RemoveStatus(PSF_MOVEABLE|PSF_SIZEABLE);
    
    // track text area
    ChildRect.Set(mReal.wLeft, mReal.wTop + 24, mReal.wRight, mReal.wTop + 40);
    m_pActionTextString = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
    m_pActionTextString->SetFont(&FONT_PLAYSCREENBIG);
    m_pActionTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pActionTextString);
    //SetActionText(LS(SID_PUSH_SELECT_TO_RESTORE));
    
    // screen title
    ChildRect.Set(mReal.wLeft, mReal.wTop, mReal.wRight, mReal.wTop + 13);
    m_pScreenTitle = new PegString(ChildRect, NULL, 0, FF_NONE | TT_COPY);
    m_pScreenTitle->SetFont(&FONT_MENUSCREENTITLE);
    m_pScreenTitle->RemoveStatus(PSF_ACCEPTS_FOCUS);
    Add(m_pScreenTitle);
    SetTitleText(LS(SID_RESTORE_PROCEDURE));
    
    // the horizontal bar on the top of the screen
    ChildRect.Set(mReal.wLeft, mReal.wTop + 16, mReal.wRight, mReal.wTop + 17);
    m_pTopScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
    m_pTopScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pTopScreenHorizontalDottedBarIcon);
    
    // the horizontal bar on the screen
    ChildRect.Set(mReal.wLeft, mReal.wBottom - 14, mReal.wRight, mReal.wBottom - 13);
    m_pScreenHorizontalDottedBarIcon = new PegIcon(ChildRect, &gbHorizontalBarBitmap);
    m_pScreenHorizontalDottedBarIcon->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pScreenHorizontalDottedBarIcon);
    
    // the message region of the screen
    ChildRect.Set(mReal.wLeft, mReal.wBottom - 13, mReal.wRight, mReal.wBottom);
    m_pMessageTextString = new CSystemMessageString(ChildRect, NULL, 0, FF_NONE | TT_COPY );
    m_pMessageTextString->SetFont(&FONT_PLAYSCREEN);
    m_pMessageTextString->RemoveStatus(PSF_ACCEPTS_FOCUS);    
    Add(m_pMessageTextString);
    //SetMessageText("Push Cancel to Exit Restore Procedure", CSystemMessageString::STATUS);
}


bool
CRestoreScreen::SetRestoreOption(eRestoreOption eOption, bool bEnable)
{
    switch (eOption)
    {
    case EVERYTHING:
        m_bFormat                    = bEnable;
        m_bCheckDisk                 = false;
        m_bUpdateCDDB                = bEnable;
        m_bResetCDDB                 = bEnable;
        m_bUpdateSoftware            = bEnable;
        m_bDeleteLocalContent        = false;
        m_bClearCDCache              = false;
        m_bClearLocalContentDatabase = false;
        m_bClearSavedSettings        = false;
        break;
    case FORMAT_HD:
        m_bFormat = bEnable;
        break;
    case CHECK_DISK:
        m_bCheckDisk = bEnable;
        break;
    case UPDATE_CDDB:
        m_bUpdateCDDB = bEnable;
        break;
    case UPDATE_SOFTWARE:
        m_bUpdateSoftware = bEnable;
        break;
    case DELETE_CONTENT:
        m_bDeleteLocalContent = bEnable;
        break;
    case RESET_CDDB:
        m_bResetCDDB = bEnable;
        break;
    case CLEAR_CD_CACHE:
        m_bClearCDCache = bEnable;
        break;
    case CLEAR_LOCAL_CONTENT_DATABASE:
        m_bClearLocalContentDatabase = bEnable;
        break;
    case CLEAR_SAVED_SETTINGS:
        m_bClearSavedSettings = bEnable;
        break;
    }

    return bEnable;
}


bool
CRestoreScreen::GetRestoreOption(eRestoreOption eOption)
{
    switch (eOption)
    {
    case EVERYTHING:
        return (m_bFormat && m_bUpdateCDDB && m_bUpdateSoftware);
    case FORMAT_HD:
        return m_bFormat;
    case CHECK_DISK:
        return m_bCheckDisk;
    case UPDATE_CDDB:
        return m_bUpdateCDDB;
    case UPDATE_SOFTWARE:
        return m_bUpdateSoftware;
    case DELETE_CONTENT:
        return m_bDeleteLocalContent;
    case RESET_CDDB:
        return m_bResetCDDB;
    case CLEAR_CD_CACHE:
        return m_bClearCDCache;
    case CLEAR_LOCAL_CONTENT_DATABASE:
        return m_bClearLocalContentDatabase;
    case CLEAR_SAVED_SETTINGS:
        return m_bClearSavedSettings;
    }
    // satisfy compiler
    return false;
}

bool
CRestoreScreen::ChkdskStatusCallback(int iPass, int iPhase, int iCurrent, int iTotal)
{
    if(s_pRestoreScreen)
        return s_pRestoreScreen->ChkdskStatusCB(iPass, iPhase, iCurrent, iTotal);
    
    return true;
}

bool
CRestoreScreen::ChkdskStatusCB(int iPass, int iPhase, int iCurrent, int iTotal)
{
    const WORD phase_string_id[] =
        {
            SID_CHECKING_DRIVE_PASS,
            SID_READING_FATS,
            SID_CHECKING_CLUSTER_CHAINS,
            SID_CHECKING_DIRECTORIES,
            SID_CHECK_FOR_LOST_FILES,
            SID_UPDATING_FATS,
        };
    
    static int lastpass = -1;
    static int lastphase = -1;
    
    char szInfo[64];
    TCHAR tszInfo[64];
    TCHAR tszMessage[256];
    
    if(iPass != lastpass)
    {
        tstrcpy(tszMessage, LS(SID_CHECKING_DRIVE_PASS));
        sprintf(szInfo," %d", iPass+1);
        CharToTcharN(tszInfo, szInfo, 63);
        tstrcat(tszMessage, tszInfo);
        SetActionText(tszMessage);
       
        lastpass = iPass;
    }
    
    if(iPhase != lastphase)
    {
        tstrcpy(tszMessage, LS(SID_CHECK_PHASE));
        sprintf(szInfo," %d - ", iPhase+1); 
        CharToTcharN(tszInfo, szInfo, 63);
        tstrcat(tszMessage, tszInfo);
        tstrcat(tszMessage, LS(phase_string_id[iPhase]));	    
        SetMessageText(tszMessage, CSystemMessageString::STATUS);
        
        lastphase = iPhase;
    }

    ResetProgressBar(iCurrent,iTotal);
    
    // return if chkdsk should continue (true) or cancel (false)
    return true;
}

void
CRestoreScreen::DoSelectedOptions()
{
    ++m_iTryCount;
    
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Starting Restore\n");
    if (m_bFormat)
    {
        m_bDeleteLocalContent = false;
        m_bClearLocalContentDatabase = false;
        m_bClearCDCache = false;
        m_bClearSavedSettings = false;
        DoFormat();
    }

    if (m_bCheckDisk)
        DoChkdsk();
    
    if (m_bDeleteLocalContent)
        DoDeleteLocalContent();

    if (m_bClearLocalContentDatabase)
        DoClearLocalContentDatabase();

    if (m_bClearCDCache)
        DoClearCDCache();

    if (m_bClearSavedSettings)
        DoClearSavedSettings();

    if (m_bUpdateCDDB || m_bUpdateSoftware)
        DoRestore();

    if (m_bResetCDDB)
        DoResetCDDB();

    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Restore Complete\n");
    DoSummary();
}


void
CRestoreScreen::DoFormat()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Formatting HD\n");
    ResetProgressBar();

    // format drive
    SetMessageText(LS(SID_EMPTY_STRING), CSystemMessageString::STATUS);
    SetActionText(LS(SID_FORMATTING_HARD_DISK));
	
	// shutdown and restart fat around a format
	pc_system_close(0);
    int Status = format_drive("/dev/hda/",&ProgressCallback);
	pc_system_init(0);

    ResetProgressBar();
    m_bFormatStatus = (Status == 0);
}


void 
CRestoreScreen::DoChkdsk()
{
    const char fname[] = "/dev/hda/";

	int status = chkdsk_fat32(fname,&ChkdskStatusCallback);
    
    SetActionText(LS(SID_DRIVE_CHECK_COMPLETE));
    
    m_bCheckDiskStatus = (status == FSOK);
}


void 
CRestoreScreen::DoDeleteLocalContent()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Deleting All Local Content - Not Implemented\n" );
    // todo: implement this
    m_bDeleteLocalContentStatus = true;
}


void 
CRestoreScreen::DoClearLocalContentDatabase()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Deleting Local Content Database %s\n", METAKIT_HD_PERSIST_FILE );
    SetActionText(LS(SID_CLEAR_LOCAL_CONTENT_DATABASE));
    STAT Stat;
    if ((pc_stat(METAKIT_HD_PERSIST_FILE, &Stat) == 0) && !pc_unlink(METAKIT_HD_PERSIST_FILE))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete Local Content Database %s\n", METAKIT_HD_PERSIST_FILE );
        m_bClearLocalContentDatabaseStatus = false;
    }
    else
    {
        m_bClearLocalContentDatabaseStatus = true;
    }
}


void 
CRestoreScreen::DoClearCDCache()
{
    bool Status = true;
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Deleting CD Cache Files\n" );
    SetActionText(LS(SID_CLEAR_CD_CACHE));
    STAT Stat;
    if ((pc_stat(DISK_METADATA_CACHE_PATH, &Stat) == 0) && !pc_unlink(DISK_METADATA_CACHE_PATH))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete data CD Cache File %s\n", DISK_METADATA_CACHE_PATH );
        Status = false;
    }
    if ((pc_stat(CDDB_CACHE_FILE, &Stat) == 0) && !pc_unlink(CDDB_CACHE_FILE))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete CDDB Cache File %s %d\n", CDDB_CACHE_FILE, errno );
        Status = false;
    }
    if ((pc_stat(CDDB_CACHE_BACKUP_FILE, &Stat) == 0) && !pc_unlink(CDDB_CACHE_BACKUP_FILE))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete CDDB Backup Cache File %s %d\n", CDDB_CACHE_BACKUP_FILE, errno );
        Status = false;
    }
    m_bClearCDCacheStatus = Status;
}


void 
CRestoreScreen::DoClearSavedSettings()
{
    bool Status = false;
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Deleting Saved Settings File %s\n", SAVE_SETTINGS_PATH );
    SetActionText(LS(SID_RESTORE_DEFAULT_USER_SETTINGS));
    STAT Stat;
    if ((pc_stat(SAVE_SETTINGS_PATH, &Stat) == 0) && !pc_unlink(SAVE_SETTINGS_PATH))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete Saved Settings File %s\n", SAVE_SETTINGS_PATH );
        if (errno != PENOENT)
            Status = false;
    }
    if ((pc_stat(SYSTEM_PROGRESS_PATH, &Stat) == 0) && !pc_unlink(SYSTEM_PROGRESS_PATH))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete System Progress File %s\n", SYSTEM_PROGRESS_PATH );
        if (errno != PENOENT)
            Status = false;
    }
    char* szPlaylistURL = const_cast<char*>(FullFilenameFromURLInPlace(CURRENT_PLAYLIST_URL));
    if ((pc_stat(szPlaylistURL, &Stat) == 0) && !pc_unlink(szPlaylistURL))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete Playlist File %s\n", szPlaylistURL );
        if (errno != PENOENT)
            Status = false;
    }
    if ((pc_stat(TEST_RERUN_FILE, &Stat) == 0) && !pc_unlink(TEST_RERUN_FILE))
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Unable to Delete Monkey Test File %s\n", TEST_RERUN_FILE );
        if (errno != PENOENT)
            Status = false;
    }
    m_bClearSavedSettings = Status;
}


void
CRestoreScreen::DoResetCDDB()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Reset CDDB\n" );
    SetActionText(LS(SID_RESET_CDDB));

    bool        Status   = true;
    gn_uchar_t* heap      = NULL;
    gn_size_t   heap_size = 0;
    gn_error_t  error     = gnmem_initialize(heap,heap_size);
    if (error)
    {
        DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Error initializing cddb memory: 0x%x: %s\n", error, gnerr_get_error_message(error));
        Status = false;
    }
    else
    {
        // Resets CDDB, clearing all updates.
        // The CDDB cache is unchanged.
        error = gnsys_reset_db();
        if (error)
        {
            DEBUG( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Error resetting db: 0x%x: %s\n", error, gnerr_get_error_message(error));
            Status = false;
        }
    }
    m_bResetCDDBStatus = Status;
}


void
CRestoreScreen::ProgressCallback(int iCurrent, int iTotal)
{
    if(s_pRestoreScreen)
        s_pRestoreScreen->ProgressCB(iCurrent, iTotal);
}

void
CRestoreScreen::ProgressCB(int iCurrent, int iTotal)
{
    ResetProgressBar(iCurrent, iTotal);
}
void
CRestoreScreen::FileCopyCallback(const char* szFilename, int iCurrent, int iTotal)
{
    if(s_pRestoreScreen)
        s_pRestoreScreen->FileCopyCB(szFilename, iCurrent, iTotal);
}

void
CRestoreScreen::FileCopyCB(const char* szFilename, int iCurrent, int iTotal)
{
    if( iCurrent == 0 ) {
        // New file being copied
        SetMessageText(FullFilenameFromURLInPlace(szFilename), CSystemMessageString::STATUS);
    }
    ResetProgressBar(iCurrent, iTotal);
}

void
CRestoreScreen::DoRestore()
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Restoring CDDB and/or Firmware from CD\n");

	CFatDataSource* pFatDS;
	CCDDataSource* pCDDS;

    if (DoCDCheck(this, LS(SID_RESTORE_PROCEDURE)))
    {
        // make sure the fat layer is initialized, in case we did a format/whatever
        pc_system_init(0);

		pFatDS = (CFatDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( FAT_DATA_SOURCE_CLASS_ID, 0 );
	    
	    // make sure FAT datasource is working, as well
	    if(!pFatDS)
	    {
			DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_ERROR, "Can't find fat data source, re-opening\n");
		    pFatDS = CFatDataSource::Open(0, false);
		    if( !pFatDS ) {
			    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_ERROR, "Can't open fat data source, bailing\n");
			    return;
		    }
		    else
		    {

				pCDDS = (CCDDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( CD_DATA_SOURCE_CLASS_ID, 0 );
				
				if(pCDDS)
				{
					CDataSourceManager::GetInstance()->RemoveDataSource(pCDDS);
					CDataSourceManager::GetInstance()->AddDataSource( pFatDS );
					CDataSourceManager::GetInstance()->AddDataSource( pCDDS );
				}

		    }
	    }

        CUpdateManager* pUpdate = CUpdateManager::GetInstance();
        pUpdate->Configure( FileCopyCallback, ProgressCallback );
        update_t updates = pUpdate->CheckForUpdates( RESTORE_CD_CONFIG_PATH );

        if( updates == UM_UPDATE_UNAVAIL ) {
            // No updates found on cd/no restore file; error out
            DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_ERROR, "Restore config file parse error\n");
            return ;
        }

        if( m_bUpdateCDDB && (updates & UM_UPDATE_CDDBFULL) ) {
            SetActionText(LS(SID_UPDATING_CDDB));
            DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Installing CDDB\n");
            if( pUpdate->UpdateFromSource( RESTORE_CD_CONFIG_PATH, UM_UPDATE_CDDBFULL ) < 0 ) {
                m_bUpdateCDDBStatus = false;
            } else {
                m_bUpdateCDDBStatus = true;
            }
        }

        if( m_bUpdateSoftware ) {
            bool bStatus = true;
            if( (updates & UM_UPDATE_SYSTEM) ) {
                ResetProgressBar();
                SetActionText(LS(SID_UPDATING_SYSTEM_FILES));
                DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Updating system files\n");
                if( pUpdate->UpdateFromSource( RESTORE_CD_CONFIG_PATH, UM_UPDATE_SYSTEM ) < 0 ) {
                    bStatus = false;
                }
            }
            // -- Only update firmware if system file update worked
            if( bStatus && (updates & UM_UPDATE_FIRMWARE) ) {
                ResetProgressBar();
                SetActionText(LS(SID_UPDATING_FIRMWARE));
                SetMessageText(LS(SID_PLEASE_WAIT), CSystemMessageString::STATUS);
                DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Updating firmware\n");
                if( pUpdate->UpdateFromSource( RESTORE_CD_CONFIG_PATH, UM_UPDATE_FIRMWARE ) < 0 ) {
                    bStatus = false;
                }
            }
            m_bUpdateSoftwareStatus = bStatus;
            ResetProgressBar();
        }
    }
}

void CRestoreScreen::DoSummary()
{
    SetTitleText(LS(SID_FACTORY_RESTORE));
    if (IsRestoreSuccessful()) {
        SetActionText(LS(SID_RESTORE_SUCCEEDED));
        cyg_thread_delay(200);
        DoReset();
    }
    else {
        SetActionText(LS(SID_RESTORE_FAILED));
        if (m_iTryCount < 3) {
            SetMessageText(LS(SID_RETRYING), CSystemMessageString::STATUS);
            cyg_thread_delay(200);
            
			// FIXME: tg 1314 - a bad iso read can trigger this case
			// so chkdsk is not always appropriate

            // m_bFormat doesn't warrant a CheckDisk, but keep the code path
            // the same for simplicity's sake.
            DoChkdsk();
            
            DoSelectedOptions();
        }
        else {
            SetMessageText(LS(SID_CALL_TECH_SUPPORT), CSystemMessageString::STATUS);
        }
    }
}

void CRestoreScreen::DoReset(const TCHAR* MessageText)
{
    DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "\n *** Restarting the Device via Soft Reset ***\n\n");

    ResetProgressBar();
    
    SetMessageText(LS(SID_RESTARTING_PLEASE_WAIT), CSystemMessageString::STATUS);
    cyg_thread_delay(200);
    
    // quick and dirty reset
    DAIDisable();
    
    // disable interrupts, flush cache
    int oldints;
    HAL_DISABLE_INTERRUPTS(oldints);
    HAL_UCACHE_DISABLE();
    HAL_UCACHE_INVALIDATE_ALL();
    
    // Trigger POR
    *(volatile unsigned char*)PDDDR &= ~(0x01);
    *(volatile unsigned char*)PDDR  |= 0x01;
    
    // Soft reset for rev-02 boards
    void (*f)() = (void(*)())hal_vsr_table[0];
    f();
}

bool CRestoreScreen::IsRestoreSuccessful()
{
    bool Status = true;
    if (m_bFormat && !m_bFormatStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "Format");
        Status = false;
    }
    if (m_bCheckDisk && !m_bCheckDiskStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "CheckDisk");
        Status = false;
    }
    if (m_bUpdateCDDB && !m_bUpdateCDDBStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "UpdateCDDB");
        Status = false;
    }
    if (m_bUpdateSoftware && !m_bUpdateSoftwareStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "UpdateSoftware");
        Status = false;
    }
    if (m_bDeleteLocalContent && !m_bDeleteLocalContentStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "DeleteLocalContent");
        Status = false;
    }
    if (m_bClearCDCache && !m_bClearCDCacheStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "ClearCDCache");
        Status = false;
    }
    if (m_bClearLocalContentDatabase && !m_bClearLocalContentDatabase)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "ClearLocalContent");
        Status = false;
    }
    if (m_bClearSavedSettings && !m_bClearSavedSettingsStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "ClearSavedSettings");
        Status = false;
    }
    if (m_bResetCDDB && !m_bResetCDDBStatus)
    {
        DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_TRACE, "ResetCDDB");
        Status = false;
    }
    return Status;
}

#define MOUNT_TIMEOUT 50
bool
CRestoreScreen::DoCDCheck(CScreen* pScreen, const TCHAR* TitleText)
{
	if(GetRestoreOption(UPDATE_CDDB) || GetRestoreOption(UPDATE_SOFTWARE))
	{
		int  MountWait = 0;

		// test for restore cd here
		
        //		CMiniCDMgr* pCDDS = CMiniCDMgr::GetInstance();
        CCDDataSource* pCDDS = (CCDDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( CD_DATA_SOURCE_CLASS_ID, 0 );

        bool bUseAlertScreen = (this != Presentation()->GetCurrentThing());
        CAlertScreen* pAS = CAlertScreen::GetInstance();
        if (bUseAlertScreen) {
            pAS->Config(pScreen, AS_DEFAULT_TIMEOUT_LENGTH);
            pAS->SetTitleText(TitleText);
            pAS->SetActionText(LS(SID_CHECKING_FOR_RESTORE_CD));
            pAS->ResetProgressBar(0, MOUNT_TIMEOUT);
            pScreen->Add(pAS);
        }
        else {
            SetTitleText(TitleText);
            SetActionText(LS(SID_CHECKING_FOR_RESTORE_CD));
            ResetProgressBar(0, MOUNT_TIMEOUT);
        }
        
        if ((pCDDS->GetMediaStatus(false) != ENOERR) && MountWait < MOUNT_TIMEOUT)
        {
            while((pCDDS->GetMediaStatus(false) != ENOERR) && MountWait < MOUNT_TIMEOUT) 
            {
                DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_WARNING, "Failed to mount CD, try %d\n",MountWait);
                cyg_thread_delay( 10 );
                MountWait++;
                if (bUseAlertScreen)
                    pAS->UpdateProgressBar(MountWait);
                else
                    UpdateProgressBar(MountWait);
            }
        }

        // bypass the data source so we can loop on this a bit
		while((mount(CD_DEVICE_NAME, CD_MOUNT_DIR, "cd9660") < 0) && MountWait < MOUNT_TIMEOUT)
		{
            cyg_thread_delay( 10 );
			MountWait++;
            if (bUseAlertScreen)
                pAS->UpdateProgressBar(MountWait);
            else
                UpdateProgressBar(MountWait);
		}

        //		CIsoFileInputStream ifis;

        if( MountWait         == MOUNT_TIMEOUT ||
            UM_UPDATE_UNAVAIL == CUpdateManager::GetInstance()->CheckForUpdates( RESTORE_CD_CONFIG_PATH ) )
		{
            if (bUseAlertScreen) {
                pAS->ResetProgressBar();
                pAS->SetActionText(LS(SID_PLEASE_INSERT_RESTORE_CD));
                pAS->SetMessageText(LS(SID_NO_RESTORE_CD_FOUND));
            }
            else {
                ResetProgressBar();
                SetActionText(LS(SID_PLEASE_INSERT_RESTORE_CD));
                SetMessageText(LS(SID_NO_RESTORE_CD_FOUND));
            }
			DEBUGP( DBG_RESTORE_SCREEN, DBGLEV_INFO, "Restore CD Not Found\n");
			umount("/");
            return false;
		}
        if (bUseAlertScreen)
            pAS->UpdateProgressBar(MOUNT_TIMEOUT);
        else
            UpdateProgressBar(MOUNT_TIMEOUT);

        if (bUseAlertScreen)
            pAS->HideScreen();
		return true;
	}

	return true;
}

void
CRestoreScreen::NotifyMediaInserted()
{
    if (m_bRestoreCDDB)
    {
        if (DoCDCheck(this, LS(SID_RESTORING_CDDB)))
        {
//            SetMessageText(LS(SID_EMPTY_STRING), CSystemMessageString::STATUS);
//            SetMessageText(LS(SID_EMPTY_STRING));
            DoSelectedOptions();
        }
    }
}

void
CRestoreScreen::DoRestoreCDDB()
{
    m_bRestoreCDDB = true;
    SetRestoreOption(UPDATE_CDDB, true);

    SetTitleText(LS(SID_RESTORE_PROCEDURE));
    SetActionText(LS(SID_CHECKING_FOR_RESTORE_CD));
    SetMessageText("", CSystemMessageString::STATUS);
    ResetProgressBar();

    put_event(EVENT_RESTORE_CDDB, 0);
}
