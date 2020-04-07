//
// DJPlayerState.cpp
//
// Copyright (c) 1998 - 2002 Fullplay Media (TM). All rights reserved
//

#include <main/main/DJPlayerState.h>
#include <codec/codecmanager/CodecManager.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/cddatasource/CDDataSource.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/lineindatasource/LineInDataSource.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <devs/lcd/lcd.h>
#include <devs/audio/dai.h>
#include <extras/idlecoder/IdleCoder.h>
#include <main/playlist/djplaylist/DJPlaylist.h>
#include <playlist/plformat/manager/PlaylistFormatManager.h>

#include <main/main/AppSettings.h>
#include <main/main/DJEvents.h>
#include <main/main/DJHelper.h>
#include <main/main/LEDStateManager.h>
#include <main/main/Recording.h>
#include <main/main/SpaceMgr.h>
#include <core/events/SystemEvents.h>

#include <main/ui/common/UserInterface.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/EditScreen.h>
#include <main/ui/BitrateMenuScreen.h>
#include <main/ui/QuickBrowseMenuScreen.h>

#include <util/eventq/EventQueueAPI.h>
#include <util/registry/Registry.h>
#include <util/timer/Timer.h>
#include <util/utils/utils.h>

#include <main/testharness/testharness.h>
#include <main/webcontrol/webcontrol.h>
#include <_version.h>

#include <network.h>
#include <io/net/Net.h>

#include <fs/fat/sdapi.h>
#include <main/main/FatHelper.h>
#include <cyg/hal/hal_cache.h>   // SafeReset - cache disable
#include <cyg/hal/hal_edb7xxx.h> // SafeReset - POR

#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
#include <main/content/datasourcecontentmanager/DataSourceContentManager.h>
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
#include <main/content/djcontentmanager/DJContentManager.h>
#endif
#include <main/content/simplercontentmanager/SimplerContentManager.h>

#ifndef DISABLE_VOLUME_CONTROL
#include <io/audio/VolumeControl.h>
#endif // DISABLE_VOLUME_CONTROL

#ifdef LINE_RECORDER_ENABLED
#include <main/main/LineRecorder.h>
#endif

#include <cyg/kernel/kapi.h>    // cyg_current_time
#include <stdio.h>  // sprintf
#include <stdlib.h> // malloc, free

#define DEFAULT_ENCODE_BITRATE 192

// this should match the #define in main.cpp
//#define ENABLE_PARTIAL_BOOT

#include <util/debug/debug.h>
DEBUG_MODULE_S( DJPS, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( DJPS );

// Construct registry keys.
static const RegKey DJPlayerStateRegKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_CD_SESSION_INDEX_KEY_NAME );
static const RegKey DJPlaylistCounterRegKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_PLAYLIST_COUNTER_KEY_NAME );
static const RegKey DJPlayerEncodeBitrateRegKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_ENCODE_BITRATE_KEY_NAME );
static const RegKey DJPlayerUIViewModeRegKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_UI_VIEW_MODE_KEY_NAME );
static const RegKey DJPlayerRandSeedRegKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_RANDOM_NUMBER_SEED_KEY_NAME );
static const RegKey DJPlayerCurrentDSKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_RANDOM_NUMBER_SEED_KEY_NAME );
static const RegKey DJPlayerPlaylistIndexKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_PLAYLIST_INDEX_KEY_NAME );
static const RegKey DJPlayerPlaylistTitleKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_PLAYLIST_TITLE_KEY_NAME );
static const RegKey DJPlayerEjectCDAfterRipKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_EJECT_CD_AFTER_RIP_NAME );
static const RegKey DJPlayerEnableExtCharsKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_ENABLE_EXT_CHARS_NAME );
static const RegKey DJPlayerEnableWebControlKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_ENABLE_WEB_CONTROL_NAME );
static const RegKey DJPlayerShowTrackNumInTitleKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_SHOW_TRACK_NUM_IN_TITLE_NAME );
static const RegKey DJPlayerShowAlbumWithArtistKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_SHOW_ALBUM_WITH_ARTIST_NAME );
static const RegKey DJPlayerTextScrollSpeedKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_TEXT_SCROLL_SPEED_NAME );
static const RegKey DJPlayerLEDBrightnessKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_LED_BRIGHTNESS_NAME );
static const RegKey DJPlayerLCDBrightnessKey = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_LCD_BRIGHTNESS_NAME );
static const RegKey DJPlayerPlayCDWhenInserted = REGKEY_CREATE( DJ_PLAYER_STATE_REGISTRY_KEY_TYPE, DJ_PLAYER_STATE_PLAY_CD_WHEN_INSERTED );

// The global singleton DJ state.
CDJPlayerState* CDJPlayerState::s_pSingleton = 0;

// Returns a pointer to the global DJ state instance.
CDJPlayerState*
CDJPlayerState::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CDJPlayerState;
    return s_pSingleton;
}

// Destroy the singleton global DJ state instance.
void
CDJPlayerState::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

CDJPlayerState::CDJPlayerState()
    : m_pCDDS(0),
    m_pFatDS(0),
    m_pNetDS(0),
    m_pCurrentIML(0),
    m_szIMLUDN(0),
    m_eSource(HD),
    m_ePowerState(POWER_ON),
    m_bScanningCD(false),
    m_eCDState(NONE),
    m_bScanningHD(false),
    m_bSystemIsShuttingDown(false),
    m_eUIViewMode(NORMAL),
    m_eEncodingState(NOT_ENCODING),
    m_iCDSessionIndex(0),
    m_iEncodeBitrate(DEFAULT_ENCODE_BITRATE),
    m_uiPlaylistCounter(1),
    m_uiPlaylistIndex(0),
    m_bUIEjectCDAfterRip(false), 
    m_bUIEnableExtChars(false), 
    m_bUIEnableWebControl(false), 
    m_bUIShowTrackNumInTitle(true), 
    m_bUIShowAlbumWithArtist(false),
    m_bUIPlayCDWhenInserted(false),
    m_eUITextScrollSpeed(SLOW),
    m_bUIEnableRecordLED(true),
    m_eUILCDBrightness(BRIGHT)
{
    m_pPlayManager = CPlayManager::GetInstance();
    m_pUserInterface = NULL;
}

