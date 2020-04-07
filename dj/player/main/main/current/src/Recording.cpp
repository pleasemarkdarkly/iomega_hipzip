// Recording.cpp: helper functions for recording content
// edwardm@iobjects.com 11/27/01
// (c) Interactive Objects

// The CRecordingManager class is responsible for recording files to the HD.  It can
// record CDs (both audio and data) and tracks from an FML.
// There are two ways of copying tracks: ripping and recording.  Ripping copies the
// track without playback, recording copies the track while playing it back.
// Ripping and recording operations can be done on a single track or for the entire playlist.
//
// Copying is keyed off SDK events.  When any track is played, the following sequence
// of events occur:
// 1. CreatePlaystream is called by the media player.
// 2. When the track is actually set in the media player and ready to play,
// NotifyStreamSet is called.
// 3. When the track is played to completion, NotifyStreamEnd is called.  If playback is
// stopped before the end of the track is reached, then NotifyStreamAbort is called.
//
// The recording manager has two ways of copying files.  The method it uses depends on
// the type of file to be copied.  CDDA files are copied by using the SDK's filter
// system to direct a stream of decoded data to a file on the HD.  Other files types
// (i.e., ISO and net streams) are copied by opening a separate stream that copies
// the file to the HD.
//
// The process for copying CDDA files is as follows:
// 1. CreatePlaystream is called.  A FAT file output filter is added to the list of
//    filters, and a filename is created in the format
//       "a:\{content dir}\{disk ID}\{x}t{track #}.rxx", where
//          disk ID = Disk ID assigned by the CCDDirGen class; uses the form "CD1", "CD3F", etc.
//          x = counter
//          track # = 1-based index of the track on the CD
//    If in ripping mode, then the media player's buffer multiplier is set to 64 (for
//    fastest ripping) and the waveout filter is not added.
//    If in recording mode, then the buffer multiplier is set to 2 and the waveout filter is
//    used.
// 2. NotifyStreamSet is called.  A copy of the track's metadata is made so it can be
//    added to the content manager later.
// 3. NotifyStreamEnd is called.  The output file is moved from
//    "a:\{content dir}\{disk ID}\{x}t{track #}.rxx" to "a:\{content dir}\{disk ID}\{track #}.raw".
//    The file is added to the content manager if a record for that track doesn't already exist.
//    A job is added to the idle coder to encode the raw file to an MP3 file called
//    "a:\{content dir}\{disk ID}\{track #}.mxx".  The creation date of the raw file is also
//    set to the bitrate it should be encoded at.  During a content rescan this value is
//    used to reconstruct the encoding queue.
// 4. NotifyEncodingFinished is called when the idle coder is done.  The encoded file is
//    moved from "a:\{content dir}\{disk ID}\{track #}.mxx" to "a:\{disk ID}\{track #}.mp3".
//    The raw content record is removed from the content manager and the MP3 content record
//    is added in its place.  The raw file is deleted unless it is in the current playlist.
//    In that case it cannot be removed without affecting buffering, so instead the file is
//    added to a queue of files to be replaced when the playlist changes.  The creation time
//    of the raw file is set to 1 to indicate that there's an encoded version of the file
//    ready to be copied.  During a content rescan this is used to reconstruct the replacement queue.
//
// ISO and net files don't enjoy the luxury of being copied entirely through the media player,
// and so another class, the CFileCopier, is used in parallel with playback.
// 1. CreatePlaystream is called.  Nothing special.
// 2. NotifyStreamSet is called.  A job is added to the file copier to copy the file.
//    ISO tracks are copied to
//       "a:\{content dir}\{disk ID}\{path on cd}\t{x}"
//    FML tracks are copied to
//       "a:\{content dir}\f{fml UDN}\d{dir hash}\t{x}", where
//          fml UDN = munged UDN of the fml to extract only the tick count
//          dir hash = the media key of the item div 100, so only 100 items max can be in a dir.
// 
//    The track's metadata is also copied for later use.  If in ripping mode, the file copier's
//    buffer is set high and told to run.  If in recording mode, the file copier waits until
//    playback begins.
// 3. NotifyCopyingFinished is called when the file copier is done.  If in rip mode, then
//    the new file is moved to either
//       "a:\{content dir}\{disk ID}\{path on cd}\{filename}"
//    or
//       "a:\{content dir}\f{fml UDN}\d{dir hash}\{media key}.{codec extension}"
//    Files from the fml are also scanned for metadata, since the fml only gives us the title
//    and the artist.  MFT tags are written to PCM and WAV files so fml or edited metadata
//    won't be lost on a content rescan.  If ripping, the file is added to the content manager
//    when NotifyCopyingFinished is called.  If recording, then the record's data isn't added
//    until NotifyStreamEnd is called.


#include <main/main/Recording.h>
#include <codec/codecmanager/CodecManager.h>
#include <codec/common/Codec.h>
#include <core/events/SystemEvents.h>
#include <core/mediaplayer/MediaPlayer.h>
#include <core/playmanager/PlayManager.h>
#include <datasource/cddatasource/CDDataSource.h>   // We handle CD tracks specially, so we need to ID the cd data source
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datasource/fatdatasource/FatDataSource.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <datastream/fatfile/FileOutputStream.h>    // FATFILE_OUTPUT_ID
#include <datastream/waveout/WaveOutKeys.h> // Keys for the playstream_settings_t structure
#include <extras/idlecoder/IdleCoder.h>     // Ripped tracks are idle encoded
#include <extras/filecopier/FileCopier.h>   // ISO files are copied to HD in another thread
#include <main/content/djcontentmanager/DJContentManager.h>
#include <main/main/AppSettings.h>      // MAX_HD_TRACKS
#include <main/main/DJHelper.h>         // SetSong with feedback and recovery
#include <main/main/DJPlayerState.h>    // get the current encode bitrate
#include <main/main/FatHelper.h>        // Filename <-> URL
#include <main/main/LEDStateManager.h>  // LED control
#include <main/main/ProgressWatcher.h>
#include <main/main/RecordingEvents.h>
#include <main/main/SpaceMgr.h>
#include <main/ui/common/UserInterface.h>   // send system message text
#include <main/ui/PlayerScreen.h>
#include <main/ui/LibraryMenuScreen.h>      // GetQueryRemainingCount
#include <main/ui/Strings.hpp>
#include <util/debug/debug.h>   // debugging hooks
#include <util/eventq/EventQueueAPI.h>
#include <util/registry/Registry.h>
#include <util/utils/utils.h>

#include "CDDirGen.h"

#include <_modules.h>

#ifdef DDOMOD_DJ_METADATAFILETAG
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#endif

#include <stdio.h>  // sprintf
#include <stdlib.h> // malloc, free

#include <fs/fat/sdapi.h>


// If USE_FREEDB_ID_DIR, then CDs will be ripped to directories named after the matching freedb ID.
// If undefined, then the CDDirGen object will generate directory names like "cd00000004".
//#define USE_FREEDB_ID_DIR