CDJPlayerState::~CDJPlayerState()
{
    suspend_timer( m_TimerHandle );
    unregister_timer( m_TimerHandle );
    suspend_timer( m_SystemShutdownTimerHandle );
    unregister_timer( m_SystemShutdownTimerHandle );

    delete [] m_szIMLUDN;
}

//! If the current source is an FML and it sends a byebye then store the FML's UDN so
//! we can make it the current source again if it comes back.
void
CDJPlayerState::SetIMLToRestore(const char* szUDN)
{
    delete [] m_szIMLUDN;
    m_szIMLUDN = strdup_new(szUDN);
}

//! If the user manually changes source while the FML is gone, then forget about it.
void
CDJPlayerState::ClearIMLToRestore()
{
    delete [] m_szIMLUDN;
    m_szIMLUDN = 0;
}

//! Save player state.
void
CDJPlayerState::SaveState(bool bSaveRegistry, bool bSaveCurrentPlaylist, bool bSaveContentDB)
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: Saving settings\n");

    if (bSaveRegistry)
    {
        //
        // Sync the registry clients with the registry
        //
#ifndef DISABLE_VOLUME_CONTROL
        CVolumeControl::GetInstance()->SaveToRegistry();
#endif // DISABLE_VOLUME_CONTROL
        CPlayerScreen::GetPlayerScreen()->SaveToRegistry();

        //
        // Save the DJ state variables to the registry
        //
        CRegistry::GetInstance()->RemoveItem(DJPlayerStateRegKey);
        CRegistry::GetInstance()->AddItem( DJPlayerStateRegKey, (void*)m_iCDSessionIndex, REGFLAG_PERSISTENT, sizeof(m_iCDSessionIndex) );

        CRegistry::GetInstance()->RemoveItem(DJPlaylistCounterRegKey);
        CRegistry::GetInstance()->AddItem( DJPlaylistCounterRegKey, (void*)m_uiPlaylistCounter, REGFLAG_PERSISTENT, sizeof(m_uiPlaylistCounter) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerEncodeBitrateRegKey);
        CRegistry::GetInstance()->AddItem( DJPlayerEncodeBitrateRegKey, (void*)m_iEncodeBitrate, REGFLAG_PERSISTENT, sizeof(m_iEncodeBitrate) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerUIViewModeRegKey);
        CRegistry::GetInstance()->AddItem( DJPlayerUIViewModeRegKey, (void*)m_eUIViewMode, REGFLAG_PERSISTENT, sizeof(m_eUIViewMode) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerRandSeedRegKey);
        CRegistry::GetInstance()->AddItem( DJPlayerRandSeedRegKey, (void*)rand(), REGFLAG_PERSISTENT, sizeof(int) );

        //
        // Save the User Customizable Settings
        //
        // NOTE: We store these bool values as integers in the registry due to a limitation of the registry
        CRegistry::GetInstance()->RemoveItem(DJPlayerEjectCDAfterRipKey);
        CRegistry::GetInstance()->AddItem( DJPlayerEjectCDAfterRipKey, (void*)m_bUIEjectCDAfterRip, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerEnableExtCharsKey);
        CRegistry::GetInstance()->AddItem( DJPlayerEnableExtCharsKey, (void*)m_bUIEnableExtChars, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerEnableWebControlKey);
        CRegistry::GetInstance()->AddItem( DJPlayerEnableWebControlKey, (void*)m_bUIEnableWebControl, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerShowTrackNumInTitleKey);
        CRegistry::GetInstance()->AddItem( DJPlayerShowTrackNumInTitleKey, (void*)m_bUIShowTrackNumInTitle, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerShowAlbumWithArtistKey);
        CRegistry::GetInstance()->AddItem( DJPlayerShowAlbumWithArtistKey, (void*)m_bUIShowAlbumWithArtist, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerPlayCDWhenInserted);
        CRegistry::GetInstance()->AddItem( DJPlayerPlayCDWhenInserted, (void*)m_bUIPlayCDWhenInserted, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerTextScrollSpeedKey);
        CRegistry::GetInstance()->AddItem( DJPlayerTextScrollSpeedKey, (void*)m_eUITextScrollSpeed, REGFLAG_PERSISTENT, sizeof(EUITextScrollSpeed) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerLEDBrightnessKey);
        CRegistry::GetInstance()->AddItem( DJPlayerLEDBrightnessKey, (void*)m_bUIEnableRecordLED, REGFLAG_PERSISTENT, sizeof(int) );

        CRegistry::GetInstance()->RemoveItem(DJPlayerLCDBrightnessKey);
        CRegistry::GetInstance()->AddItem( DJPlayerLCDBrightnessKey, (void*)m_eUILCDBrightness, REGFLAG_PERSISTENT, sizeof(EUIBrightness) );
    }

    if (bSaveCurrentPlaylist)
    {
        // If the data source is CD or HD, then save the current playlist
        if ((m_eSource == CD) || (m_eSource == HD) || (m_eSource == FML))
            SaveCurrentPlaylistInternal();
        else
        {
            // Otherwise erase the playlist.
            pc_unlink(const_cast<char*>(FilenameFromURLInPlace(CURRENT_PLAYLIST_URL)));
            CRegistry::GetInstance()->RemoveItem(DJPlayerPlaylistIndexKey);
            CRegistry::GetInstance()->RemoveItem(DJPlayerPlaylistTitleKey);

            // Remember the data source.
            // This is usually handled by SavePlaylistInternal.
            CRegistry::GetInstance()->RemoveItem(DJPlayerCurrentDSKey);
            CRegistry::GetInstance()->AddItem( DJPlayerCurrentDSKey, (void*)m_eSource, REGFLAG_PERSISTENT, sizeof(int) );
            // Emergent behavior: an unset key has value 0, which in the DS enum is the CD, which is what we default to.
        }
    }

    if (bSaveRegistry || bSaveCurrentPlaylist)
    {
        //
        // Save the registry to disk
        //
        SaveRegistry();
    }

    if (bSaveContentDB)
    {
        //
        // Save the content manager DB.
        //
#ifdef DDOMOD_CONTENT_DATASOURCECONTENTMANAGER
        ((CDataSourceContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
#elif defined(DDOMOD_CONTENT_DJCONTENTMANAGER)
        ((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->Commit();
#endif
    }
}

//! Load player state.
void
CDJPlayerState::LoadState()
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: Loading settings\n");

    //
    // Load the registry from disk
    //
    CFatFileInputStream file;
    file.Open(SAVE_SETTINGS_PATH);
    CRegistry::GetInstance()->RestoreState( &file );
    file.Close();
}

void
CDJPlayerState::Initialize()
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: Initializing\n");
    //
    // Restore the DJ state variables from the registry
    //
    m_iCDSessionIndex = (int)CRegistry::GetInstance()->FindByKey( DJPlayerStateRegKey );
    if (!m_iCDSessionIndex || (m_iCDSessionIndex > 1000))
    {
        m_iCDSessionIndex = 1;
    }

	m_uiPlaylistCounter = (unsigned int)CRegistry::GetInstance()->FindByKey( DJPlaylistCounterRegKey );

    int iBitrate;
    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerEncodeBitrateRegKey, (void**)&iBitrate) ))
    {
        DEBUGP( DJPS, DBGLEV_INFO, "djps: Bitrate: %d\n", iBitrate);
        SetEncodeBitrate(iBitrate, false);
    }

    int iRandSeed;
    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerRandSeedRegKey, (void**)&iRandSeed) ))
    {
        DEBUGP( DJPS, DBGLEV_INFO, "djps: Random seed: %d\n", iRandSeed);
        srand(iRandSeed);
    }
    else
    {
        DEBUGP( DJPS, DBGLEV_INFO, "djps: No random seed in registry, so using tick count\n");
        srand(cyg_current_time());
    }

    m_uiPlaylistIndex = (unsigned int)CRegistry::GetInstance()->FindByKey( DJPlayerPlaylistIndexKey );
    ESourceMode eSource = (ESourceMode)(int)CRegistry::GetInstance()->FindByKey( DJPlayerCurrentDSKey );
    // Emergent behavior: an unset key has value 0, which in the DS enum is the CD, which is what we default to.  God is great.

    //
    // Sync the registry clients with the registry
    //
    CIdleCoder::GetInstance()->Halt();
    CIdleCoder::GetInstance()->LoadFromRegistry();
    CRecordingManager::GetInstance()->LoadEncodingUpdateList();
    CPlayerScreen::GetPlayerScreen()->RestoreFromRegistry();
    // Rudeness: to dodge timing considerations at startup, tell the bitrate menuscreen what the bitrate is
    // Extra rudeness: tell the bitratemenuscreen it's a bitratemenuscreen!
    ((CBitrateMenuScreen*)CBitrateMenuScreen::GetBitrateMenuScreen())->SetBitrate( GetEncodeBitrate() );
#ifndef DISABLE_VOLUME_CONTROL
    CVolumeControl::GetInstance()->RestoreFromRegistry();
#endif // DISABLE_VOLUME_CONTROL


    DEBUGP( DJPS, DBGLEV_INFO, "djps: set the ui view mode\n");
    SetUIViewMode((EUIViewMode)(int)CRegistry::GetInstance()->FindByKey( DJPlayerUIViewModeRegKey ));

    // If the data source is HD or FML, then set the source as HD.
    if ((eSource == HD) || (eSource == FML))
    {
        SetSource(HD);
    }
    else
    {
        // Otherwise, set the source.
        SetSource(eSource);
    }

    //
    // Load some of the user defined customizations
    //
    // NOTE: We store these bool values as integers in the registry due to a limitation of the registry
    //       So, we need to pull them out as integers, then cast them back down as bool
    int iBool;
    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerEjectCDAfterRipKey, (void**)&iBool) ))
        SetUIEjectCDAfterRip((bool)iBool);
    else
        SetUIEjectCDAfterRip(m_bUIEjectCDAfterRip);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerEnableExtCharsKey, (void**)&iBool) ))
        SetUIEnableExtChars((bool)iBool);
    else
        SetUIEnableExtChars(m_bUIEnableExtChars);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerEnableWebControlKey, (void**)&iBool) ))
        SetUIEnableWebControl((bool)iBool);
    else
        SetUIEnableWebControl(m_bUIEnableWebControl);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerShowTrackNumInTitleKey, (void**)&iBool) ))
        SetUIShowTrackNumInTitle((bool)iBool);
    else
        SetUIShowTrackNumInTitle(m_bUIShowTrackNumInTitle);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerShowAlbumWithArtistKey, (void**)&iBool) ))
        SetUIShowAlbumWithArtist((bool)iBool);
    else
        SetUIShowAlbumWithArtist(m_bUIShowAlbumWithArtist);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerPlayCDWhenInserted, (void**)&iBool) ))
        SetUIPlayCDWhenInserted((bool)iBool);
    else
        SetUIPlayCDWhenInserted(m_bUIPlayCDWhenInserted);

    EUITextScrollSpeed eTextScrollSpeed;
    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerTextScrollSpeedKey, (void**)&eTextScrollSpeed) ))
        SetUITextScrollSpeed(eTextScrollSpeed);
    else
        SetUITextScrollSpeed(m_eUITextScrollSpeed);

    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerLEDBrightnessKey, (void**)&iBool) ))
        SetUIEnableRecordLED((bool)iBool);
    else
        SetUIEnableRecordLED(m_bUIEnableRecordLED);

    EUIBrightness eBrightness;
    if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerLCDBrightnessKey, (void**)&eBrightness) ))
        SetUILCDBrightness(eBrightness);
    else
        SetUILCDBrightness(m_eUILCDBrightness);

    //
    // Establish our idle timer (const from appsettings.h)
    //
    register_timer_persist( (pTimerFunc)(CDJPlayerState::IdleTimerCallback), (void*)this,
                            TIMER_MILLISECONDS(IDLE_TIME_LIMIT), -1, &m_TimerHandle );
    resume_timer( m_TimerHandle );
    
    //
    // Establish our shutdown timer (const from appsettings.h), but don't start it
    //
    register_timer_persist( (pTimerFunc)(CDJPlayerState::ShutdownSystemTimerCallback), (void*)this,
                            TIMER_MILLISECONDS(SHUTDOWN_WAIT_TIME), -1, &m_SystemShutdownTimerHandle );
    DEBUGP( DJPS, DBGLEV_INFO, "djps: done loading settings\n");
}