DEBUG_MODULE_S( REC, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( REC );

// The global singleton recording manager.
CRecordingManager* CRecordingManager::s_pSingleton = 0;

// fdecl
static void IdleCodingCB(CIdleCoder::job_rec_t *, CIdleCoder::state_t);
static void FileCopyingCB(CFileCopier::job_rec_t *, CFileCopier::state_t);

//
// Playstream settings - we hold this here so we can
//  adjust them based on various events
//
static char szConfigurableStreamName[256];
static const unsigned int sDefaultOutputList[] =  { WAVEOUT_KEY, 0, 0, };
static const unsigned int sDefaultFilterList[] =  { 0, 0, 0 };

// Returns a pointer to the global recording manager.
CRecordingManager*
CRecordingManager::GetInstance()
{
    if (!s_pSingleton)
        s_pSingleton = new CRecordingManager;
    return s_pSingleton;
}

// Destroy the singleton global recording manager.
void
CRecordingManager::Destroy()
{
    delete s_pSingleton;
    s_pSingleton = 0;
}

CRecordingManager::CRecordingManager()
    : m_bCDInitialized(false),
    m_bRecordingEnabled(false),
    m_bRecording(false),
    m_bRipping(false),
    m_bSingle(false),
    m_bPaused(false),
    m_bKeepCurrent(false),
    m_bCopyReady(false),
    m_pFileCopyInfo(0),
    m_iTempIndex(0)
{
    // Initialize the media_record_info_t structure
    memset((void*)&m_mri, 0, sizeof(media_record_info_t));

    // Set up the default (playback) playstream
    CPlayStreamSettings settings;
    memcpy( (void*)&(settings.m_FilterList[0]), (void*)sDefaultFilterList, PLAYSTREAM_FILTER_LIMIT * sizeof(unsigned int) );
    memcpy( (void*)&(settings.m_OutputList[0]), (void*)sDefaultOutputList, PLAYSTREAM_OUTPUT_LIMIT * sizeof(unsigned int) );
    CPlayManager::GetInstance()->SetPlaystream( &settings, &CreatePlayStreamCB );

    // Set the callback for the idle coder.
    CIdleCoder::GetInstance()->SetCallback(IdleCodingCB);

    // Set the callback for the file copier.
    CFileCopier::GetInstance()->SetCallback(FileCopyingCB);

    // Create the directory name generator.
    CProgressWatcher::GetInstance()->SetTask(TASK_LOADING_CD_DIR_CACHE);
    m_pCDDirGen = new CCDDirGen(CD_DIR_NAME_DB_FILE, CD_DIR_NAME_BASE);
    CProgressWatcher::GetInstance()->UnsetTask(TASK_LOADING_CD_DIR_CACHE);

    // Find the CD and FAT data sources.
    // Currently there can be only one CD and one FAT data source.
    m_pCDDS = (CCDDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( CD_DATA_SOURCE_CLASS_ID, 0 );
    DBASSERT( REC, m_pCDDS, "CD datasource not found\n" );

    m_pFatDS = (CFatDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( FAT_DATA_SOURCE_CLASS_ID, 0 );
    DBASSERT( REC, m_pFatDS, "FAT datasource not found\n" );

    m_pNetDS = (CNetDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByClassID( NET_DATA_SOURCE_CLASS_ID, 0 );
    DBASSERT( REC, m_pNetDS, "Net datasource not found\n" );

    m_iPCMCodecID = CCodecManager::GetInstance()->FindCodecID("raw");
    m_iMP3CodecID = CCodecManager::GetInstance()->FindCodecID("mp3");
    m_iWMACodecID = CCodecManager::GetInstance()->FindCodecID("wma");
    m_iWAVCodecID = CCodecManager::GetInstance()->FindCodecID("wav");

}

CRecordingManager::~CRecordingManager()
{
    delete m_pCDDirGen;
}

void
CRecordingManager::SetUserInterface( IUserInterface* pUI ) 
{
    m_pUserInterface = pUI;
}

//! Tell the recording manager that a CD is in the drive.
//! If recording is possible then LED will turn on.
void
CRecordingManager::NotifyCDInserted(const cdda_toc_t* pTOC)
{
    // TODO: check if this cd has already been recorded.
    // TODO: check if there's enough space on the HD to rip this CD
//    SetLEDState(RECORDING_ENABLED, true);

    InitializeCDRip(pTOC);
}

//! Tell the recording manager that the CD has been ejected.
//! Stops recording if the current data source is the CD.
void
CRecordingManager::NotifyCDRemoved()
{
    if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
    {
        SetLEDState(RECORDING_ENABLED, false);
        m_bRecordingEnabled = false;
        if (IsRipping())
            StopRipping();
        if (IsRecording())
            StopRecording();
    }
    m_bCDInitialized = false;
}

#ifdef DDOMOD_DJ_BUFFERING
IInputStream* CreateBufferInStream( IMediaContentRecord* mcr );
#endif



ERESULT
CRecordingManager::StartRecording()
{
    DEBUGP( REC, DBGLEV_INFO, "rm:Start recording\n" );
    if (!m_bRecordingEnabled)
        return RECORDING_NOT_INITIALIZED;

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    SetLEDState(RECORDING, true);

    m_bRipping = false;
    m_bRecording = true;
    m_bSingle = false;

    return RECORDING_NO_ERROR;
}

ERESULT
CRecordingManager::StartRecordFull()
{
    DEBUGP( REC, DBGLEV_INFO, "Start full CD record\n" );

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    // Switch to normal playlist mode so we don't rip the CD over and over and over and over and over
    m_eMode = CPlayManager::GetInstance()->GetPlaylistMode();
    CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
    m_pUserInterface->SetPlaylistMode(IPlaylist::NORMAL);

    m_bRecordingEnabled = true;
    m_bRecording = true;
    m_bSingle = false;
    m_bRipping = false;
    m_bKeepCurrent = true;

#ifdef DDOMOD_DJ_BUFFERING
    // Use the stock input stream generator
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
#endif
    
    // Make a playlist of all files on the CD.
    if (!CDJPlayerState::GetInstance()->CreateCDPlaylist())
    {
        // We've were unable to set a track, so return an error.
        DEBUG( REC, DBGLEV_ERROR, "Unable to set any tracks from the CD\n" );
        m_pUserInterface->SetMessage(LS(SID_CANT_READ_CD), CSystemMessageString::STATUS);
        m_bRecording = false;
        m_bRipping = false;
        m_bRecordingEnabled = false;
        m_bKeepCurrent = false;
#ifdef DDOMOD_DJ_BUFFERING
        CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
        return RECORDING_ERROR;
    }

    SetLEDState(RECORDING, true);

    /*
    {
        // construct and send our system message
        char szNumber[32];
        TCHAR tszNumber[32];
        TCHAR tszMessage[256];
        tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
        sprintf(szNumber, " 1 ");
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_OF));
        sprintf(szNumber, " %d ", CPlayManager::GetInstance()->GetPlaylist()->GetSize());
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_TO_HD));
        m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
    }
    */

    return RECORDING_NO_ERROR;
}

ERESULT
CRecordingManager::StartRecordSingle(IPlaylistEntry* pEntry)
{
    DEBUGP( REC, DBGLEV_INFO, "rm:Start recording\n" );

    // Don't record radio streams.
    if (IsRadioStream(pEntry->GetContentRecord()))
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Can't record radio stream\n" );
        m_pUserInterface->SetMessage(LS(SID_CANT_RECORD_INTERNET_RADIO), CSystemMessageString::REALTIME_INFO);
        return RECORDING_RADIO_STREAM;
    }

    if (!m_bRecordingEnabled)
        return RECORDING_NOT_INITIALIZED;

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    m_bKeepCurrent = true;

    // Warn the user if we already have a copy of this track.
    if (FindTrackOnHD(pEntry->GetContentRecord()))
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Already recorded\n" );
        m_pUserInterface->SetMessage(LS(SID_TRACK_ALREADY_RECORDED), CSystemMessageString::REALTIME_INFO);
        m_bKeepCurrent = false;
//        return RECORDING_DUPLICATE;
    }

    // Determine if this is a CDDA track or an ISO file.
    bool bCDDA = m_pCDDS->GetAudioTrackIndex(pEntry->GetContentRecord()->GetURL()) != -1;

    m_bRipping = false;
    m_bRecording = true;
    m_bSingle = true;

    CPlayManager* pPM = CPlayManager::GetInstance();
#ifdef DDOMOD_DJ_BUFFERING
    if (bCDDA)
    {
        // Use the stock input stream generator
        CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
    }
#endif
    // Set the song in the MP.
    if (SUCCEEDED(pPM->SetSong(pEntry)))
    {
#ifdef DDOMOD_DJ_BUFFERING
        if (bCDDA)
        {
            // Use the stock input stream generator
            CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
        }
#endif
        DEBUGP( REC, DBGLEV_INFO, "rm:Single track recording file\n" );
        SetLEDState(RECORDING, true);

        if (m_bKeepCurrent)
        {
            // construct and send our system message
            char szNumber[32];
            TCHAR tszNumber[32];
            TCHAR tszMessage[256];
            tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
            sprintf(szNumber, " %d ", pPM->GetPlaylist()->GetEntryIndex(pEntry) + 1);
            CharToTcharN(tszNumber, szNumber, 31);
            tstrcat(tszMessage, tszNumber);
            tstrcat(tszMessage, LS(SID_TO_HD));
            m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
        }
        return RECORDING_NO_ERROR;
    }
    else
        DEBUG( REC, DBGLEV_WARNING, "Unable to set file\n" );

    m_bRecording = false;
    m_bSingle = false;
    m_bKeepCurrent = false;

    return RECORDING_ERROR;
}

ERESULT
CRecordingManager::StopRecording(bool bPrintMessage)
{
    DEBUGP( REC, DBGLEV_INFO, "rm:Stop recording\n" );

    SetLEDState(RECORDING, false);
    if (m_bRecording)
    {
        m_bRecording = false;
        m_bSingle = false;
        if (bPrintMessage)
        {
            m_pUserInterface->SetMessage(LS(SID_STOPPED_RECORDING_TRACK_TO_HD), CSystemMessageString::REALTIME_INFO);
            m_pUserInterface->SetMessage(LS(SID_PARTIAL_TRACK_RECORDING_CANCELLED), CSystemMessageString::INFO);
        }
    }

    return RECORDING_NO_ERROR;
}

bool
CRecordingManager::IsRecording() const
{
    return m_bRecording;
}

bool
CRecordingManager::IsRecordingFull() const
{
    return m_bRecording && !m_bSingle;
}

bool
CRecordingManager::IsRecordingSingle() const
{
    return m_bRecording && m_bSingle;
}

bool
CRecordingManager::IsKeepingCurrentTrack() const
{
    return m_bKeepCurrent;
}

ERESULT
CRecordingManager::StartRipping()
{
    DEBUGP( REC, DBGLEV_INFO, "Start ripping\n" );

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    m_bRecordingEnabled = true;
    m_bRecording = true;
    m_bRipping = true;
    m_bSingle = false;
    m_bKeepCurrent = true;

    // Switch to normal playlist mode so we don't rip the CD over and over and over and over and over
    m_eMode = CPlayManager::GetInstance()->GetPlaylistMode();
    CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
    m_pUserInterface->SetPlaylistMode(IPlaylist::NORMAL);
    
#ifdef DDOMOD_DJ_BUFFERING
    // Use the stock input stream generator
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
#endif
    
    return RECORDING_NO_ERROR;
}

//#define PROFILE_MEM_USAGE
#ifdef PROFILE_MEM_USAGE
static unsigned long s_fordblks_start = 0;
static unsigned long s_ripped_count = 0;
#endif  // PROFILE_MEM_USAGE

ERESULT
CRecordingManager::StartRipPlaylist()
{
    DEBUGP( REC, DBGLEV_INFO, "Start ripping playlist\n" );

    // Make sure the current playlist isn't empty.
    IPlaylist* pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
    if (!pPlaylist || pPlaylist->IsEmpty())
        return RECORDING_NO_TRACKS;

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    m_bRecordingEnabled = true;
    m_bRecording = true;
    m_bRipping = true;
    m_bSingle = false;
    m_bKeepCurrent = true;

    // Switch to normal playlist mode so we don't rip the CD over and over and over and over and over
    m_eMode = CPlayManager::GetInstance()->GetPlaylistMode();
    CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
    m_pUserInterface->SetPlaylistMode(IPlaylist::NORMAL);

#ifdef PROFILE_MEM_USAGE
    struct mallinfo mem_info = mallinfo();
    s_fordblks_start = mem_info.fordblks;
    diag_printf("***\n* Ripping:\n* Bytes: %d\n***\n", mem_info.fordblks);
#endif  // PROFILE_MEM_USAGE

#ifdef DDOMOD_DJ_BUFFERING
    // Use the stock input stream generator
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
#endif
    
    // How many files are left to query?
    int nQueryLeft = ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->GetQueryRemainingCount();

    // Set the first song in the playlist.
    pPlaylist->SetCurrentEntry(pPlaylist->GetEntry(0));
    res = DJSetCurrentOrNext();
    if (FAILED(res))
    {
        // We've were unable to set any tracks, so return an error.
        DEBUG( REC, DBGLEV_ERROR, "Unable to set any tracks from the playlist\n" );
        bool bQuerying = nQueryLeft != 0;
        if( res == PM_PLAYLIST_END ) {
            if( bQuerying ) {
                // Let the user know it's just because we've hit the end of the playlist, and it's still growing
                m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_QUERIED_TRACKS_TO_HD), CSystemMessageString::REALTIME_INFO);
            }
            else {
                // Nothing to record in the playlist
                m_pUserInterface->SetMessage(LS(SID_NO_TRACKS_TO_RECORD_IN_CURRENT_PLAYLIST), CSystemMessageString::REALTIME_INFO);
            }
        } else {
            // Probably an FML error
            m_pUserInterface->SetMessage(LS(SID_FAILED_TO_COPY_LAST_TRACK), CSystemMessageString::REALTIME_INFO);
        }
        m_bRecording = false;
        m_bRipping = false;
        m_bRecordingEnabled = false;
        m_bKeepCurrent = false;
#ifdef DDOMOD_DJ_BUFFERING
        CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif

        RestorePlayMode();
        if (IPlaylistEntry* pEntry = pPlaylist->GetEntry(0, m_eMode))
        {
            CPlayManager::GetInstance()->GetPlaylist()->SetCurrentEntry(pEntry);
            DJSetCurrentOrNext(true);
        }

        return RECORDING_ERROR;
    }

    SetLEDState(RECORDING, true);

    {
        // construct and send our system message
        char szNumber[32];
        TCHAR tszNumber[32];
        TCHAR tszMessage[256];
        tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
        sprintf(szNumber, " %d ", pPlaylist->GetCurrentEntry()->GetIndex() + 1);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_OF));
        sprintf(szNumber, " %d ", pPlaylist->GetSize() + nQueryLeft);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_TO_HD));
        m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
    }

    return RECORDING_NO_ERROR;
}

ERESULT
CRecordingManager::StartRipFull()
{
    DEBUGP( REC, DBGLEV_INFO, "Start ripping\n" );

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    m_bRecordingEnabled = true;
    m_bRecording = true;
    m_bRipping = true;
    m_bSingle = false;
    m_bKeepCurrent = true;

    // Switch to normal playlist mode so we don't rip the CD over and over and over and over and over
    m_eMode = CPlayManager::GetInstance()->GetPlaylistMode();
    CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::NORMAL);
    m_pUserInterface->SetPlaylistMode(IPlaylist::NORMAL);
    
#ifdef DDOMOD_DJ_BUFFERING
    // Use the stock input stream generator
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
#endif
    
    // Make a playlist of all files on the CD.
    if (!CDJPlayerState::GetInstance()->CreateCDPlaylist())
    {
        // We've were unable to set a track, so return an error.
        DEBUG( REC, DBGLEV_ERROR, "Unable to set any tracks from the CD\n" );
        m_pUserInterface->SetMessage(LS(SID_CANT_READ_CD), CSystemMessageString::STATUS);
        m_bRecording = false;
        m_bRipping = false;
        m_bRecordingEnabled = false;
        m_bKeepCurrent = false;
#ifdef DDOMOD_DJ_BUFFERING
        CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif

        RestorePlayMode();
        if (IPlaylistEntry* pEntry = CPlayManager::GetInstance()->GetPlaylist()->GetEntry(0, m_eMode))
        {
            CPlayManager::GetInstance()->GetPlaylist()->SetCurrentEntry(pEntry);
            DJSetCurrentOrNext(true);
        }
        return RECORDING_ERROR;
    }

    SetLEDState(RECORDING, true);

    {
        // construct and send our system message
        char szNumber[32];
        TCHAR tszNumber[32];
        TCHAR tszMessage[256];
        tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
        sprintf(szNumber, " %d ", CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetIndex() + 1);
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_OF));
        sprintf(szNumber, " %d ", CPlayManager::GetInstance()->GetPlaylist()->GetSize() + ((CLibraryMenuScreen*)CLibraryMenuScreen::GetLibraryMenuScreen())->GetQueryRemainingCount());
        CharToTcharN(tszNumber, szNumber, 31);
        tstrcat(tszMessage, tszNumber);
        tstrcat(tszMessage, LS(SID_TO_HD));
        m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
    }

    return RECORDING_NO_ERROR;
}

ERESULT
CRecordingManager::StartRipSingle(IPlaylistEntry* pEntry)
{
    DEBUGP( REC, DBGLEV_INFO, "Start ripping\n" );

    // Don't record radio streams.
    if (IsRadioStream(pEntry->GetContentRecord()))
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Can't record radio stream\n" );
        m_pUserInterface->SetMessage(LS(SID_CANT_RECORD_INTERNET_RADIO), CSystemMessageString::REALTIME_INFO);
        return RECORDING_RADIO_STREAM;
    }

    // Check space and track limits before proceeding.
    ERESULT res = CheckSpaceAndTrackLimits();
    if (FAILED(res))
        return res;

    // Make sure we don't already have a copy of this track.
    if (FindTrackOnHD(pEntry->GetContentRecord()))
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Already recorded\n" );
        m_pUserInterface->SetMessage(LS(SID_TRACK_ALREADY_RECORDED), CSystemMessageString::REALTIME_INFO);
        return RECORDING_DUPLICATE;
    }

    // Determine if this is a CDDA track or an ISO file.
    bool bCDDA = m_pCDDS->GetAudioTrackIndex(pEntry->GetContentRecord()->GetURL()) != -1;

    m_bRipping = true;
    m_bRecording = false;
    m_bSingle = true;
    m_bKeepCurrent = true;

    CPlayManager* pPM = CPlayManager::GetInstance();
    if (bCDDA)
    {
#ifdef DDOMOD_DJ_BUFFERING
        // Use the stock input stream generator
        CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( 0 );
#endif
        // Set the song in the MP again so that the playstream can be configured to be silent.
        if (SUCCEEDED(pPM->SetSong(pEntry)))
        {
#ifdef DDOMOD_DJ_BUFFERING
            // Use the stock input stream generator
            CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
            DEBUGP( REC, DBGLEV_INFO, "rm:Single track ripping CDDA file\n" );
            SetLEDState(RECORDING, true);
            {
                // construct and send our system message
                char szNumber[32];
                TCHAR tszNumber[32];
                TCHAR tszMessage[256];
                tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
                sprintf(szNumber, " %d ", pPM->GetPlaylist()->GetEntryIndex(pEntry) + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_TO_HD));
                m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
            }
            return RECORDING_NO_ERROR;
        }
        else
            DEBUG( REC, DBGLEV_WARNING, "Unable to set CDDA file\n" );
    }
    else
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Single track ripping ISO file\n" );

        // Set the song in the MP again so that the playstream can be configured to be silent.
        if (SUCCEEDED(pPM->SetSong(pEntry)))
        {
            // Start the copier thread.
//            CFileCopier::GetInstance()->SetBufferSize(1 << 17);
//            CFileCopier::GetInstance()->Run();

            SetLEDState(RECORDING, true);
            m_bRipping = true;
            m_bSingle = true;

            {
                // construct and send our system message
                char szNumber[32];
                TCHAR tszNumber[32];
                TCHAR tszMessage[256];
                tstrcpy(tszMessage, LS(SID_RECORDING_TRACK));
                sprintf(szNumber, " %d ", pPM->GetPlaylist()->GetEntryIndex(pEntry) + 1);
                CharToTcharN(tszNumber, szNumber, 31);
                tstrcat(tszMessage, tszNumber);
                tstrcat(tszMessage, LS(SID_TO_HD));
                m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
            }
            return RECORDING_NO_ERROR;
        }
    }

    m_bRipping = false;
    m_bSingle = false;
    m_bKeepCurrent = false;

    return RECORDING_ERROR;
}

ERESULT
CRecordingManager::StopRipping(bool bPrintMessage)
{
    DEBUGP( REC, DBGLEV_INFO, "rm:Stop ripping\n" );

    if (m_bSingle)
    {
        if (bPrintMessage)
        {
            m_pUserInterface->SetMessage(LS(SID_STOPPED_RECORDING_TRACK_TO_HD), CSystemMessageString::REALTIME_INFO);
            m_pUserInterface->SetMessage(LS(SID_PARTIAL_TRACK_RECORDING_CANCELLED), CSystemMessageString::INFO);
        }
        m_pUserInterface->SetTrackTime( 0 );
    }
    else if (m_bRecording || m_bRipping)
    {
        if (bPrintMessage)
        {
            if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
                m_pUserInterface->SetMessage(LS(SID_STOPPED_RECORDING_CD_TO_HD), CSystemMessageString::REALTIME_INFO);
            else
                m_pUserInterface->SetMessage(LS(SID_STOPPED_RECORDING_CURRENT_PLAYLIST_TO_HD), CSystemMessageString::REALTIME_INFO);
            m_pUserInterface->SetMessage(LS(SID_PARTIAL_TRACK_RECORDING_CANCELLED), CSystemMessageString::INFO);
        }
        m_pUserInterface->SetTrackTime( 0 );

        if (m_bRipping)
        {
            // Revert back to the play mode before ripping started.
            RestorePlayMode();
        }
    }

    // Stop the file copier.
    CFileCopier::GetInstance()->Halt();

    // Reset the file copier's buffer size.
    CFileCopier::GetInstance()->SetBufferSize(1 << 15);

#ifdef DDOMOD_DJ_BUFFERING
    // Reestablish the DJ input stream generator
    CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
    
#ifdef PROFILE_MEM_USAGE
    if (s_fordblks_start)
    {
        struct mallinfo mem_info = mallinfo();
        diag_printf("***\n* Stop ripping, pre commit:\n* Bytes: %d Ripped: %d Bytes per ripped: %d\n***\n",
            s_fordblks_start - mem_info.fordblks, s_ripped_count, s_ripped_count ? (s_fordblks_start - mem_info.fordblks) / s_ripped_count : 0);
    }
#endif  // PROFILE_MEM_USAGE

#ifdef PROFILE_MEM_USAGE
    if (s_fordblks_start)
    {
        struct mallinfo mem_info = mallinfo();
        diag_printf("***\n* Stop ripping, post commit:\n* Bytes: %d Ripped: %d Bytes per ripped: %d\n***\n",
            s_fordblks_start - mem_info.fordblks, s_ripped_count, s_ripped_count ? (s_fordblks_start - mem_info.fordblks) / s_ripped_count : 0);
        s_ripped_count = 0;
        s_fordblks_start = 0;
    }
#endif  // PROFILE_MEM_USAGE

    m_bRecording = false;
    m_bRipping = false;
    m_bSingle = false;

    SetLEDState(RECORDING, false);

    return RECORDING_NO_ERROR;
}

bool
CRecordingManager::IsRipping() const
{
    return m_bRipping;
}

bool
CRecordingManager::IsRippingFull() const
{
    return m_bRipping && !m_bSingle;
}

bool
CRecordingManager::IsRippingSingle() const
{
    return m_bRipping && m_bSingle;
}


void
CRecordingManager::DisableRecording()
{
    StopRecording();
    m_bRecordingEnabled = false;
    SetLEDState(RECORDING_ENABLED, false);

    // Stop the file copier.
    CFileCopier::GetInstance()->Halt();
    CleanupCurrentIsoCopy();

    // If the file has been copied and we were waiting for playback to end before adding
    // it to the content manager, then free the memory.
    if (m_bCopyReady)
    {
        CleanupPreparedIsoCopy(m_pFileCopyInfo);
        m_pFileCopyInfo = 0;
        m_bCopyReady = false;
    }
}

bool
CRecordingManager::IsRecordingEnabled() const
{
    return m_bRecordingEnabled;
}

void
CRecordingManager::RestorePlayMode()
{
    // Revert back to the play mode before ripping started.
    CPlayManager::GetInstance()->SetPlaylistMode(m_eMode);
    m_pUserInterface->SetPlaylistMode(m_eMode);
}

bool
CRecordingManager::NotifyStreamSet(set_stream_event_data_t* pSSED, IMediaContentRecord* pContentRecord)
{
    m_bCopyReady = false;
    bool bShouldSkip = false;

    IDataSource* pDS = CDataSourceManager::GetInstance()->GetDataSourceByID(pContentRecord->GetDataSourceID());
    if (pDS && pDS->GetClassID() == CD_DATA_SOURCE_CLASS_ID)
    {
        // Has this track has already been copied?
        if (FindCDTrackOnHD(pContentRecord->GetURL()))
        {
            // Yep.  Forget this track when it's finished.
            m_bKeepCurrent = false;

            // Are we ripping?
            if (m_bRipping)
            {
                // Yep.  Skip this track.
//                CPlayManager::GetInstance()->NextTrack();
                bShouldSkip = true;
            }
        }
        else
            m_bKeepCurrent = true;

        // This track is recordable, so check if it's a CDDA track or an ISO file.
        int iTrackIndex = m_pCDDS->GetAudioTrackIndex( pContentRecord->GetURL() );
        if (iTrackIndex != -1)
        {
            // CDDA track
            m_bRecordingEnabled = true;

            // Stop recording if we've run out of space or if the maximum number of tracks
            // on the hard drive has been reached.
            if (FAILED(CheckSpaceAndTrackLimits(false)))
            {
                if (m_bRipping)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRipCD\n");
                    StopRipping(false);
                    CPlayManager::GetInstance()->Stop();
                }
                else if (m_bRecording)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRecordCD\n");
                    m_bRecording = false;
                    SetLEDState(RECORDING, false);
                }
            }
            else
                SetLEDState(RECORDING_ENABLED, true);

            DEBUGP( REC, DBGLEV_TRACE, "NotifyStreamSet on item %s, track %d\n", pContentRecord->GetURL(), iTrackIndex);

            m_mri.iCodecID = m_iPCMCodecID;
            m_mri.iDataSourceID = m_pFatDS->GetInstanceID();
            m_mri.bVerified = true;
            m_mri.pMetadata = 0;

            // If in ripping mode, then start playback.
            if (m_bRipping || m_bRecording)
                CPlayManager::GetInstance()->Play();
        }
        else
        {
            // ISO file
            m_bRecordingEnabled = true;

            // Stop recording if we've run out of space or if the maximum number of tracks
            // on the hard drive has been reached.
            if (FAILED(CheckSpaceAndTrackLimits(false)))
            {
                if (m_bRipping)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRipISO\n");
                    StopRipping(false);
                    CPlayManager::GetInstance()->Stop();
                }
                else if (m_bRecording)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRipISO\n");
                    m_bRecording = false;
                    SetLEDState(RECORDING, false);
                }
            }
            else
                SetLEDState(RECORDING_ENABLED, true);

            DEBUGP( REC, DBGLEV_TRACE, "NotifyStreamSet on item %s, ISO file\n", pContentRecord->GetURL());

            PrepIsoCopy(pContentRecord, pSSED->streamInfo.Duration);
            // TODO: cleanup if this fails.

            // If in ripping mode, then start copying this track.