//! Save the registry.
void
CDJPlayerState::SaveRegistry()
{
    CFatFileOutputStream file;
    file.Open(SAVE_SETTINGS_PATH);
    CRegistry::GetInstance()->SaveState( &file );
    file.Close();
}

//! Save the current playlist.
//! Saves the registry as a side effect.
ERESULT
CDJPlayerState::SaveCurrentPlaylist()
{
    ERESULT err = SaveCurrentPlaylistInternal();
    if (SUCCEEDED(err))
    {
        SaveRegistry();
    }

    return err;
}

//! Save the current playlist without saving the registry.
ERESULT
CDJPlayerState::SaveCurrentPlaylistInternal()
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: Saving the current playlist to %s\n", CURRENT_PLAYLIST_URL);

    CDJPlaylist* pPlaylist = (CDJPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    const int iFormat = CPlaylistFormatManager::GetInstance()->FindPlaylistFormat("djp");

    // If the playlist is dirty then save it, otherwise use the copy.
    ERESULT err = MAKE_ERESULT(SEVERITY_SUCCESS, 0, 0);

    if (!pPlaylist->IsDirty())
    {
        // If the current playlist file already exists, then assume it's this playlist.
        // This is a reasonable assumption, since the only way that file can exist is if
        // we've been through this procedure before (i.e., moved the temp file back to the
        // current playlist file) or if the pc_unlink or pc_mv command in LoadCurrentPlaylist
        // failed (in which case we can do nothing but suggest a chkdsk).
        if (!FileExists(FullFilenameFromURLInPlace(CURRENT_PLAYLIST_URL)) &&
            !pc_mv(CURRENT_PLAYLIST_TEMP_FILE,
                const_cast<char*>(FullFilenameFromURLInPlace(CURRENT_PLAYLIST_URL))))
        {
            DEBUGP( DJPS, DBGLEV_INFO, "djps: Unable to reuse previous current playlist %s\n", CURRENT_PLAYLIST_TEMP_FILE);
            pPlaylist->SetDirty(true);
        }
    }

    // The playlist is either dirty or we can't locate the previous clean playlist file, so
    // write a new playlist file.
    if (pPlaylist->IsDirty())
    {
        err = CPlaylistFormatManager::GetInstance()->SavePlaylist(iFormat, CURRENT_PLAYLIST_URL, CPlayManager::GetInstance()->GetPlaylist());
        if (SUCCEEDED(err))
            pPlaylist->SetDirty(false);
        else
            DEBUGP( DJPS, DBGLEV_INFO, "djps: Unable to save current playlist %s: %x\n", CURRENT_PLAYLIST_URL, err);
    }

    if (SUCCEEDED(err))
    {
        // Find the index of the current entry and save it to the registry.
        IPlaylistEntry* pCurrentEntry = pPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
            m_uiPlaylistIndex = pCurrentEntry->GetIndex();
        else
            m_uiPlaylistIndex = 0;

        DEBUGP( DJPS, DBGLEV_INFO, "djps: Current playlist index %d\n", m_uiPlaylistIndex);

        CRegistry::GetInstance()->RemoveItem(DJPlayerPlaylistIndexKey);
        CRegistry::GetInstance()->AddItem( DJPlayerPlaylistIndexKey, (void*)m_uiPlaylistIndex, REGFLAG_PERSISTENT, sizeof(unsigned int) );

        // Save the title of the playlist
        TCHAR* pszPlaylistTitle = CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->GetCurrentPlaylistTitle();
        CRegistry::GetInstance()->RemoveItem(DJPlayerPlaylistTitleKey);
        CRegistry::GetInstance()->AddItem( DJPlayerPlaylistTitleKey, (void*)tstrdup(pszPlaylistTitle), REGFLAG_PERSISTENT, (tstrlen(pszPlaylistTitle) + 1) * sizeof(TCHAR));

        // Remember the data source.
        CRegistry::GetInstance()->RemoveItem(DJPlayerCurrentDSKey);
        CRegistry::GetInstance()->AddItem( DJPlayerCurrentDSKey, (void*)m_eSource, REGFLAG_PERSISTENT, sizeof(int) );
    }

    return err;
}

IMediaContentRecord*
CDJPlayerState::GetMediaContentRecord(media_record_info_t* pMediaRecord)
{
    IMediaContentRecord* pRecord = 0;
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();

    // Don't verify records that come from the net, since they can't be verified.
    if (pMediaRecord->iDataSourceID == CDJPlayerState::GetInstance()->GetNetDataSource()->GetInstanceID())
    {
        pRecord = CSimplerContentManager::GetInstance()->AddMediaRecord(*pMediaRecord);
    }
    else
    {
        // This is a local track, so check for a local match.
        pRecord = pCM->GetMediaRecord(pMediaRecord->szURL);
        if (!pRecord)
        {
            // Now for the tricky shit.  Maybe we can't find this file because it was
            // a .raw file when the playlist was created and now it's a happily
            // encoded .mp3 track.  Or maybe it was a .mp3 file and the user re-ripped
            // it to .raw form.
            // Flip the file extension and run another check to be sure.
            int len = strlen(pMediaRecord->szURL);
            if (len > 4)
            {
                // An extra hack for speed: the only file format we handle with an
                // extension that ends in '3' is mp3.  So instead of checking for
                // ".mp3", just check the 3.
                if (pMediaRecord->szURL[len - 1] == '3')
                {
                    strcpy(pMediaRecord->szURL + len - 3, "raw");
                    pRecord = pCM->GetMediaRecord(pMediaRecord->szURL);
                }
                else if ((pMediaRecord->szURL[len - 1] == 'w') || (pMediaRecord->szURL[len - 1] == 'W'))
                {
                    strcpy(pMediaRecord->szURL + len - 3, "mp3");
                    pRecord = pCM->GetMediaRecord(pMediaRecord->szURL);
                }
            }
        }
    }

    return pRecord;

}

//! Load the current playlist.
ERESULT
CDJPlayerState::LoadCurrentPlaylist()
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: Loading the current playlist from %s\n", CURRENT_PLAYLIST_URL);

    const int iFormat = CPlaylistFormatManager::GetInstance()->FindPlaylistFormat("djp");
    CDJPlaylist* pPlaylist = (CDJPlaylist*)CPlayManager::GetInstance()->GetPlaylist();
    pPlaylist->Clear();
    IMediaRecordInfoVector records;
    ERESULT err = CPlaylistFormatManager::GetInstance()->LoadPlaylist(iFormat, CURRENT_PLAYLIST_URL, records, 0);

    // Rename this playlist to a temporary filename so it won't be picked up on a scan if we crash
    // (just in case something in the playlist causes us to crash and we enter into a load/crash/reboot loop).
    pc_unlink(CURRENT_PLAYLIST_TEMP_FILE);
    pc_mv(const_cast<char*>(FullFilenameFromURLInPlace(CURRENT_PLAYLIST_URL)),
        CURRENT_PLAYLIST_TEMP_FILE);

    if (SUCCEEDED(err))
    {
        MediaRecordList mrlTracks;
        for (int i = 0; i < records.Size(); ++i)
        {
            // Call the lookup function provided by the caller to find matching IMediaContentRecord pointers.
            IMediaContentRecord* pRecord = GetMediaContentRecord(&(records[i]));
            if (pRecord)
                mrlTracks.PushBack(pRecord);
            free(records[i].szURL);
        }

        // Add the records to the given playlist.
        if (!mrlTracks.IsEmpty())
            pPlaylist->AddEntries(mrlTracks);

        if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(m_uiPlaylistIndex))
            pPlaylist->SetCurrentEntry(pEntry);

        // try to load the playlist's title from the registry
        TCHAR* pszPlaylistTitle;
        if (SUCCEEDED(CRegistry::GetInstance()->FindByKey( DJPlayerPlaylistTitleKey, (void**)&pszPlaylistTitle )) && pszPlaylistTitle)
            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistTitle(pszPlaylistTitle);
        else
            CQuickBrowseMenuScreen::GetQuickBrowseMenuScreen()->SetCurrentPlaylistAsDefault();
    }

    pPlaylist->SetDirty(false);

    if (pPlaylist->IsEmpty())
        m_pUserInterface->NotifyPlaylistCleared();

    return err;
}

//! Set the input source state.
//! Returns true if the playlist is cleared during transition.
bool
CDJPlayerState::SetSource(ESourceMode eSource, bool bSendSourceChangedEvent, bool bSendPlaylistClearedEvent)
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: %s -> %s\n",
        m_eSource == CD ? "CD" : (m_eSource == HD ? "HD" : (m_eSource == FML ? "FML" : "LINE IN")),
        eSource == CD ? "CD" : (eSource == HD ? "HD" : (eSource == FML ? "FML" : "LINE IN")));

    bool bRetVal = false;
    IPlaylist* pPlaylist = m_pPlayManager->GetPlaylist();

    // If the source is being switched then forget about trying to restore an resynching fml.
    ClearIMLToRestore();

    switch (m_eSource)
    {
        case CD:
        {
            if (eSource != CD)
            {
                // Stop ripping/recording on transitions between HD-FML/CD/Line-in
                if (CRecordingManager::GetInstance()->IsRipping())
                    CRecordingManager::GetInstance()->StopRipping();
                else if (CRecordingManager::GetInstance()->IsRecording())
                    CRecordingManager::GetInstance()->StopRecording();

                m_pPlayManager->Deconfigure();
                if( pPlaylist )
                    pPlaylist->Clear();

                // Clear the fml records so that more can be added.
                CSimplerContentManager::GetInstance()->Clear();
                bRetVal = true;
            } else {
                // Fun case of switching from CD to CD - make sure we have a decent playlist
                if( pPlaylist->GetSize() == 0 ) {
                    CreateCDPlaylist(false);
                    bRetVal = true;
                }
            }
            break;
        }

        case HD:
            if (eSource == CD)
            {
                // Stop ripping/recording on transitions between HD-FML/CD/Line-in
                if (CRecordingManager::GetInstance()->IsRipping())
                    CRecordingManager::GetInstance()->StopRipping();
                else if (CRecordingManager::GetInstance()->IsRecording())
                    CRecordingManager::GetInstance()->StopRecording();

                m_pPlayManager->Deconfigure();
                CPlayerScreen::GetPlayerScreen()->ClearTrack();

                CreateCDPlaylist(false);
                bRetVal = true;
            }
            else if (eSource == LINE_IN)
            {
                // Stop ripping/recording on transitions between HD-FML/CD/Line-in
                if (CRecordingManager::GetInstance()->IsRipping())
                    CRecordingManager::GetInstance()->StopRipping();
                else if (CRecordingManager::GetInstance()->IsRecording())
                    CRecordingManager::GetInstance()->StopRecording();

                // Switching to line in should always clear the playlist.
                m_pPlayManager->Deconfigure();
                if( pPlaylist )
                    pPlaylist->Clear();
                bRetVal = true;
            }
            break;

        case FML:
            if (eSource == CD)
            {
                // Stop ripping/recording on transitions between HD-FML/CD/Line-in
                if (CRecordingManager::GetInstance()->IsRipping())
                    CRecordingManager::GetInstance()->StopRipping();
                else if (CRecordingManager::GetInstance()->IsRecording())
                    CRecordingManager::GetInstance()->StopRecording();

                m_pPlayManager->Deconfigure();
                CPlayerScreen::GetPlayerScreen()->ClearTrack();

                CreateCDPlaylist(false);
                bRetVal = true;
            }
            else if (eSource == LINE_IN)
            {
                // Stop ripping/recording on transitions between HD-FML/CD/Line-in
                if (CRecordingManager::GetInstance()->IsRipping())
                    CRecordingManager::GetInstance()->StopRipping();
                else if (CRecordingManager::GetInstance()->IsRecording())
                    CRecordingManager::GetInstance()->StopRecording();

                // Switching to line in should always clear the playlist.
                m_pPlayManager->Deconfigure();
                if( pPlaylist )
                    pPlaylist->Clear();
                bRetVal = true;
            }
            break;

        case LINE_IN:
            // Stop ripping/recording on transitions between HD-FML/CD/Line-in
            if (CRecordingManager::GetInstance()->IsRipping())
                CRecordingManager::GetInstance()->StopRipping();
            else if (CRecordingManager::GetInstance()->IsRecording())
                CRecordingManager::GetInstance()->StopRecording();

            // Switching from line in should always clear the playlist.
            m_pPlayManager->Deconfigure();
            CPlayerScreen::GetPlayerScreen()->ClearTrack();

            if (eSource == CD)
            {
                CreateCDPlaylist(false);
            }
            else
            {
                if( pPlaylist )
                    pPlaylist->Clear();
            }
            bRetVal = true;
            break;
    };
    m_eSource = eSource;

    // post a message to the ui that the source has changed
    if (bSendSourceChangedEvent && m_pUserInterface)
        m_pUserInterface->NotifySourceChanged(m_eSource);

    // post a message to the ui that the playlist was cleared
    if (bSendPlaylistClearedEvent && bRetVal && m_pUserInterface)
        m_pUserInterface->NotifyPlaylistCleared();

    return bRetVal;
}