//            if (m_bRipping && !m_bSingle)
            CFileCopier::GetInstance()->SetBufferSize(1 << 15);
            if (m_bRipping)
            {
                // On a multisession disk we can transition from an audio track to a
                // data track while ripping, which will cause the data track to start playing.
                // This is bad.  Tell the play manager to stop playback.
                if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
                    CPlayManager::GetInstance()->Stop();
                CFileCopier::GetInstance()->SetBufferSize(1 << 17);
                CFileCopier::GetInstance()->Run();
                m_bPaused = false;
            }
            else if (m_bRecording)
            {
                // If recording, then start playback.
                CPlayManager::GetInstance()->Play();
            }
        }
    }
    else if (pDS && pDS->GetClassID() == NET_DATA_SOURCE_CLASS_ID)
    {
        // If the stream has a duration value, then assume it comes from an fml instead of internet radio.
        if (pSSED->streamInfo.Duration)
        {
            m_bRecordingEnabled = true;

            // Has this track has already been copied?
            if (FindTrackOnHD(pContentRecord))
            {
                // Yep.  Forget this track when it's finished.
                m_bKeepCurrent = false;

                // Are we ripping?
                if (m_bRipping)
                {
                    // Yep.  Skip this track.
                    bShouldSkip = true;
                }
            }
            else
                m_bKeepCurrent = true;

            // Stop recording if we've run out of space or if the maximum number of tracks
            // on the hard drive has been reached.
            if (FAILED(CheckSpaceAndTrackLimits(false)))
            {
                if (m_bRipping)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRipNet\n");
                    StopRipping(false);
                    CPlayManager::GetInstance()->Stop();
                }
                else if (m_bRecording)
                {
                    DEBUGP( REC , DBGLEV_INFO, "rm:SpcMgr-NoRipNet\n");
                    m_bRecording = false;
                    SetLEDState(RECORDING, false);
                }
            }
            else
                SetLEDState(RECORDING_ENABLED, true);

            PrepNetCopy(pContentRecord, pSSED->streamInfo.Duration);
            // TODO: cleanup if this fails.

            // If in ripping mode, then start copying this track.
            CFileCopier::GetInstance()->SetBufferSize(1 << 15);
            if (m_bRipping)
            {
                // On a multisession disk we can transition from an audio track to a
                // data track while ripping, which will cause the data track to start playing.
                // This is bad.  Tell the play manager to stop playback.
                if (CPlayManager::GetInstance()->GetPlayState() == CMediaPlayer::PLAYING)
                    CPlayManager::GetInstance()->Stop();
                // dc- the below seems faster but makes progress choppy
                //                CFileCopier::GetInstance()->SetBufferSize(1 << 19);
                CFileCopier::GetInstance()->SetBufferSize(1 << 17);
                CFileCopier::GetInstance()->Run();
                m_bPaused = false;
            }
            else if (m_bRecording)
            {
                // If recording, then start playback.
                CPlayManager::GetInstance()->Play();
            }

        }
        else
        {
            // No duration, so assume this is internet radio.
            // Are we ripping?
            if (m_bRipping)
            {
                // Yep.  Skip this track.
                bShouldSkip = true;
            }
            else
            {
                // Disable recording.
                m_bRecording = false;
                m_bRecordingEnabled = false;
                SetLEDState(RECORDING_ENABLED, false);
                SetLEDState(RECORDING, false);
            }
        }
    }
    else if (pDS && pDS->GetClassID() == FAT_DATA_SOURCE_CLASS_ID)
    {
        // Are we ripping?
        if (m_bRipping)
        {
            // Yep.  Skip this track.
            bShouldSkip = true;
        }
        // Recording is enabled, but don't actually set up a recording stream.
        m_bRecordingEnabled = true;
        m_bKeepCurrent = false;
    }
    // We're not playing from CD or FML.  If we're not in the line in source, then recording is disabled.
    else if (CDJPlayerState::GetInstance()->GetSource() != CDJPlayerState::LINE_IN)
    {
        m_bRecording = false;
        m_bRecordingEnabled = false;
        SetLEDState(RECORDING_ENABLED, false);
    }

    return bShouldSkip;
}

//! Called from the event loop whenever a track starts playing.
//! Starts/continues copying a data CD.
void
CRecordingManager::NotifyStreamPlaying()
{
    // Make sure the LED is lit while recording/ripping.
    if (m_bRecording || m_bRipping)
        SetLEDState(RECORDING, true);
    CFileCopier::GetInstance()->Run();
    m_bPaused = false;
}

//! Called from the event loop whenever a track starts playing.
//! Pauses copying a data CD.
void
CRecordingManager::NotifyStreamPaused()
{
    CFileCopier::GetInstance()->Pause();
    m_bPaused = true;
}

#define DeleteFile(a) pc_unlink(const_cast<char*>(a))
#if DEBUG_LEVEL == 0
#define MoveAndVerify(a, b) pc_mv(const_cast<char*>(a), const_cast<char*>(b))
#else
#include <datastream/fatfile/FileInputStream.h>
static SDBOOL MoveAndVerify(const char* szSource, const char* szDest)
{
    if (pc_mv(const_cast<char*>(szSource), const_cast<char*>(szDest)))
    {
        CFatFileInputStream ffis;
        if (SUCCEEDED(ffis.Open(szDest)))
            return YES;
        else
            return NO;
    }
    else
    {
        DEBUGP( REC, DBGLEV_INFO, "Unable to move file %s to %s: Error %d: %s\n", szSource, szDest, pc_get_error(0), GetFatErrorString(pc_get_error(0)) );
        return NO;
    }
}
#endif

extern "C"
{
extern SDVOID oem_setsysdate(
    UINT16 tdate,
    UINT16 ttime,
    UTINY  ttime_tenths);
}

extern void SetMainThreadPriority(int nPrio);
extern int GetMainThreadPriority();

bool
CRecordingManager::NotifyStreamEnd(change_stream_event_data_t* pCSED)
{
    DBEN( REC );

    m_bPaused = false;
    bool bShouldStop = false;
    CDJContentManager* pCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();

    const char* szStreamName = pCSED->pPreviousStream->GetStreamName();

    if (m_bRipping || (m_bRecording && m_bKeepCurrent))
    {
        // Make sure this is a CDDA track.
        if (szStreamName && szStreamName[0] != '\0')
        {
            DEBUGP( REC, DBGLEV_INFO, "rm:Adding record %s\n", szStreamName );

            // Scan the filename used by the stream to get the index of the track that just finished recording.
            // TODO: Real time encoding #defines.
            int temp, index;
            char szExt[5];
            if (sscanf(szStreamName, m_szRipToFileBase, &temp, &index, &szExt ) != 3)
                DEBUG( REC, DBGLEV_ERROR, "Error parsing temp filename string %s\n", szStreamName );

            // Get the metadata record for this stream.
            // This is done after the stream is done playing so that the most up-to-date metadata will be stored.
            m_mri.pMetadata = CPlayManager::GetInstance()->GetPlaylist()->GetCurrentEntry()->GetContentRecord()->Copy();

            // Make URLs that point to the raw recorded file and to the destination file for encoding.
            // The base URL is in the form file://content/cdxxxxxxxx/%d.%s
            // So to allocate enough space for the real filename just use the base string length
            // plus 4 (%d expands to at most "99", so no extra character needed there; %s expands
            // to a 3-letter extension, so add 1 for the overrun and 1 for the null character).
            m_mri.szURL = (char*)malloc(strlen(m_szSaveToURLBase) + 5);
            sprintf(m_mri.szURL, m_szSaveToURLBase, index, "raw");

            // Use .mxx extension so it won't be picked up on file scans until encoding is completed.
            char szOutURL[strlen(m_szSaveToURLBase) + 5];
            sprintf(szOutURL, m_szSaveToURLBase, index, "mxx");

            char szFinalURL[strlen(m_szSaveToURLBase) + 5];
            sprintf(szFinalURL, m_szSaveToURLBase, index, "mp3");

            const char* szOutFile = FullFilenameFromURLInPlace(m_mri.szURL);

            // Delete the original raw file from the drive, if there is one.
            bool bOriginalDeleted = DeleteFile( szOutFile );

            // If there's already a content record in the database for this track, then remove it and
            // delete the file it points to.
            IMediaContentRecord* pRawCR = pCM->GetMediaRecord(m_mri.szURL);

            bool bSaveIdleCoderRegistry = false;

            if (pRawCR)
            {
                DEBUGP( REC, DBGLEV_INFO, "rm:Record %s already in database\n", m_mri.szURL );

                // Give a warning if the original file wasn't deleted.
                if (!bOriginalDeleted)
                    DEBUG( REC, DBGLEV_WARNING, "Unable to delete original file %s\n", szOutFile );

                // Don't remove the content record, since it will point to the same filename in the end.

                // Remove the original file from the encoding queue.
                bSaveIdleCoderRegistry = CIdleCoder::GetInstance()->RemoveJob(m_mri.szURL);

            }
            else if (IMediaContentRecord* pMP3CR = pCM->GetMediaRecord(szFinalURL))
            {
                DEBUGP( REC, DBGLEV_INFO, "rm:Encoded record %s already in database\n", szFinalURL );

                const char* szMP3File = FullFilenameFromURLInPlace(szFinalURL);
                if (!DeleteFile( szMP3File ))
                    DEBUG( REC, DBGLEV_WARNING, "Unable to delete file %s\n", szMP3File );

                // Remove the MP3 content record.
                pCM->DeleteMediaRecord(pMP3CR);
            }

            DEBUGP( REC, DBGLEV_INFO, "rm:Moving temp file %s to %s\n", szStreamName, szOutFile );
            if (MoveAndVerify(szStreamName, szOutFile) == YES)
            {
                // Use the creation date of the file to store the bitrate.
                // This can be used later to restore the encode queue if the registry is lost.
                oem_setsysdate(CDJPlayerState::GetInstance()->GetEncodeBitrate(), 0, 0);
                pc_touch(const_cast<char*>(szOutFile));

                CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);

                // lower thread prio to prevent audio drop out
                int nPrio=0; // appease compiler
                if (!m_bRipping)
                {
                    // Unless we're ripping, in which case we don't care about audio drop out.
                    nPrio = GetMainThreadPriority();
                    SetMainThreadPriority(UI_THREAD_BUSY_PRIORITY);
                }

                IMediaContentRecord* pCR = pCM->AddMediaRecord(m_mri);

                // return prio to normal level
                if (!m_bRipping)
                {
                    SetMainThreadPriority(nPrio);
                }

#ifdef DDOMOD_DJ_METADATAFILETAG
                CMetadataFileTag::GetInstance()->UpdateTag(m_mri.szURL, pCR);
#endif

                if (CDJPlayerState::GetInstance()->GetEncodeBitrate())
                {
                    // Add the track to the idle coder queue.
                    CIdleCoder::GetInstance()->Enqueue(m_mri.szURL, szOutURL, CDJPlayerState::GetInstance()->GetEncodeBitrate());
                    bSaveIdleCoderRegistry = true;
                }
                else
                    DEBUGP( REC, DBGLEV_WARNING, "rm: Leaving file %s uncompressed\n", m_mri.szURL );
            }
            else
            {
                DEBUGP( REC, DBGLEV_INFO, "rm:Unable to move file %s to %s: Error %d: %s\n",
                    szStreamName, szOutFile, pc_get_error(0), GetFatErrorString(pc_get_error(0)) );

                // Well, we're boned.  The file can't be moved, and the original file is probably dead.
                // So if there was a content record to match this file, get rid of it.
                // TODO: apologize to the user
                if (pRawCR)
                    pCM->DeleteMediaRecord(pRawCR);
                delete m_mri.pMetadata;
            }

            // Save the registry if the idle coder's job queue was changed.
            if (bSaveIdleCoderRegistry)
            {
                CIdleCoder::GetInstance()->SaveToRegistry();
                SetRegistryDirty();
            }

            // Print a warning if we've reached the limits of recording.
            CheckSpaceAndTrackLimits();

            free(m_mri.szURL);
            memset((void*)&m_mri, 0, sizeof(media_record_info_t));
        }
        else
        {
            DEBUGP( REC, DBGLEV_INFO, "rm:Finished playing back ISO file\n");
            if (m_bRecording)
            {
                DEBUGP( REC, DBGLEV_INFO, "rm:Time to add a new content record\n");
                if (m_bCopyReady)
                {
                    DEBUGP( REC, DBGLEV_INFO, "rm:Adding to URL %s\n",  m_pFileCopyInfo->szFinalURL);

                    AddCopyToContentManager(m_pFileCopyInfo);
                    m_pFileCopyInfo = 0;
                    m_bCopyReady = false;
                }
                else
                    DEBUG( REC, DBGLEV_WARNING, "The copy isn't ready.  This is bad.\n");

                // Print a warning if we've reached the limits of recording.
                CheckSpaceAndTrackLimits();
            }
        }

        // If in single-track ripping/recording mode, then stop ripping/recording.
        if (m_bSingle)
        {
            m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_TRACK_TO_HD), CSystemMessageString::REALTIME_INFO);
            bShouldStop = m_bRipping;
            m_bRipping = false;
            m_bRecording = false;
            m_bSingle = false;
            SetLEDState(RECORDING, false);
//            CPlayManager::GetInstance()->Stop();

#ifdef DDOMOD_DJ_BUFFERING
            // Reestablish the DJ input stream generator
            CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif

            // Notify the UI so it can recache the top level queries.
            m_pUserInterface->NotifyMetadataUpdateEnd(m_pFatDS->GetInstanceID(), 0);
        }
        // If we've ripped the entire CD, then eject it.
        else if (m_bRipping)
        {
#ifdef ENABLE_FIORI_TEST
            // force this for the fiori test
            CPlayManager::GetInstance()->SetPlaylistMode(IPlaylist::REPEAT_ALL);
            m_pUserInterface->SetPlaylistMode(IPlaylist::REPEAT_ALL);
            IPlaylist::PlaylistMode eMode = IPlaylist::REPEAT_ALL;
#else
            IPlaylist::PlaylistMode eMode = IPlaylist::NORMAL;
#endif
            CPlayManager* pPM = CPlayManager::GetInstance();
            IPlaylistEntry* pEntry = pPM->GetPlaylist()->GetCurrentEntry();
            if (!pEntry || !pPM->GetPlaylist()->GetNextEntry(pEntry, eMode))
            {
                if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
                    m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_CD_TO_HD), CSystemMessageString::REALTIME_INFO);
                else
                    m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_CURRENT_PLAYLIST_TO_HD), CSystemMessageString::REALTIME_INFO);
                SetLEDState(RECORDING, false);
                m_bRipping = false;
                m_bRecording = false;

#ifdef DDOMOD_DJ_BUFFERING
                // Reestablish the DJ input stream generator
                CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif

                // Notify the UI so it can recache the top level queries.
                m_pUserInterface->NotifyMetadataUpdateEnd(m_pFatDS->GetInstanceID(), 0);
            }
        }

        DBEX( REC );
        return bShouldStop;
    }
    else
    {
        // If this was a CDDA track that was saved to HD but not chosen for recording, delete it.
        if (szStreamName && szStreamName[0] != '\0')
        {
            DEBUGP( REC, DBGLEV_INFO, "rm:Deleting temp file %s\n", szStreamName );
            if (!DeleteFile(szStreamName))
                DEBUG( REC, DBGLEV_WARNING, "Unable to delete temp file %s\n", szStreamName );
        }
        if (m_bSingle)
        {
            m_bRipping = false;
            m_bRecording = false;
            m_bSingle = false;
            SetLEDState(RECORDING, false);
            bShouldStop = true;

#ifdef DDOMOD_DJ_BUFFERING
            // Reestablish the DJ input stream generator
            CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
        }
    }

    // If we prepared a data CD track copy but didn't use it, then clean up the data.
    if (m_bCopyReady)
    {
        // Clean up.
        CleanupPreparedIsoCopy(m_pFileCopyInfo);
        m_pFileCopyInfo = 0;
        m_bCopyReady = false;
    }


    DBEX( REC );
    return bShouldStop;
}

void
CRecordingManager::NotifyStreamAbort(change_stream_event_data_t* pCSED)
{
    DBEN( REC );

    m_bPaused = false;
    // If the track was being copied to file, then delete the partial file.
    if (pCSED) {
        const char* szStreamName = pCSED->pPreviousStream->GetStreamName();
        if( szStreamName && szStreamName[0] != '\0') {
            delete m_mri.pMetadata;
            memset((void*)&m_mri, 0, sizeof(media_record_info_t));

            // This is done through the event system so the play manager will have a chance to clean up the
            // filters (and close the output file) before we delete the partial recording.
            CEventQueue::GetInstance()->PutEvent(EVENT_DELETE_PARTIAL_RECORDING, (void*)strdup_new(szStreamName));
        }
    }

    // Stop the file copier.
    CFileCopier::GetInstance()->Halt();
    CleanupCurrentIsoCopy();

    // If the file has been copied and we were waiting for playback to end before adding
    // it to the content manager, then free the memory.
    if (m_bCopyReady)
    {
        CleanupPreparedIsoCopy(m_pFileCopyInfo);
        m_pFileCopyInfo = 0;
        m_bCopyReady = false;
    }

    DBEX( REC );
}

//! Called when a partial file is to be deleted.
void
CRecordingManager::DeletePartialRecording(char* szFileName)
{
    DEBUGP( REC, DBGLEV_INFO, "rm:Deleting partial recording %s\n", szFileName );
    if (!DeleteFile(szFileName))
        DEBUG( REC, DBGLEV_WARNING, "Unable to delete file %s\n", szFileName );
    delete [] szFileName;
}


static void IdleCodingCB(CIdleCoder::job_rec_t* pJob, CIdleCoder::state_t state)
{
    switch (state)
    {
        case CIdleCoder::kStarting:
            DEBUGP( REC, DBGLEV_INFO, "rm:Starting to encode %s to %s\n", pJob->in_url, pJob->out_url );
            break;
        case CIdleCoder::kFinished:
        {
            DEBUGP( REC, DBGLEV_INFO, "rm:Finished to encoding %s to %s\n", pJob->in_url, pJob->out_url );
            if (CIdleCoder::GetInstance()->NoJobs()) {
                SetLEDState(IDLE_ENCODING, false);
            }
            // Tell the ui thread that the file is finished encoding.
            idle_coder_finish_t* pEvent = new idle_coder_finish_t;
            pEvent->szInURL = strdup_new(pJob->in_url);
            pEvent->szOutURL = strdup_new(pJob->out_url);
            CEventQueue::GetInstance()->PutEvent(EVENT_IDLE_CODING_FINISH, (void*)pEvent);
            break;
        }
#ifdef ENABLE_FIORI_TEST
        case CIdleCoder::kEncoding:
        {
            // annoying - manually blink
            static int iCount = 15;
            static bool bNewState = false;
            iCount--;
            if( !iCount ) {
                iCount = 15;
                bNewState = !bNewState;
                SetLEDState(IDLE_ENCODING, bNewState);
            }
            
            break;
        }
#endif
    }
}


//! Called from the event loop when the idle coder finished encoding a file.
//! The old entry is removed from the content manager and replaced with the new version.
void
CRecordingManager::NotifyEncodingFinished(const char* szInURL, const char* szOutURL)
{
    DBASSERT( REC, strlen(szOutURL) > 4, "Invalid destination URL: %s\n", szOutURL);
    DBASSERT( REC, !strncmp(szOutURL + strlen(szOutURL) - 3, "mxx", 3), "Invalid extension: %s\n", szOutURL);

    // Try to replace the raw file with its encoded equivalent.
    if (ProcessEncodingUpdate(szInURL, szOutURL))
    {
        // The replacement succeeded, so commit the new content database.
        DEBUGP( REC, DBGLEV_INFO, "rm:Committing db\n" );
        CommitUpdatesIfSafe();
    }
    else
    {
        // That failed, so add it to the queue to be processed later.
        AddEncodingUpdate(szInURL, szOutURL);

        // Mark the raw file as having an encoded version ready, just in case the registry
        // is corrupted.
        STAT stat;
        pc_stat(const_cast<char*>(FullFilenameFromURLInPlace(szInURL)), &stat);
        oem_setsysdate(stat.st_ctime.date, 1, 0);
        pc_touch(const_cast<char*>(FullFilenameFromURLInPlace(szInURL)));
    }
}


void
CRecordingManager::InitializeCDRip(const cdda_toc_t* pTOC)
{
#ifdef USE_FREEDB_ID_DIR

    // TODO: unhack this.
    if (int iDiscID = m_pCDDS->GetDiscID())
    {
        char szScratch[EMAXPATH];
        sprintf(szScratch, "%s/0x%x", m_pFatDS->GetContentRootDirectory(), iDiscID);
        if (!pc_isdir(szScratch))
        {
            SDBOOL bDirCreated = pc_mkdir(szScratch);
            // Assert on this, because if it fails nothing good will ever come.
            DBASSERT(REC, bDirCreated, "Unable to create directory: %s\n", szScratch);
        }

        // Create the URL format string.
        if (m_pFatDS->GetContentRootURLPrefix(szScratch, EMAXPATH))
        {
            sprintf(m_szRipToFileBase, "%s/0x%x/%%dt%%d.%%s", m_pFatDS->GetContentRootDirectory(), iDiscID);
            sprintf(m_szSaveToURLBase, "%s/0x%x/%%d.%%s", szScratch, iDiscID);
            sprintf(m_szCopyToURLBase, "%s/0x%x/", szScratch, iDiscID);
            DEBUGP( REC, DBGLEV_INFO, "rm:File base: %s\n", m_szRipToFileBase);
        }
        m_pCDDS->GetRootURLPrefix(m_szIsoRoot, PLAYLIST_STRING_SIZE);

        m_bCDInitialized = true;
    }
    else
        m_bCDInitialized = false;

#else   // USE_FREEDB_ID_DIR

    // Make the directory to store the tracks in.
    const char* szDirName = m_pCDDirGen->GetDirectory(pTOC);

    if (!pc_isdir(const_cast<char*>(szDirName)))
    {
        if (!pc_mkdir(const_cast<char*>(szDirName)))
        {
            DEBUG( REC, DBGLEV_ERROR, "Unable to create directory: %s\n", szDirName);
            m_bCDInitialized = false;
            return;
        }
    }

    // Create the URL format strings.
    char* szDirURL = URLFromPath(szDirName);

    sprintf(m_szRipToFileBase, "%s/%%dt%%d.%%s", szDirName);
    sprintf(m_szSaveToURLBase, "%s/%%d.%%s", szDirURL);
    sprintf(m_szCopyToURLBase, "%s/", szDirURL);
    m_pCDDS->GetRootURLPrefix(m_szIsoRoot, PLAYLIST_STRING_SIZE);
    DEBUGP( REC, DBGLEV_INFO, "rm:File base: %s\n", m_szRipToFileBase);

    delete [] szDirURL;

    m_bCDInitialized = true;

#endif  // USE_FREEDB_ID_DIR
}

bool
CRecordingManager::CreatePlayStreamCB( IMediaContentRecord* pContentRecord, CPlayStreamSettings* pSettings )
{
    return CRecordingManager::GetInstance()->CreatePlayStream( pContentRecord, pSettings );
}