//! Get the current playlist entry source
CDJPlayerState::ESourceMode
CDJPlayerState::GetCurrentSongSource()
{
    // if we can't find a current source, then default to our current browse source
    ESourceMode eSource = m_eSource;

    IPlaylist *pPlaylist = m_pPlayManager->GetPlaylist();
    if (pPlaylist)
    {
        IPlaylistEntry *pCurrentEntry = pPlaylist->GetCurrentEntry();
        if (pCurrentEntry)
        {
            IMediaContentRecord* pContentRecord = pCurrentEntry->GetContentRecord();
            if (pContentRecord)
            {
                IDataSource* pDataSource = CDataSourceManager::GetInstance()->GetDataSourceByID(pContentRecord->GetDataSourceID());
                if (pDataSource)
                {
                    switch (pDataSource->GetClassID())
                    {
                    case CD_DATA_SOURCE_CLASS_ID:
                        eSource = CD;
                        break;
                    case FAT_DATA_SOURCE_CLASS_ID:
                        eSource = HD;
                        break;
                    case LINE_IN_DATA_SOURCE_CLASS_ID:
                        eSource = LINE_IN;
                        break;
                    case NET_DATA_SOURCE_CLASS_ID:
                        eSource = FML;
                        break;
                    default:
                        eSource = m_eSource;
                        break;
                    }
                }
                else
                    DEBUGP( DJPS, DBGLEV_WARNING, "djps:GetCurrentSongSource FAILED: No DataSource\n");
            }
            else
                DEBUGP( DJPS, DBGLEV_WARNING, "djps:GetCurrentSongSource FAILED: No ConentRecord\n");
        }
        else
            DEBUGP( DJPS, DBGLEV_WARNING, "djps:GetCurrentSongSource FAILED: No CurrentEntry\n");
    }
    else
        DEBUGP( DJPS, DBGLEV_WARNING, "djps:GetCurrentSongSource FAILED: No Playlist\n");

    DEBUGP( DJPS, DBGLEV_TRACE, "djps: current song datasource is %s\n",
        eSource == CD ? "CD" : (eSource == HD ? "HD" : (eSource == FML ? "FML" : "LINE IN")));
    return eSource;
}


//! Clear the playlist, fill it with all the content on the CD, and set the first track.
//! Returns true if a track was set in the media player, false otherwise.
bool
CDJPlayerState::CreateCDPlaylist(bool bSendPlaylistClearedEvent)
{
    IPlaylist* pPlaylist = m_pPlayManager->GetPlaylist();
    if (!pPlaylist)
        return false;

    // Stop playback, clear the playlist, and build a new playlist of CD content.
    m_pPlayManager->Deconfigure();
    pPlaylist->Clear();

    MediaRecordList records;
    ((IQueryableContentManager*)m_pPlayManager->GetContentManager())->GetMediaRecordsByDataSourceID(records, m_pCDDS->GetInstanceID());

    bool bPlaylistCreated = true;
    if (pPlaylist)
    {
        pPlaylist->AddEntries(records);
        pPlaylist->SetCurrentEntry(pPlaylist->GetEntry(0, m_pPlayManager->GetPlaylistMode()));
        if (FAILED(DJSetCurrentOrNext()))
        {
            // There are no playable tracks on this CD.
            bPlaylistCreated = false;
        }
    }

    if (bSendPlaylistClearedEvent)
        m_pUserInterface->NotifyPlaylistCleared(!bPlaylistCreated);

    return bPlaylistCreated;
}


// return int, increment counter
unsigned int
CDJPlayerState::GetUniquePlaylistID()
{

	unsigned int uiRet = m_uiPlaylistCounter;
	
	m_uiPlaylistCounter++;

	return uiRet;

}

void CDJPlayerState::SetDrivesPower(bool on)
{
    DEBUGP( DJPS, DBGLEV_TRACE, "djps: SetDrivesPower %s\n", on ? "ON" : "OFF");

	if(on)
	{
        CCDDataSource::WakeupDrive("/dev/cda/");
        CFatDataSource::WakeupDrive("/dev/hda/");
	}
	else
	{
        CFatDataSource::SuspendDrive("/dev/hda/");
        CCDDataSource::SuspendDrive("/dev/cda/");
	}

}