bool
CRecordingManager::CreatePlayStream( IMediaContentRecord* pContentRecord, CPlayStreamSettings* pSettings ) 
{
    // This gets called from SetSong prior to setting the specified track

    int DSID = pContentRecord->GetDataSourceID();
    IDataSource* pDS = (CDataSourceManager::GetInstance())->GetDataSourceByID( DSID );

    // See if this track is on the CD
    if (pDS->GetClassID() == CD_DATA_SOURCE_CLASS_ID)
    {
        int iTrackIndex = m_pCDDS->GetAudioTrackIndex( pContentRecord->GetURL() );

        DBASSERT( REC, m_bCDInitialized, "CD ripping not initialized\n");

        if (iTrackIndex != -1)
        {
            // This track is CDDA.

            // Generate a filename, store it in the playstream_settings_t
            DEBUGP( REC, DBGLEV_INFO, "rm:CreatePlaystream on item %s, track %d, id %x\n", pContentRecord->GetURL(), iTrackIndex, m_pCDDS->GetDiscID() );

            int i = 0;

            m_iTempIndex = (m_iTempIndex + 1) % 10000;

            // Use .rxx extension so it won't be picked up on file scans until ripping is completed.
            sprintf(szConfigurableStreamName, m_szRipToFileBase, m_iTempIndex, iTrackIndex + 1, "rxx");
            pSettings->SetStreamName( szConfigurableStreamName );
            DEBUGP( REC, DBGLEV_INFO, "rm:CreatePlaystream: record to file %s\n", szConfigurableStreamName );

            if (!m_bRipping)
            {
                // We're not ripping, so add the waveout filter and keep the buffer sizes down.
                pSettings->m_OutputList[i++] = WAVEOUT_KEY;
                pSettings->SetBufferMultiplier(2);
            }
            else
            {
                // BufferMultiplier of 64 yields a rip rbuf of 64*4704 = 301,056
                // larger buffer multipliers yield minimal (< 2%) performance gains at the cost
                // of fewer UI updates
                pSettings->SetBufferMultiplier(64);
            }
            pSettings->m_OutputList[i++] = FATFILE_OUTPUT_ID;
            pSettings->m_OutputList[i] = 0;
            return true;
        }
    }
    
    return false;
}

// Given a URL for a track on the CD, this function returns a pointer to a recorded
// version of that track on the HD.
// If this track hasn't been recorded, then 0 is returned.
IMediaContentRecord*
CRecordingManager::FindCDTrackOnHD(const char* szURL)
{
    CDJContentManager* pCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();
    char szTestURL[256 + 20];
    IMediaContentRecord* pCR = 0;

    // Determine if this is a CDDA track or an ISO file.
    int index = m_pCDDS->GetAudioTrackIndex(szURL);

    if (index != -1)
    {
        // CDDA

        // Check for the raw record.
        sprintf(szTestURL, m_szSaveToURLBase, index + 1, "raw");
        pCR = pCM->GetMediaRecord(szTestURL);
        if (pCR)
        {
            return pCR;
        }

        // Check for the MP3 record.
        sprintf(szTestURL, m_szSaveToURLBase, index + 1, "mp3");
        pCR = pCM->GetMediaRecord(szTestURL);
        if (pCR)
        {
            return pCR;
        }
    }
    else
    {
        // ISO

        // Check for the copied record.
        strcpy(szTestURL, m_szCopyToURLBase);
        strcat(szTestURL, szURL + strlen(m_szIsoRoot));
        pCR = pCM->GetMediaRecord(szTestURL);
        if (pCR)
        {
            return pCR;
        }
    }
    return 0;
}

// Given a media content record for a track on CD or FML, this function returns a pointer to a recorded
// version of that track on the HD.
// If this track hasn't been recorded, then 0 is returned.
IMediaContentRecord*
CRecordingManager::FindTrackOnHD(IMediaContentRecord* pContentRecord)
{
    CDJContentManager* pCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();

    if (pContentRecord->GetDataSourceID() == m_pCDDS->GetInstanceID())
    {
        return FindCDTrackOnHD(pContentRecord->GetURL());
    }
    else if (pContentRecord->GetDataSourceID() == m_pNetDS->GetInstanceID())
    {
        // Create the URL for the FAT output file.
        char szFinalURL[256 + 7];
        if (GetNetCopyURL(pContentRecord, szFinalURL, 256 + 7))
        {
            return pCM->GetMediaRecord(szFinalURL);
        }
        return 0;
    }
    else if (pContentRecord->GetDataSourceID() == m_pFatDS->GetInstanceID())
    {
        return pContentRecord;
    }
    else
        return 0;
}

// Checks space and track limits on the hard drive.
// If the space limit is reached, then RECORDING_NO_SPACE is returned.
// If the track limit is reached, then RECORDING_TRACK_LIMIT_REACHED is returned.
// In both cases a message is printed in the player screen's system text.
// Otherwise, RECORDING_NO_ERROR is returned.
ERESULT
CRecordingManager::CheckSpaceAndTrackLimits(bool bPrintMessage)
{
    // make sure we have enough space to proceed
    if (CSpaceMgr::GetInstance()->Status() != SPACE_OK)
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Not enough space\n" );
        if (bPrintMessage)
        {
            m_pUserInterface->SetMessage(LS(SID_NO_SPACE_LEFT_ON_HD_FOR_RECORDING), CSystemMessageString::REALTIME_INFO);
            m_pUserInterface->SetMessage(LS(SID_DELETE_TRACKS_TO_ENABLE_RECORDING), CSystemMessageString::INFO);
        }
        return RECORDING_NO_SPACE;
    }
#ifdef MAX_HD_TRACKS
    // Don't record if the maximum number of tracks on the hard drive has been reached.
    else if (((CDJContentManager*)CPlayManager::GetInstance()->GetContentManager())->GetMediaRecordCount(m_pFatDS->GetInstanceID()) >= MAX_HD_TRACKS)
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Track limit reached\n" );
        if (bPrintMessage)
        {
            m_pUserInterface->SetMessage(LS(SID_MAXIMUM_NUMBER_OF_TRACKS_RECORDED), CSystemMessageString::REALTIME_INFO);
            m_pUserInterface->SetMessage(LS(SID_DELETE_TRACKS_TO_ENABLE_RECORDING), CSystemMessageString::INFO);
        }
        return RECORDING_TRACK_LIMIT_REACHED;
    }
#endif  // MAX_HD_TRACKS
    else
        return RECORDING_NO_ERROR;
}

// Given a media content record this function returns the index in STRING_IDS specifying why
// the track can't be recorded, or SID_EMPTY_STRING if it can be recorded.
unsigned short
CRecordingManager::CannotRecord(IMediaContentRecord* pContentRecord)
{
    CDJContentManager* pCM = (CDJContentManager*)CPlayManager::GetInstance()->GetContentManager();

    if (pContentRecord->GetDataSourceID() == m_pCDDS->GetInstanceID())
    {
        return FindCDTrackOnHD(pContentRecord->GetURL()) ? SID_TRACK_ALREADY_RECORDED : SID_EMPTY_STRING;
    }
    else if (pContentRecord->GetDataSourceID() == m_pNetDS->GetInstanceID())
    {
        // Create the URL for the FAT output file.
        char szFinalURL[256 + 7];
        if (GetNetCopyURL(pContentRecord, szFinalURL, 256 + 7))
        {
            return pCM->GetMediaRecord(szFinalURL) ? SID_TRACK_ALREADY_RECORDED : SID_EMPTY_STRING;
        }
        return SID_CANT_RECORD_INTERNET_RADIO;
    }
    else if (pContentRecord->GetDataSourceID() == m_pFatDS->GetInstanceID())
    {
        return SID_TRACK_ALREADY_RECORDED;
    }
    else
        return SID_UNABLE_TO_RECORD_TRACK;
}


///////////////////////////////////////////////////////////////////////////////
// File copying
///////////////////////////////////////////////////////////////////////////////

typedef struct copy_job_info_s
{
    unsigned long   ulTotalTrackTime;
    char*           szFinalURL;
    int             iCodecID;
    IMetadata*      pMetadata;
    bool            bScanFile;
} copy_job_info_t;

bool
CRecordingManager::PrepIsoCopy(IMediaContentRecord* pContentRecord, unsigned long ulTrackTime)
{
    // Create the URL for the FAT output file.
    const char* szURL = pContentRecord->GetURL();
    char szFatURL[strlen(m_szCopyToURLBase) + strlen(szURL) - strlen(m_szIsoRoot) + 1];
    strcpy(szFatURL, m_szCopyToURLBase);
    strcat(szFatURL, szURL + strlen(m_szIsoRoot));

    // Make the directory for the FAT file.
    if (const char* szFatFile = FullFilenameFromURLInPlace(szFatURL))
    {
        char* szFatDir = strdup_new(szFatFile);
        if (char *pch = strchr(szFatDir + 7 /* file:// */, '/'))
        {
            while (char* pchn = strchr(pch + 1, '/'))
            {
                *pchn = '\0';
                if (!pc_isdir(szFatDir))
                {
                    if (!pc_mkdir(szFatDir))
                        DEBUG( REC, DBGLEV_ERROR, "Unable to create directory: %s\n", szFatDir);
                }
                *pchn = '/';
                pch = pchn;
            }
        }
        delete [] szFatDir;

        // To prevent an aborted copy from overwriting a previously copied file,
        // first copy the file off the CD to a temp file on the hard drive.
        // After the copy is finished then change the name of the HD file.
        copy_job_info_t* pJobInfo = new copy_job_info_t;
        pJobInfo->ulTotalTrackTime = ulTrackTime;
        pJobInfo->szFinalURL = strdup_new(szFatURL);
        if (char* pch = strrchr(szFatURL, '/'))
        {
            *pch = '\0';
            char szTempFilename[16];
            m_iTempIndex = (m_iTempIndex + 1) % 10000;
            sprintf(szTempFilename, "/t%d", m_iTempIndex);
            strcat(szFatURL, szTempFilename);
        }

        // Remember the source file's metadata.
        pJobInfo->pMetadata = CPlayManager::GetInstance()->GetContentManager()->CreateMetadataRecord();
        pJobInfo->pMetadata->MergeAttributes(pContentRecord, true);
        pJobInfo->iCodecID = pContentRecord->GetCodecID();
        pJobInfo->bScanFile = false;

        // Add the file to the copier's queue.
        CFileCopier::GetInstance()->Enqueue(pContentRecord->GetURL(), szFatURL, (void*)pJobInfo);

        return true;
    }

    return false;
}

bool
CRecordingManager::PrepNetCopy(IMediaContentRecord* pContentRecord, unsigned long ulTrackTime)
{
    // Create the URL for the FAT output file.
    char szFinalURL[256 + 7];

    if (GetNetCopyURL(pContentRecord, szFinalURL, 256 + 7))
    {
        if (char* pchDir = strrchr(szFinalURL, '/'))
        {
            // Create the directory.
            *pchDir = '\0';
            VerifyOrCreateDirectory(const_cast<char*>(FullFilenameFromURLInPlace(szFinalURL)));

            // Create a temp file name.
            char szCopyToURL[256 + 7];
            m_iTempIndex = (m_iTempIndex + 1) % 10000;
            sprintf(szCopyToURL, "%s/t%d", szFinalURL, m_iTempIndex);
            *pchDir = '/';

            copy_job_info_t* pJobInfo = new copy_job_info_t;

            // Remember the source file's metadata.
            pJobInfo->pMetadata = CPlayManager::GetInstance()->GetContentManager()->CreateMetadataRecord();
            pJobInfo->pMetadata->MergeAttributes(pContentRecord, true);
            pJobInfo->iCodecID = pContentRecord->GetCodecID();
            pJobInfo->ulTotalTrackTime = ulTrackTime;
            pJobInfo->szFinalURL = strdup_new(szFinalURL);
            pJobInfo->bScanFile = true;
            DEBUGP( REC, DBGLEV_TRACE, "bt:Copying file %s to %s (final %s)\n", pContentRecord->GetURL(), szCopyToURL, szFinalURL);

            // Add the file to the copier's queue.
            CFileCopier::GetInstance()->Enqueue(pContentRecord->GetURL(), szCopyToURL, (void*)pJobInfo);

            return true;
        }
    }

    return false;
}

// Given a content record, this function constructs the directory 
bool
CRecordingManager::GetNetCopyURL(IMediaContentRecord* pContentRecord, char* szBuffer, int iBufferSize)
{
    // Create the URL for the FAT output file.
    if (!CDJPlayerState::GetInstance()->GetFatDataSource()->GetContentRootURLPrefix(szBuffer, iBufferSize))
        return false;

    strcat(szBuffer, "/");
    const char* szURL = pContentRecord->GetURL();

/*
    char szFinalURL[256 + 7];
    char* pch = strchr(szURL + 7, ':');
    strncat(szFinalURL, szURL + 7, pch - szURL - 7);
    strcat(szFinalURL, "/");
*/
    char* pchUDN = strrchr(szURL, '/');
    if (pchUDN)
    {
        char* pchMK = strrchr(szURL, '=');
        if (pchMK)
        {
            strncat(szBuffer, pchUDN + 1, pchMK - pchUDN - 1);
            strcat(szBuffer, "/");

            // Use the media key to create a subdirectory for the track.
            char szDir[10];
            sprintf(szDir, "d%d", atoi(pchMK + 1) / 100);
            strcat(szBuffer, szDir);
            strcat(szBuffer, "/");

            // Append the media key.
            strcat(szBuffer, pchMK + 1);

            // Tack on the matching codec extension.
            if (pContentRecord->GetCodecID() == m_iMP3CodecID)
                strcat(szBuffer, ".mp3");
            else if (pContentRecord->GetCodecID() == m_iWMACodecID)
                strcat(szBuffer, ".wma");
            else if (pContentRecord->GetCodecID() == m_iWAVCodecID)
                strcat(szBuffer, ".wav");
            else
                return false;

            return true;
        }
    }

    return false;
}

// Checks to see if the given content record is a radio stream.
// A record is judged as a radio stream if it comes from the net
// and doesn't match the URL pattern of an fml stream.
bool
CRecordingManager::IsRadioStream(IMediaContentRecord* pContentRecord)
{
    int blah;
    return pContentRecord && (pContentRecord->GetDataSourceID() == m_pNetDS->GetInstanceID()) &&
        (sscanf(pContentRecord->GetURL(), "http://%d.%d.%d.%d:%d/f%x=%d", &blah, &blah, &blah, &blah, &blah, &blah, &blah) != 7);
}

static void FileCopyingCB(CFileCopier::job_rec_t *pJob, CFileCopier::state_t state)
{
    switch (state)
    {
        case CFileCopier::kStarting:
            DEBUGP( REC, DBGLEV_INFO, "rm:Starting to record %s to %s\n", pJob->in_url, pJob->out_url );
            break;
        case CFileCopier::kFinishing:
        {
            DEBUGP( REC, DBGLEV_INFO, "rm:Finished recording %s to %s\n", pJob->in_url, pJob->out_url );
            copy_job_info_t* pJobInfo = (copy_job_info_t*)pJob->user_data;

            // Fake a progress update if we're ripping.
            // (If we're just recording, then the media player will handle progress updates.)
            if (CRecordingManager::GetInstance()->IsRipping())
                CEventQueue::GetInstance()->PutEvent( EVENT_STREAM_PROGRESS, (void*)(pJobInfo->ulTotalTrackTime));

            // Tell the ui thread that the file has been copied.
            file_copier_finish_t* pEvent = new file_copier_finish_t;
            pEvent->szTempURL = strdup_new(pJob->out_url);
            pEvent->szFinalURL = pJobInfo->szFinalURL;
            pEvent->pMetadata = pJobInfo->pMetadata;
            pEvent->iCodecID = pJobInfo->iCodecID;
            pEvent->bScanFile = pJobInfo->bScanFile;
            delete pJobInfo;
            CEventQueue::GetInstance()->PutEvent(EVENT_FILE_COPIER_FINISH, (void*)pEvent);
            break;
        }
        case CFileCopier::kError:
        {
            DEBUGP( REC, DBGLEV_ERROR, "rm:Failed to record track %s to %s\n", pJob->in_url, pJob->out_url );
            copy_job_info_t* pJobInfo = (copy_job_info_t*)pJob->user_data;
            
            // Fire off an event to clean up the temporary file if it was created
            CEventQueue::GetInstance()->PutEvent(EVENT_DELETE_PARTIAL_RECORDING, (void*)strdup_new(pJob->out_url));
            
            // Clean up the JobInfo struct
            delete pJobInfo->pMetadata;
            delete [] pJobInfo->szFinalURL;
            delete pJobInfo;

            // Notify the UI that we had an error
            CEventQueue::GetInstance()->PutEvent(EVENT_FILE_COPIER_ERROR, NULL);

            break;
        }
        case CFileCopier::kCopying:
        {
            // Fake a progress update if we're ripping.
            // (If we're just recording, then the media player will handle progress updates.)
            if (CRecordingManager::GetInstance()->IsRipping())
                CEventQueue::GetInstance()->PutEvent( EVENT_STREAM_PROGRESS,
                    (void*)(long)((((long long)((copy_job_info_t*)pJob->user_data)->ulTotalTrackTime) * pJob->bytes_copied) / pJob->bytes_to_copy));
            break;
        }
    }
}


//! Called from the event loop after the file copier finishes copying a file.
//! A new entry is added to the content manager.
void
CRecordingManager::NotifyCopyingFinished(file_copier_finish_t* pFileCopyInfo)
{
    // If we're ripping, then add the new content record immediately.
    // If recording, then wait until playback ends before adding content.
    if (m_bRipping)
    {
        AddCopyToContentManager(pFileCopyInfo);

#ifdef PROFILE_MEM_USAGE
    if (s_fordblks_start)
    {
        ++s_ripped_count;
        struct mallinfo mem_info = mallinfo();
        diag_printf("***\n* Rip progress:\n* Bytes: %d Ripped: %d Bytes per ripped: %d\n***\n",
            s_fordblks_start - mem_info.fordblks, s_ripped_count, (s_fordblks_start - mem_info.fordblks) / s_ripped_count);
    }
#endif  // PROFILE_MEM_USAGE

        // If in single-track ripping mode, then stop ripping.
        if (m_bSingle)
        {
            m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_TRACK_TO_HD), CSystemMessageString::REALTIME_INFO);
            m_bRipping = false;
            m_bRecording = false;
            m_bSingle = false;
            CFileCopier::GetInstance()->Halt();
            SetLEDState(RECORDING, false);

#ifdef DDOMOD_DJ_BUFFERING
            // Reestablish the DJ input stream generator
            CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
        }
        // We're ripping the entire CD.
        else
        {
            // Check space and track limits before proceeding.
            if (FAILED(CheckSpaceAndTrackLimits()))
            {
                DEBUGP( REC , DBGLEV_INFO, "rm:HDLim-StopRipIso\n");
                StopRipping(false);
            }

            CPlayManager* pPM = CPlayManager::GetInstance();
            IPlaylistEntry* pEntry = pPM->GetPlaylist()->GetCurrentEntry();
            // If we've ripped the entire CD, then stop.
#ifdef ENABLE_FIORI_TEST
            if (!pEntry || !pPM->GetPlaylist()->GetNextEntry(pEntry, IPlaylist::REPEAT_ALL))
#else
            if (!pEntry || !pPM->GetPlaylist()->GetNextEntry(pEntry, IPlaylist::NORMAL))
#endif
            {
                if (CDJPlayerState::GetInstance()->GetSource() == CDJPlayerState::CD)
                    m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_CD_TO_HD), CSystemMessageString::REALTIME_INFO);
                else
                    m_pUserInterface->SetMessage(LS(SID_FINISHED_RECORDING_CURRENT_PLAYLIST_TO_HD), CSystemMessageString::REALTIME_INFO);
                SetLEDState(RECORDING, false);
                m_bRipping = false;
                m_bRecording = false;

#ifdef DDOMOD_DJ_BUFFERING
                // Reestablish the DJ input stream generator
                CMediaPlayer::GetInstance()->SetCreateInputStreamFunction( &CreateBufferInStream );
#endif
            }
        }
    }
    else
    {
        if (m_bRecordingEnabled)
        {
            m_bCopyReady = true;
            m_pFileCopyInfo = pFileCopyInfo;
        }
        else
        {
            // Clean up.
            CleanupPreparedIsoCopy(pFileCopyInfo);
            m_bCopyReady = false;
        }
    }
}

// Clears the file copy job queue, deletes partial files, cleans up memory.
void
CRecordingManager::CleanupCurrentIsoCopy()
{
    CFileCopier* pFC = CFileCopier::GetInstance();
    while (!pFC->NoJobs())
    {
        // Delete any partially copied files.
        CFileCopier::job_rec_t* pJob = pFC->Dequeue();
        copy_job_info_t* pCopyInfo = (copy_job_info_t*)pJob->user_data;

        DeleteFile(FullFilenameFromURLInPlace(pJob->out_url));

        // Clean up memory.
        delete pCopyInfo->pMetadata;
        delete [] pCopyInfo->szFinalURL;
        delete pCopyInfo;

        delete pJob;
    }
}

void
CRecordingManager::CleanupPreparedIsoCopy(file_copier_finish_t* pFileCopyInfo)
{
    // Delete the copied file.
    DeleteFile(FullFilenameFromURLInPlace(pFileCopyInfo->szTempURL));

    // Clean up the event struct.
    delete [] pFileCopyInfo->szFinalURL;
    delete [] pFileCopyInfo->szTempURL;
    delete pFileCopyInfo->pMetadata;
    delete pFileCopyInfo;
}


// Renames a temp copy file to the final filename and adds it to the content manager.
bool
CRecordingManager::AddCopyToContentManager(file_copier_finish_t* pFileCopyInfo)
{
    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();

    // Delete any old files with the same name and move the new file on top.
    DeleteFile(FullFilenameFromURLInPlace(pFileCopyInfo->szFinalURL));
    if (!MoveAndVerify(FullFilenameFromURLInPlace(pFileCopyInfo->szTempURL),
        FullFilenameFromURLInPlace(pFileCopyInfo->szFinalURL)))
    {
        DEBUG( REC, DBGLEV_WARNING, "Unable to move file %s to %s: Error %d: %s\n", pFileCopyInfo->szTempURL, pFileCopyInfo->szFinalURL, pc_get_error(0), GetFatErrorString(pc_get_error(0)) );

        // Well, we're boned.  The file can't be moved, and the original file is probably dead.
        // So if there was a content record to match this file, get rid of it.
        // TODO: apologize to the user
        if (IMediaContentRecord* pCR = pCM->GetMediaRecord(pFileCopyInfo->szFinalURL))
            pCM->DeleteMediaRecord(pCR);

        // Clean up.
        CleanupPreparedIsoCopy(pFileCopyInfo);

        return false;
    }
    else
    {
        media_record_info_t mri;

        mri.szURL = pFileCopyInfo->szFinalURL;
        mri.bVerified = true;
        mri.iCodecID = pFileCopyInfo->iCodecID;
        mri.iDataSourceID = m_pFatDS->GetInstanceID();
        mri.pMetadata = pFileCopyInfo->pMetadata;

        // If there's no metadata provided then search the file for some.
        if (pFileCopyInfo->bScanFile)
        {
            if (ICodec* pCodec = CCodecManager::GetInstance()->FindCodec(mri.iCodecID))
            {
                IInputStream* ffis = m_pFatDS->OpenInputStream(mri.szURL);

                pCodec->GetMetadata(m_pFatDS, mri.pMetadata, ffis);

                delete ffis;
                delete pCodec;
            }
        }

#ifdef DDOMOD_DJ_METADATAFILETAG
        // If the copied file doesn't naturally have a metadata tag (i.e., PCM or WAV), then
        // add our own MD tag format.
        if ((pFileCopyInfo->iCodecID == m_iPCMCodecID) || (pFileCopyInfo->iCodecID == m_iWAVCodecID))
            CMetadataFileTag::GetInstance()->UpdateTag(mri.szURL, mri.pMetadata);
#endif
        // Create a new content record for the encoded file.
        CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);
        pCM->AddMediaRecord(mri);
        // Notify the UI so it can recache the top level queries.
        m_pUserInterface->NotifyMetadataUpdateEnd(m_pFatDS->GetInstanceID(), 0);

        // Clean up the event struct.
        delete [] pFileCopyInfo->szFinalURL;
        delete [] pFileCopyInfo->szTempURL;
        delete pFileCopyInfo;

        return true;
    }
}