void
CDJPlayerState::SetPowerState(EPowerState ePowerState)
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: %s -> %s\n",
        m_ePowerState == POWER_ON ? "POWER_ON" : (m_ePowerState == SOFT_POWER_OFF ? "SOFT_POWER_OFF" : "HARD_POWER_OFF"),
        ePowerState == POWER_ON ? "POWER_ON" : (ePowerState == SOFT_POWER_OFF ? "SOFT_POWER_OFF" : "HARD_POWER_OFF"));

    switch (m_ePowerState)
    {
        case POWER_ON:
            switch (ePowerState)
            {
                case SOFT_POWER_OFF:
                case HARD_POWER_OFF:
                {
                    SetLEDState(POWERING_OFF, true);

                    //
                    // Stop all player activity
                    //
                    CPlayerScreen::GetPlayerScreen()->PowerOff();
                    CRecordingManager::GetInstance()->StopRipping();
                    if (m_pPlayManager->GetPlayState() != CMediaPlayer::NOT_CONFIGURED)
                        m_pPlayManager->Deconfigure();

                    //
                    // Close CD tray, turn off the screen
                    //
                    GetCDDataSource()->CloseTray();
#ifdef __DJ
                   	LCDSetBacklight(LCD_BACKLIGHT_OFF);
#endif  // __DJ

                    suspend_timer( m_TimerHandle );
                    if (ePowerState == SOFT_POWER_OFF)
                    {
                        //
                        // Save state
                        //
                        SaveState();

                        //
                        // Start the Idle Encoding Jobs
                        //
                        if(!CIdleCoder::GetInstance()->NoJobs())
                        {
                            StartIdleEncoding();
                        }

                        SetLEDState(POWERING_OFF, false);
                        SetLEDState(POWER_OFF, true);

                    }
                    else
                    {                        
                        m_bSystemIsShuttingDown = true;
						// start the shutdown timer
                        // this give the event queue time to clear
                        // the timer calls ShutdownSystemTimerCallback() which posts an event
                        // that calls ShutdownSystem()
                        resume_timer( m_SystemShutdownTimerHandle );
                    }
                    break;
                }
            }
            break;

        case SOFT_POWER_OFF:
            switch (ePowerState)
            {
                case POWER_ON:
                {
                    StopIdleEncoding();
                    // Save the registry to disk
                    SaveState();

                    // make sure drives are powered up
                    //                    SetDrivesPower(true);
                    // wait half a second for the drives to spin up, since hard reset doesn't appear
                    //  to make the CD happy if it is powered off
                    //                    cyg_thread_delay(40);
                    SetLEDState( POWERING_ON, true );
                    cyg_thread_delay(30);
                    SafeReset( false );
                    break;
                }

                case HARD_POWER_OFF:
                {
                    StopIdleEncoding();
                    suspend_timer( m_TimerHandle );
                    
                    SetLEDState(IDLE_ENCODING, false);
                    SetLEDState(POWERING_OFF, true);

                    m_bSystemIsShuttingDown = true;
					// start the shutdown timer
                    // this give the event queue time to clear
                    // the timer calls ShutdownSystemTimerCallback() which posts an event
                    // that calls ShutdownSystem()
                    resume_timer( m_SystemShutdownTimerHandle );
                    break;
                }
            }
            break;

        case HARD_POWER_OFF:
            //
            // The only transition that makes sense is from hard power off to power on.
            //
            if (ePowerState == POWER_ON && !m_bSystemIsShuttingDown)
            {
                //                SetDrivesPower(true);
                // wait half a second for the drives to spin up, since hard reset doesn't appear
                //  to make the CD happy if it is powered off
                //                cyg_thread_delay(40);
                SetLEDState( POWERING_OFF, false );
                SetLEDState( HARD_POWERING_OFF, false );
                SetLEDState( POWERING_ON, true );
                cyg_thread_delay(30);
                SafeReset( false );
            }
            break;
    }

    m_ePowerState = ePowerState;
}

void
CDJPlayerState::SafeReset( bool bPowerOffFirst )
{
    DEBUGP( DJPS, DBGLEV_TRACE, "djps::SafeReset( %s )\n", bPowerOffFirst ? "Power Off First" : "Do Not Power Off First");

    // do a soft power off to avoid spinning down the drives
    if( bPowerOffFirst ) {
        GetInstance()->SetPowerState(SOFT_POWER_OFF);
    }

    // flush the logfile
    CDebugRouter::GetInstance()->Flush();

    // sync the disk
    CFatDataSource::Sync();
    
    // disable the dai/dac for some fucking reason
    DAIDisable();
    
    // disable interrupts, flush cache
    int oldints;
    HAL_DISABLE_INTERRUPTS(oldints);
    HAL_UCACHE_DISABLE();
    HAL_UCACHE_INVALIDATE_ALL();

    // Trigger a POR
    *(volatile unsigned char*)PDDDR &= ~(0x01);
    *(volatile unsigned char*)PDDR  |=  (0x01);

    // Soft reset for rev-02 boards
    void (*f)() = (void(*)())hal_vsr_table[0];
    f();
}

// Notifies us of UI activity
void
CDJPlayerState::NotifyDeviceActive()
{
    if( m_ePowerState == POWER_ON ) {
        reset_timer( m_TimerHandle );
    
        if( m_eEncodingState == ENCODING ) {
            PauseIdleEncoding();
        }
        resume_timer( m_TimerHandle );
    }
}

void
CDJPlayerState::IdleTimerCallback(void* pObj)
{
    ((CDJPlayerState*)pObj)->IdleTimerExpired();
}

void
CDJPlayerState::IdleTimerExpired()
{
    if( (m_eSource != LINE_IN) && (m_pPlayManager->GetPlayState() != CMediaPlayer::PLAYING) ) {
        StartIdleEncoding(false);
    }
}

// Set the current bitrate for the encoder.
void
CDJPlayerState::SetEncodeBitrate(int iBitrate, bool bSave)
{
    // check and see if this new bitrate is valid
    switch (iBitrate)
    {
    case 256:
    case 224:
    case 192:
    case 160:
    case 128:
    case 112:
    case 96:
    case 64:
    case 0:
        m_iEncodeBitrate = iBitrate;
#ifdef LINE_RECORDER_ENABLED
        CLineRecorder::GetInstance()->SetBitrate(iBitrate);
#endif
        break;
    default:
        m_iEncodeBitrate = DEFAULT_ENCODE_BITRATE;
    }
    if( bSave ) {
        // only save the registry when changing the bitrate
        SaveState(true, false, false);
    }
}