///////////////////////////////////////////////////////////////////////////////
// Encoding updates
///////////////////////////////////////////////////////////////////////////////

static const RegKey EncodingUpdateListRegKey = REGKEY_CREATE( DJ_ENCODING_UPDATE_LIST_KEY_TYPE, DJ_ENCODING_UPDATE_LIST_KEY_NAME );

#define REC_REGKEY_RAW_URL(x) REGKEY_CREATE( DJ_ENCODING_UPDATE_LIST_KEY_TYPE, 2 * (x) + DJ_ENCODING_UPDATE_LIST_KEY_NAME + 1)
#define REC_REGKEY_ENCODED_URL(x) REGKEY_CREATE( DJ_ENCODING_UPDATE_LIST_KEY_TYPE, 2 * (x) + 1 + DJ_ENCODING_UPDATE_LIST_KEY_NAME + 1)

void
CRecordingManager::SaveEncodingUpdateList()
{
    CRegistry* pRegistry = CRegistry::GetInstance();

    // Get the old record count so extra records can be deleted.
    unsigned int iPreviousUpdateCount = (unsigned int)pRegistry->FindByKey(EncodingUpdateListRegKey);
    DEBUGP( REC, DBGLEV_INFO, "rm:Previous count: %d records\n", iPreviousUpdateCount);

    // Save the record count.
    pRegistry->RemoveItem(EncodingUpdateListRegKey);
    DEBUGP( REC, DBGLEV_INFO, "rm:%d records\n", m_slEncodeUpdates.Size());
    pRegistry->AddItem(EncodingUpdateListRegKey, (void*)m_slEncodeUpdates.Size(), REGFLAG_PERSISTENT, sizeof(int));

    // Save the records.
    unsigned int i = 0;
    for (SimpleListIterator<encode_update_t> it = m_slEncodeUpdates.GetHead(); it != m_slEncodeUpdates.GetEnd(); ++i, ++it)
    {
        // Erase any previous jobs in the registry.
        if (char* szUrl = (char*)pRegistry->FindByKey(REC_REGKEY_RAW_URL(i)))
        {
            pRegistry->RemoveItem(REC_REGKEY_RAW_URL(i));
            delete [] szUrl;
        }
        if (char* szUrl = (char*)pRegistry->FindByKey(REC_REGKEY_ENCODED_URL(i)))
        {
            pRegistry->RemoveItem(REC_REGKEY_ENCODED_URL(i));
            delete [] szUrl;
        }
        
        // Save this record.
        DEBUGP( REC, DBGLEV_INFO, "rm:Record %d raw url: %s\n", i, (*it).szRawURL);
        pRegistry->AddItem(REC_REGKEY_RAW_URL(i), (void*)strdup_new((*it).szRawURL), REGFLAG_PERSISTENT, strlen((*it).szRawURL) + 1);
        DEBUGP( REC, DBGLEV_INFO, "rm:Record %d encoded url: %s\n", i, (*it).szEncodedURL);
        pRegistry->AddItem(REC_REGKEY_ENCODED_URL(i), (void*)strdup_new((*it).szEncodedURL), REGFLAG_PERSISTENT, strlen((*it).szEncodedURL) + 1);
    }

    // Erase old records.
    while (i < iPreviousUpdateCount)
    {
        if (char* szUrl = (char*)pRegistry->FindByKey(REC_REGKEY_RAW_URL(i)))
        {
            pRegistry->RemoveItem(REC_REGKEY_RAW_URL(i));
            delete [] szUrl;
        }
        if (char* szUrl = (char*)pRegistry->FindByKey(REC_REGKEY_ENCODED_URL(i)))
        {
            pRegistry->RemoveItem(REC_REGKEY_ENCODED_URL(i));
            delete [] szUrl;
        }
        ++i;
    }

    // Save the registry.
    CDJPlayerState::GetInstance()->SaveRegistry();
}

void
CRecordingManager::LoadEncodingUpdateList()
{
    CRegistry* pRegistry = CRegistry::GetInstance();

    // Load the record count.
    unsigned int iUpdateCount = (unsigned int)pRegistry->FindByKey(EncodingUpdateListRegKey);
    DEBUGP( REC, DBGLEV_INFO, "rm:%d records\n", iUpdateCount);
    if (!iUpdateCount)
        // No records, so stop loading.
        return;
    
    // Clear the old update list.
    while (!m_slEncodeUpdates.IsEmpty())
    {
        encode_update_t update = m_slEncodeUpdates.PopFront();
        delete [] update.szRawURL;
        delete [] update.szEncodedURL;
    }

    // Load the records.
    for (unsigned int i = 0; i < iUpdateCount; ++i)
    {
        // Grab the input url
        char* szRawUrl = (char*)pRegistry->FindByKey(REC_REGKEY_RAW_URL(i));
        if (!szRawUrl)
        {
            DEBUG( REC, DBGLEV_ERROR, "Error reading raw URL for job %d\n", i);
            continue;
        }
        DEBUGP( REC, DBGLEV_INFO, "rm:Job %d raw url: %s\n", i, szRawUrl);
        
        // Grab the output url
        char* szEncodedUrl = (char*)pRegistry->FindByKey(REC_REGKEY_ENCODED_URL(i));
        if (!szEncodedUrl)
        {
            DEBUG( REC, DBGLEV_ERROR, "Error reading out_url for job %d\n", i);
            continue;
        }
        DEBUGP( REC, DBGLEV_INFO, "rm:Job %d encoded url: %s\n", i, szEncodedUrl);
        
        // Add the job to the queue.
        AddEncodingUpdate(szRawUrl, szEncodedUrl);
    }
}

// Called to attempt to replace a raw file with its encoded equivalent.
// If the raw file is in the current playlist then nothing is done and false is returned.
// If the raw file's content record isn't in the content manager then assume the file has
// been deleted.  Delete the encoded file and return true.
// Otherwise, remove the raw content record from the content manager and replace it with
// a record for the encoded file.  Delete the raw file and return true.
bool
CRecordingManager::ProcessEncodingUpdate(const char* szRawURL, const char* szEncodedURL)
{
    DEBUG( REC, DBGLEV_TRACE, "Attempting to replace raw URL %s with encoded URL %s\n", szRawURL, szEncodedURL);

    IContentManager* pCM = CPlayManager::GetInstance()->GetContentManager();
    if (pCM)
    {
        // Try to find the raw content record.
        if (IMediaContentRecord* pContentRecord = pCM->GetMediaRecord(szRawURL))
        {
            // Make sure the raw file isn't in the current playlist.
            IPlaylist* pPlaylist = CPlayManager::GetInstance()->GetPlaylist();
            if (pPlaylist)
            {
                IPlaylistEntry* pEntry = pPlaylist->GetEntry(0, IPlaylist::NORMAL);
                while (pEntry)
                {
                    if (pEntry->GetContentRecord() == pContentRecord)
                    {
                        // The raw file is in the current playlist.  Try again later.
                        DEBUG( REC, DBGLEV_TRACE, "Raw URL %s is in the current playlist\n", szRawURL);
                        return false;
                    }
                    pEntry = pPlaylist->GetNextEntry(pEntry, IPlaylist::NORMAL);
                }
            }

            // Delete the unencoded file.
            if (!DeleteFile(FullFilenameFromURLInPlace(szRawURL)))
            {
                DEBUG( REC, DBGLEV_WARNING, "Unable to delete raw file %s\n", szRawURL );
                return false;
            }

            // Create a media record for the encoded file.
            media_record_info_t mri;

            mri.szURL = strdup_new(szEncodedURL);
            // Change the temporary "mxx" extension to "mp3".
            strcpy(mri.szURL + strlen(mri.szURL) - 3, "mp3");
            mri.iCodecID = m_iMP3CodecID;
            mri.iDataSourceID = m_pFatDS->GetInstanceID();
            mri.bVerified = true;
            mri.pMetadata = pContentRecord->Copy();

            // Delete any existing encoded file and move the temp .mxx file to its final .mp3 form.
            DeleteFile(FullFilenameFromURLInPlace(mri.szURL));
            MoveAndVerify(FullFilenameFromURLInPlace(szEncodedURL), FullFilenameFromURLInPlace(mri.szURL));

            // Create a new content record for the encoded file.
            CProgressWatcher::GetInstance()->SetTask(TASK_CONTENT_UPDATE);
            IMediaContentRecord* pNewContentRecord = pCM->AddMediaRecord(mri);

#ifdef DDOMOD_DJ_METADATAFILETAG
            CMetadataFileTag::GetInstance()->UpdateTag(mri.szURL, pNewContentRecord);
#endif // DDOMOD_DJ_METADATAFILETAG

            // Remove the unencoded content record from the content manager.
            pCM->DeleteMediaRecord(pContentRecord);

            // Notify the UI so it can recache the top level queries.
            m_pUserInterface->NotifyMetadataUpdateEnd(m_pFatDS->GetInstanceID(), 0);

        }
        else
        {
            // The content record for the raw file wasn't found, so assume it's been deleted.
            // Delete the encoded version, too.
            DEBUGP( REC, DBGLEV_INFO, "rm:Missing content record for %s, so deleting %s\n", szRawURL, szEncodedURL );
            if (!DeleteFile(FullFilenameFromURLInPlace(szEncodedURL)))
            {
                DEBUG( REC, DBGLEV_WARNING, "Unable to delete encoded file %s\n", szRawURL );
            }
        }
    }

    return true;
}

// Called when encoding finishes yet the raw file can't be deleted yet.
// Adds the URLs of the raw file and the encoded file to the update list for later processing.
void
CRecordingManager::AddEncodingUpdate(const char* szInURL, const char* szOutURL)
{
    encode_update_t enc;
    enc.szRawURL = strdup_new(szInURL);
    enc.szEncodedURL = strdup_new(szOutURL);

    m_slEncodeUpdates.PushBack(enc);

    // Save the updated list to the registry.
    SaveEncodingUpdateList();
}

// Clears the list of pending encoded file updates.
void
CRecordingManager::ClearEncodingUpdates()
{
    while (!m_slEncodeUpdates.IsEmpty())
    {
        encode_update_t upd = m_slEncodeUpdates.PopFront();

        delete [] upd.szEncodedURL;
        delete [] upd.szRawURL;
    }
}

// Traverses the list of encoding updates and tries to replace raw files with encoded ones.
void
CRecordingManager::ProcessEncodingUpdates()
{
    DBEN( REC );

    SimpleListIterator<encode_update_t> it = m_slEncodeUpdates.GetHead();
    bool bUpdated = false;

    while (it != m_slEncodeUpdates.GetEnd())
    {
        if (ProcessEncodingUpdate((*it).szRawURL, (*it).szEncodedURL))
        {
            // This update was processed, so remove this node from the list.
            SimpleListIterator<encode_update_t> itDel = it;
            ++it;
            delete [] (*itDel).szEncodedURL;
            delete [] (*itDel).szRawURL;
            m_slEncodeUpdates.Remove(itDel);

            // Remember to save the updated list to the registry.
            bUpdated = true;
        }
        else
            ++it;
    }

    // At least one node in the list was processed, so save the updated registry.
    if (bUpdated)
    {
        DEBUGP( REC, DBGLEV_INFO, "rm:Saving new encoding update list\n");
        SaveEncodingUpdateList();

        // Commit the database.
        DEBUGP( REC, DBGLEV_INFO, "rm:Committing db\n" );
        CommitUpdatesIfSafe();
    }

    DBEX( REC );
}