// Returns the number of bytes this class uses in the registry.
int
CDJPlayerState::GetStateSize() const
{
    return sizeof(int);
}


// returns player version/build number 
int
CDJPlayerState::GetPlayerVersion() const
{
#if defined (DDO_VERSION)
	return DDO_VERSION;
#else
#warning DDO_VERSION not found
	return 0;
#endif

}

// Set the current UI view state
void
CDJPlayerState::SetUIViewMode(EUIViewMode eUIViewMode, bool bKeyPress)
{
    switch(eUIViewMode)
    {
    case NORMAL:
    case ZOOM:
    case ZOOM_EXT:
        m_eUIViewMode = eUIViewMode;
        break;
    default:
        m_eUIViewMode = NORMAL;
        break;
    }

    DEBUGP( DJPS, DBGLEV_INFO, "djps: UI ZOOM: %s\n", m_eUIViewMode == NORMAL ? "OFF" : "ON");

    if(m_pUserInterface)
        m_pUserInterface->SetUIViewMode(eUIViewMode, bKeyPress);
    else
        DEBUGP( DJPS, DBGLEV_TRACE, "djps: m_pUserInterface is NULL!\n");
}

void
CDJPlayerState::SetUserInterface( IUserInterface* pUI ) 
{
    DEBUGP( DJPS, DBGLEV_TRACE, "djps::SetUserInterface()\n");
    m_pUserInterface = pUI;
}

void
CDJPlayerState::StartIdleEncoding( bool bEnableLED )
{
    CIdleCoder* pIdleCoder = CIdleCoder::GetInstance();
    if( !IsScanningHD() && pIdleCoder->GetState() != CIdleCoder::kEncoding && !pIdleCoder->NoJobs() ) {
        //
        // Start idle encoder thread
        //
        if (CSpaceMgr::GetInstance()->Status() == SPACE_OK && bEnableLED) {
            SetLEDState(IDLE_ENCODING, true);
        } else {
            SetLEDState(IDLE_ENCODING, false);
        }
        pIdleCoder->Run();
        m_eEncodingState = ENCODING;
    }
}

void
CDJPlayerState::PauseIdleEncoding()
{
    if( m_eEncodingState == ENCODING ) {
        SetLEDState(IDLE_ENCODING, false);
        CIdleCoder::GetInstance()->Pause();
        m_eEncodingState = PAUSED_ENCODING;
    }
}

void
CDJPlayerState::StopIdleEncoding()
{
    if( m_eEncodingState != NOT_ENCODING ) {
        SetLEDState(IDLE_ENCODING, false);
        CIdleCoder::GetInstance()->Halt();
//        CIdleCoder::GetInstance()->SaveToRegistry();  // Halting doesn't alter the job queue.
        m_eEncodingState = NOT_ENCODING;
    }
}

void
CDJPlayerState::SetUIEjectCDAfterRip(bool bUIEjectCDAfterRip)
{
    m_bUIEjectCDAfterRip = bUIEjectCDAfterRip;
}

void
CDJPlayerState::SetUIEnableExtChars(bool bUIEnableExtChars)
{
    m_bUIEnableExtChars = bUIEnableExtChars;
}

void
CDJPlayerState::SetUIEnableWebControl(bool bUIEnableWebControl)
{
    if (m_bUIEnableWebControl != bUIEnableWebControl)
        if (bUIEnableWebControl)
            StartWebControlServer();            
        else
            StopWebControlServer();            
    m_bUIEnableWebControl = bUIEnableWebControl;
}

void
CDJPlayerState::SetUIShowTrackNumInTitle(bool bUIShowTrackNumInTitle)
{
    m_bUIShowTrackNumInTitle = bUIShowTrackNumInTitle;
}

void
CDJPlayerState::SetUIShowAlbumWithArtist(bool bUIShowAlbumWithArtist)
{
    m_bUIShowAlbumWithArtist = bUIShowAlbumWithArtist;
}

void
CDJPlayerState::SetUIPlayCDWhenInserted(bool bUIPlayCDWhenInserted)
{
    m_bUIPlayCDWhenInserted = bUIPlayCDWhenInserted;
}

void
CDJPlayerState::SetUITextScrollSpeed(EUITextScrollSpeed eUITextScrollSpeed)
{
    m_eUITextScrollSpeed = eUITextScrollSpeed;
}

void
CDJPlayerState::SetUIEnableRecordLED(bool bUIEnableRecordLED)
{
    if (bUIEnableRecordLED)
        EnableLED();
    else
        DisableLED();
    m_bUIEnableRecordLED = bUIEnableRecordLED;
}

void
CDJPlayerState::SetUILCDBrightness(EUIBrightness eUILCDBrightness)
{
    m_eUILCDBrightness = eUILCDBrightness;
}

void
CDJPlayerState::ShutdownSystemTimerCallback(void* pObj)
{
    // stop the timer so we post only one EVENT_SYSTEM_SHUTDOWN event
    if (s_pSingleton)
        suspend_timer( s_pSingleton->m_SystemShutdownTimerHandle );

    // post an event so we don't cross any other events that are currently using the drives
    CEventQueue::GetInstance()->PutEvent( EVENT_SYSTEM_SHUTDOWN, NULL );
}

void
CDJPlayerState::ShutdownSystem()
{
    DEBUGP( DJPS, DBGLEV_INFO, "djps: ShutdownSystem\n");

    // give the signal that we're powering off
    SetLEDState(HARD_POWERING_OFF, true);

    // stop any timers so we don't accidentally shut down twice
    suspend_timer( m_TimerHandle );
    suspend_timer( m_SystemShutdownTimerHandle );

    // Save state in case something changed very recently
    SaveState();

    // Make sure all of our precious debug is saved to disk before we shut down
	CDebugRouter::GetInstance()->Flush();

#ifdef ENABLE_PARTIAL_BOOT
    SafeReset(false);
#else
	SetDrivesPower(false);
#endif

    // we're no longer in the process of shutting down.  we're there. 
    // shut down.  fini.  work done.  fait accompli  
    m_bSystemIsShuttingDown = false;
}

