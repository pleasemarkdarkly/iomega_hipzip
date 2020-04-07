//
// The filename format of line-in tracks is:
//      "a:\{content dir}\r\s{session id}\d{dir hash}\r{recording id}.{codec extension}"
// where
//      dir hash = recording id div 100, so only 100 items max can be in a dir
//

// general
#include <_modules.h>

#include <codec/mp3/pem/codec.h>
#include <codec/mp3/pem/pem.h>
#include <core/playmanager/PlayManager.h>
#include <core/events/SystemEvents.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <main/main/AppSettings.h>
#include <main/main/FatHelper.h>
#include <main/main/DJEvents.h>
#include <main/main/EventTypes.h>
#include <main/main/RecordingEvents.h>
#include <main/main/SpaceMgr.h>
#include <main/ui/PlayerScreen.h>
#include <main/ui/common/UserInterface.h>   // send system message text
#include <main/ui/Strings.hpp>
#include <datasource/fatdatasource/FatDataSource.h>
#include <devs/audio/dai.h>
#include <devs/audio/cs5332.h>
#include <stdio.h>      /* sprintf */
#include <util/eventq/EventQueueAPI.h>     // for main event queue
#ifdef DDOMOD_DJ_METADATAFILETAG
#include <main/metadata/metadatafiletag/MetadataFileTag.h>
#endif
// self
#include <main/main/LineRecorder.h>
// registry
#include <util/registry/Registry.h>

#include <main/main/DJPlayerState.h>
#include <codec/codecmanager/CodecManager.h>

static const RegKey RecorderRegKey = REGKEY_CREATE( LINE_RECORDER_REGISTRY_KEY_TYPE, LINE_RECORDER_REGISTRY_KEY_NAME );
// debugging
#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_LINE_RECORDER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_LINE_RECORDER);  // debugging prefix : (10) lr

#define RECORDER_STACK_SIZE (32768)
#define RECORDER_THREAD_PRIORITY (8)
#define SPACE_MANAGER_ENABLED (1)
#define RECORDER_MD_MAXCHARS (32)

#define RECORDER_MAX_DIR_ENTRIES    (100)

CLineRecorder* CLineRecorder::m_pInstance = 0;

extern unsigned int DAIOverflowCounter;
extern const char *gc_szID3v1GenreTable[];

struct id3tag {
    char tag[3];
    char title[30];
    char artist[30];
    char album[30];
    char year[4];
    char comment[30];
    unsigned char genre;
};

CLineRecorder* CLineRecorder::GetInstance()
{
    if (!m_pInstance)
        m_pInstance = new CLineRecorder;
    return m_pInstance;
}

void CLineRecorder::Destroy()
{
    delete m_pInstance;
    m_pInstance = 0;
}

// (epg,4/11/2002): TODO: track the album position and additionals in mft
CLineRecorder::CLineRecorder() : m_nReturn(0), m_bInSession(false), m_bReRecording(false), m_nCrntSession(0), 
                                 m_nCrntTrack(0), m_nThrottledCrntTrack(0), m_bCompressed(true), m_nBitrate(192), 
                                 m_nGain(0), m_bMicBoostEnabled(false),
                                 m_nSampFreq(44100), m_nSecondsRecorded(0), m_nSamplesRecorded(0),
                                 m_szTrackURLPendingCoreAddition(0),
                                 m_nMaxMinutes(0)
{
    m_pEventQueue  = CEventQueue::GetInstance();
    m_pPEM = CPEMWrapper::GetInstance();
    m_pThreadStack = new char[RECORDER_STACK_SIZE+1];
   	cyg_flag_init( &m_flgControl );
	m_pthdWork = new Cyg_Thread( RECORDER_THREAD_PRIORITY, // thread priority
								&CLineRecorder::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"Line in Recorder",
								(CYG_ADDRESS)m_pThreadStack,
								(cyg_ucount32)RECORDER_STACK_SIZE);
   	cyg_mutex_init( &m_mutRcrdr );
    m_pthdWork->resume();  
}

CLineRecorder::~CLineRecorder()
{
    delete m_pPEM;
    m_pPEM = NULL;
  	cyg_flag_destroy( &m_flgControl );
	delete m_pthdWork;
    delete [] m_pThreadStack;
  	cyg_mutex_destroy( &m_mutRcrdr ); 
}

bool CLineRecorder::InSession()
{
    return m_bInSession; 
}

// check if there are any preexisting mp3 files in the candidate session directory.
// (epg,6/25/2002): warning! expects param url to have extra space to add in a wildcard suffix.
bool CLineRecorder::SessionDirNonEmpty(char* url)
{
    bool bEmpty = true;

    DSTAT statobj;

    // Check for content subdirectories.
    int nLen = strlen(url);
    strcat(url,"/*.*");

    if( pc_gfirst( &statobj, url ) )
    {
        do
        {
            if( statobj.fname[0] == '.' )
            {
		        continue;
		    }
            else if( statobj.fattribute & ADIRENT )
            {
                DSTAT statfile;

                // lop off the wildcard
                url[nLen] = 0;

                // check for mp3 files
                if (strlen(statobj.fext))
                    sprintf( url + nLen, "/%s.%s/*.mp3", statobj.fname, statobj.fext );
                else
                    sprintf( url + nLen, "/%s/*.mp3", statobj.fname );

                if (pc_gfirst(&statfile, url))
                {
                    pc_gdone(&statfile);
                    bEmpty = false;
                }

                // lop off the wildcard
                url[nLen] = 0;

                // check for raw files    
                if (strlen(statobj.fext))
                    sprintf( url + nLen, "/%s.%s/*.raw", statobj.fname, statobj.fext );
                else
                    sprintf( url + nLen, "/%s/*.raw", statobj.fname );

                if (pc_gfirst(&statfile, url))
                {
                    pc_gdone(&statfile);
                    bEmpty = false;
                }
            }
        } while (bEmpty && pc_gnext(&statobj));

    	pc_gdone( &statobj );
	}

    // lop off the wildcard
    url[nLen] = 0;

    return !bEmpty;
}

int CLineRecorder::OpenSession()
{
    if (m_bInSession)
        return 0;
    RestoreFromRegistry();
    char url[PLAYLIST_STRING_SIZE];
    VerifyOrCreateDirectory(RECORDINGS_PATH);
    bool bSessionFound = false;
    int nStartSession = m_nCrntSession;
    while (!bSessionFound)
    {
        ++m_nCrntSession;
        if (m_nCrntSession == nStartSession)
            break;
        // make sure we have the suitable parent directories where we will put the file
        VerifyOrCreateDirectory(OutputSessionDir(url));
        if (SessionDirNonEmpty(url))
            continue;
        bSessionFound = true;
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->ClearTrack();
        m_bInSession = true;
        SaveToRegistry();
    }
    if (bSessionFound)
        return m_nCrntSession;
    return m_nCrntSession = -1;
}

int CLineRecorder::CloseSession()
{
    if (!m_bInSession)
        return 0;
    m_bInSession = false;
    m_nCrntTrack = 0;
    SaveToRegistry();
    return m_nCrntSession;
}

extern "C"
{
extern SDVOID oem_setsysdate(
    UINT16 tdate,
    UINT16 ttime,
    UTINY  ttime_tenths);
}

IOutputStream* CLineRecorder::PrepOutStream()
{
    // Add the bitrate to the stream, just to make sure that raw files don't get added
    // to the idle coder on a content scan.
    oem_setsysdate(CDJPlayerState::GetInstance()->GetEncodeBitrate(), 0, 0);
    IOutputStream* out = new CFatFileOutputStream;
    char url[PLAYLIST_STRING_SIZE];
    if (SUCCEEDED(out->Open(OutputURL(url))))
        return out;
    delete out;
    return NULL;
}

int CLineRecorder::InitInputSystem()
{
    InitDAIForRecording();
    return 0;
}

int CLineRecorder::InitOutputSystem()
{
    InitBuffers();
    return 0;
}

void CLineRecorder::InitPlayerScreenTextFields()
{
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    TCHAR tszTemp[RECORDER_MD_MAXCHARS];
    char szTemp[RECORDER_MD_MAXCHARS];
    // artist
    CharToTchar(tszTemp,PopulateSessionString(szTemp));
    ps->SetArtistText(tszTemp);
    // track
    CharToTchar(tszTemp,PopulateTrackString(szTemp));
    ps->SetTrackText(tszTemp);
    // record icon
    ps->SetControlSymbol(CPlayerScreen::RECORD);
    // initial time
    ps->SetTime(0);
    // draw
    ps->Invalidate(ps->mReal);
    ps->Draw();
}

void InitSRAM()
{
    memset( (void*)SRAM_START, 0, SRAM_SIZE);
}

int CLineRecorder::StartRecordingAux()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:strtRcrdAux\n");
    // start a timer to flutter the control symbol on the playerscren.
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    // zero out track level counters
    m_nSecondsRecorded = 0;
    m_nSamplesRecorded = 0;
    m_nSecondsRecordedDiv60 = 0;
    m_nMinutesRecorded = 0;
    // increment track counter, if we're not using a throttled track number
    if (m_nThrottledCrntTrack < 1)
        ++m_nCrntTrack;
    // prep output
    InitOutputSystem();
    m_pOutStream = PrepOutStream();
    DBASSERT(DBG_LINE_RECORDER, m_pOutStream != NULL, "failed to open output stream");
    // populate the player screen with info about the currently recording track.
    InitPlayerScreenTextFields();
	NotifyUserRecordingStarted();
    // init pem
    if (m_bCompressed)
        m_pPEM->Start(m_nBitrate, m_tEncodeBufs.rdIn, m_tEncodeBufs.wrOut);
    // prep input
    InitInputSystem();
    ps->SetControlSymbol(CPlayerScreen::RECORD);
    return 0;
}

// using the given url, lookup the name and longName, and allocate them into the given out vars.
int LookupFilenames(const char* url, char** pszName, char** szLongName)
{
    DSTAT stat;
    if (pc_gfirst(&stat, (char*)url))
    {
        TrimInPlace(stat.fname);
        TrimInPlace(stat.fext);
        if (strlen(stat.fext))
        {
            *pszName = (char*)malloc(strlen(stat.fname) + 2 + strlen(stat.fext));
            sprintf( *pszName, "%s.%s", stat.fname, stat.fext );
        }
        else
        {
            *pszName = (char*)malloc(strlen(stat.fname) + 1);
            sprintf( *pszName, "%s", stat.fname );
        }
        *szLongName = MakeLongFilename(stat);
        pc_gdone(&stat);
    }
    return 0;
}

char* CLineRecorder::PopulateSessionString( char* szSession )
{
    sprintf (szSession, "%s%03d", RECORDING_SESSION_NAME_STEM, m_nCrntSession);
    return szSession;
}

int CLineRecorder::GetActiveTrackNumber()
{
    int nActiveTrack;
    if (m_nThrottledCrntTrack > 0)
    {
        nActiveTrack = m_nThrottledCrntTrack;
    }
    else
    {
        nActiveTrack = m_nCrntTrack;
    }
    return nActiveTrack;
}

char* CLineRecorder::PopulateTrackString( char* szTrack )
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:PopTrackStr\n"); 
    int nActiveTrack = GetActiveTrackNumber();
    sprintf (szTrack, "%s%02d", RECORDING_TRACK_NAME_STEM, nActiveTrack);
    return szTrack;
}

int CLineRecorder::AddMetadataFileTag(IOutputStream* pStream)
{
#ifdef DDOMOD_DJ_METADATAFILETAG
    IMetadata* pMetadata = CPlayManager::GetInstance()->GetContentManager()->CreateMetadataRecord();
    char szBuf[RECORDER_MD_MAXCHARS];
    TCHAR tszBuf[RECORDER_MD_MAXCHARS];
    PopulateSessionString( szBuf );
    CharToTchar(tszBuf, szBuf);
    // artist (session name)
    pMetadata->SetAttribute(MDA_ARTIST, tszBuf);
    // album (session name)
    pMetadata->SetAttribute(MDA_ALBUM, tszBuf);
    // genre ("Recordings")
    CharToTchar(tszBuf, RECORDING_GENRE_TEXT);
    pMetadata->SetAttribute(MDA_GENRE, tszBuf);
    // title ("Recording 01")
    PopulateTrackString(szBuf);
    CharToTchar(tszBuf,szBuf);
    pMetadata->SetAttribute(MDA_TITLE, tszBuf);
    // album track position
    int nAlbumPos;
    if (m_nThrottledCrntTrack)
        nAlbumPos = m_nThrottledCrntTrack;
    else
        nAlbumPos = m_nCrntTrack;
    pMetadata->SetAttribute(MDA_ALBUM_TRACK_NUMBER, (void*)nAlbumPos);

    CFatFile* pFile = ((CFatFileOutputStream*)pStream)->m_pFile;
    CMetadataFileTag::GetInstance()->UpdateTag(pFile, pMetadata);
#endif
    // (epg,7/2/2002): ideally we'd return the size of the tag appended, but there's no real need, and that info stopped
    // propagating a couple levels into UpdateTag (my slop).
    return 0;
}

int CLineRecorder::AppendID3v1Tag(IOutputStream* pStream)
{
    int nSize = pStream->Seek(IOutputStream::SeekEnd, 0);
    if (!nSize)
        return 0;
    id3tag tag;
    memset(&tag, 0, sizeof(id3tag));
    // genre
    tag.genre = RECORDING_GENRE_V1;
    // album
    char temp[RECORDER_MD_MAXCHARS];
    PopulateSessionString( temp );
    strcpy( (char*)tag.album, temp);
    // artist
    strcpy( (char*)tag.artist, temp);
    // title
    PopulateTrackString( temp );
    strcpy( (char*)tag.title, temp);
    // year
    strcpy( (char*)tag.year, RECORDING_YEAR);
    // comment
    strcpy( (char*)tag.comment, RECORDING_COMMENT);
    // marker tag
    memcpy( (void*)&(tag.tag), "TAG", 3);
    pStream->Write((void*)&tag, sizeof(id3tag));
    return nSize + sizeof(id3tag);
}

int CLineRecorder::StopRecordingAux()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:stpRcrdAux\n");
    // flush all output to disk, and add a touch of silence for good measure.
    if (m_bCompressed)
        FinishCompressedOutput();
    // report on overflow stats
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "%d dropped ", DAIOverflowCounter);
	DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "%d/", m_tDAIState.idxMinBuf);
	DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "%d (min/max) free buffers\n", m_tDAIState.idxMaxBuf);
    // add an MFT to the end of the file
    AddMetadataFileTag(m_pOutStream);
    // add an id3v1 tag to the end of the file
    // (epg,6/25/2002): eek, don't think we should, this would bury the MFT by placing a v1 
    // tag after the mft.
    //int nFileSize = AppendID3v1Tag(m_pOutStream);
    int nFileSize = m_pOutStream->Seek(IOutputStream::SeekEnd, 0);
    if (m_szTrackURLPendingCoreAddition != NULL) {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "LR:already a track url pending core addition!\n"); 
    }
    else
    {
        if (!m_bReRecording)
        {
            // this url will be reclaimed by the content manager, using free(), thus malloc.
            m_szTrackURLPendingCoreAddition = (char*)malloc(PLAYLIST_STRING_SIZE);
            strcpy (m_szTrackURLPendingCoreAddition,"file://");
            OutputURL((char*)FullFilenameFromURLInPlace(m_szTrackURLPendingCoreAddition));
        }
    }
    // kill out stream
    m_pOutStream->Close();
    delete m_pOutStream;
    m_pOutStream = NULL;
    FreeBuffers();
    return nFileSize;
}

int CLineRecorder::AddRecordingToPlayerCoreAux()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:addToCoreAux\n");
    media_record_info_t* pMRI = new media_record_info_t;
    pMRI->bVerified = true;
    if (m_bCompressed)
        pMRI->iCodecID = CCodecManager::GetInstance()->FindCodecID("mp3");
    else
        pMRI->iCodecID = CCodecManager::GetInstance()->FindCodecID("raw");
    pMRI->iDataSourceID = CDJPlayerState::GetInstance()->GetFatDataSource()->GetInstanceID();

    if ((pMRI->pMetadata = CPlayManager::GetInstance()->GetContentManager()->CreateMetadataRecord()))
    {
        TCHAR tbuf[RECORDER_MD_MAXCHARS];
        // genre
        pMRI->pMetadata->SetAttribute(MDA_GENRE, (void*)CharToTchar(tbuf, RECORDING_GENRE_TEXT));
        // album
        char temp[RECORDER_MD_MAXCHARS];
        PopulateSessionString( temp );
        CharToTcharN(tbuf,temp,RECORDER_MD_MAXCHARS - 1);
        pMRI->pMetadata->SetAttribute(MDA_ALBUM, (void*)tbuf);
        // artist
        pMRI->pMetadata->SetAttribute(MDA_ARTIST, (void*)tbuf);
        // title
        PopulateTrackString( temp );
        pMRI->pMetadata->SetAttribute(MDA_TITLE, (void*)CharToTcharN(tbuf, temp, RECORDER_MD_MAXCHARS - 1));
        // album track number
        pMRI->pMetadata->SetAttribute(MDA_ALBUM_TRACK_NUMBER, (void*)GetActiveTrackNumber());
    }
    
    if (m_szTrackURLPendingCoreAddition == NULL) {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "LR:no pending core addition to addToCore!\n"); 
        return 0;
    }

    pMRI->szURL = m_szTrackURLPendingCoreAddition;
    m_szTrackURLPendingCoreAddition = NULL;
    
    // send the update as an event
    m_pEventQueue->PutEvent( EVENT_LINERECORDER_CONTENT_ADDITION, (void*) pMRI );
    return 0;
}

// should be called immediately after stopping recording
int CLineRecorder::AddRecordingToPlayerCore()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:addRecToCore\n");
    return CallThreadFunction(m_tFlagVals.addToCore);
}

int CLineRecorder::CallThreadFunction(int iFlagVal)
{
    cyg_mutex_lock( &m_mutRcrdr );
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:callThrdFn %d\n",iFlagVal);
	cyg_flag_wait( &m_flgControl, m_tFlagVals.done, CYG_FLAG_WAITMODE_OR );
	cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.done );
	cyg_flag_setbits( &m_flgControl, iFlagVal );
	cyg_flag_wait( &m_flgControl, m_tFlagVals.done, CYG_FLAG_WAITMODE_OR );
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:callThrdFn done\n",iFlagVal);
	cyg_mutex_unlock( &m_mutRcrdr );
	return m_nReturn;
}

int CLineRecorder::StartRecording(int nThrottledCrntTrack)
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:strtRec\n");	
    if (nThrottledCrntTrack > 0) {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "re-recording track # %d\n",nThrottledCrntTrack); 
    }
    m_nThrottledCrntTrack = nThrottledCrntTrack;
    if (m_nThrottledCrntTrack > 0)
    {
        m_bReRecording = true;
    }
	return CallThreadFunction(m_tFlagVals.start);
}

int CLineRecorder::StopRecording()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:stopRec\n");	
    cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.running );
    return CallThreadFunction(m_tFlagVals.stop);
}

int CLineRecorder::CancelRecordingAux()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:cancelRecAux\n");	

    // flush all output to disk, and add a touch of silence for good measure.
    if (m_bCompressed)
        FinishCompressedOutput();
    if (m_szTrackURLPendingCoreAddition != NULL) {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "LR:already a track url pending core addition!\n"); 
    }
    
    char szTrackPath[PLAYLIST_STRING_SIZE];
    OutputURL((char*)szTrackPath);

    // kill out stream
    m_pOutStream->Close();
    delete m_pOutStream;
    m_pOutStream = NULL;
    if (!pc_unlink(szTrackPath))
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_WARNING, "lr:failed to delete cancelled track\n"); 
    return 0;

}

int CLineRecorder::CancelRecording()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:cancelRec\n");	
    cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.running );
    return CallThreadFunction(m_tFlagVals.cancel);
}

int CLineRecorder::PauseRecording()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:pauseRec\n");
    // use pause as a toggle
    if( cyg_flag_poll( &m_flgControl, m_tFlagVals.pause, CYG_FLAG_WAITMODE_OR ) & m_tFlagVals.pause ) {
        cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.pause );
        CallThreadFunction( m_tFlagVals.resume );
        return 1;  // resumed
    } else {
        // ? not sure
        cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.running );
        CallThreadFunction(m_tFlagVals.pause);
        return 0; // paused
    }
}

int CLineRecorder::ResumeRecording()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:resumeRec\n");
    if( cyg_flag_poll( &m_flgControl, m_tFlagVals.pause, CYG_FLAG_WAITMODE_OR ) & m_tFlagVals.pause ) {
        cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.pause );
        CallThreadFunction( m_tFlagVals.resume );
        return 1;  // resumed
    }
    return 0;
}

char* CLineRecorder::OutputSessionDir(char* szURL)
{
    sprintf(szURL,"%s/%s%03d", RECORDINGS_PATH, RECORDING_SESSION_DIR_NAME_STEM, m_nCrntSession);
    return szURL;
}

char* CLineRecorder::OutputRecordingDir(char* szURL, int iActiveTrack)
{
    sprintf(szURL,"%s/%s%03d/d%03d", RECORDINGS_PATH, RECORDING_SESSION_DIR_NAME_STEM, m_nCrntSession, iActiveTrack / RECORDER_MAX_DIR_ENTRIES);
    return szURL;
}

char* CLineRecorder::OutputURL(char* szURL)
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:outputURL\n"); 
    int nActiveTrack;
    if (m_nThrottledCrntTrack > 0)
    {
        nActiveTrack = m_nThrottledCrntTrack;
    }
    else
    {
        nActiveTrack = m_nCrntTrack;
    }
    char szExt[8];
    if (m_bCompressed)
        strcpy(szExt,"mp3");
    else
        strcpy(szExt,"raw");
    VerifyOrCreateDirectory(OutputRecordingDir(szURL, nActiveTrack));
    sprintf(szURL + strlen(szURL), "/%s%02d.%s", RECORDING_FILE_NAME_STEM, nActiveTrack,szExt);
    return szURL;
}

int CLineRecorder::InitDAIForRecording()
{
  	DAIOverflowCounter = 0;

    DAIInit();
    DAIResetRecord();
	DAISetSampleFrequency( m_nSampFreq );
	DAIEnable();
    SyncGain();
    SyncMicBoost();

    m_tDAIState.nLastOverflowCheck = 0;
    m_tDAIState.nLastOverflow = 0;
    // this value isn't really needed
    unsigned int nSamplesAvailable = 0;
    // we don't care about the return value either.
    unsigned int nSamplesRemaining = DAIRead(&m_tDAIState.pBuffer, &nSamplesAvailable);
	m_tDAIState.idxMinBuf = 1024;
	m_tDAIState.idxMaxBuf = 0;
    return 0;
}

int CLineRecorder::InitBuffers()
{
  	m_tEncodeBufs.bfOut = rb_new(OUTBUF_SIZE);
	m_tEncodeBufs.rdOut = rb_new_reader(m_tEncodeBufs.bfOut);
	m_tEncodeBufs.wrOut = rb_new_writer(m_tEncodeBufs.bfOut);
    return 0;
}

int CLineRecorder::FreeBuffers()
{
	rb_free_writer(m_tEncodeBufs.wrOut);
	rb_free_reader(m_tEncodeBufs.rdOut);
	rb_free(m_tEncodeBufs.bfOut);
    return 0;
}

#define BYTES_PER_OUTPUT_WRITE 16384

int CLineRecorder::OutputCompressedToFile(bool bFlushAll)
{
	unsigned int desired = rb_read_avail(m_tEncodeBufs.rdOut);
	ERESULT err;

	if (!bFlushAll)
		desired = (desired >= BYTES_PER_OUTPUT_WRITE ) ? BYTES_PER_OUTPUT_WRITE : 0;

	if (desired) {
		unsigned int actual;
		err = rb_read_data(m_tEncodeBufs.rdOut, desired, &actual);
		switch(err) {
		case RBUF_SPLIT:
        {
            m_pOutStream->Write(rb_read_ptr(m_tEncodeBufs.rdOut), actual);
            rb_read_done(m_tEncodeBufs.rdOut, actual);
			desired -= actual;
			rb_read_data(m_tEncodeBufs.rdOut, desired, &actual);
			// fall through
        }
		case RBUF_SUCCESS:
        {
            m_pOutStream->Write(rb_read_ptr(m_tEncodeBufs.rdOut), actual);
            rb_read_done(m_tEncodeBufs.rdOut, actual);
			break;
        }
		default:
			DBASSERT(DBG_LINE_RECORDER, false, "rbuf failure");
		}
	}
    return 0;
}

void CLineRecorder::OutputUncompressedToFile(short* pBuf,int nSamples)
{
    int nBytes = nSamples*sizeof(short);
    int nWritten = m_pOutStream->Write(pBuf,nBytes);
    if (nBytes != nWritten)
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_WARNING, "lr:tried to write %d, only wrote %d bytes to file!\n",nBytes,nWritten); 
}

// drain the last data out of the compressed data in the ring bufs
int CLineRecorder::FinishCompressedOutput()
{
    m_pPEM->EncodeBuffer(m_bfSilence);
    m_pPEM->Finish();
    OutputCompressedToFile(true);
    return 0;
}

void CLineRecorder::ThreadEntry(CYG_ADDRESS data)
{
	CLineRecorder* pThis = reinterpret_cast<CLineRecorder*>(data);
	pThis->EncodingThreadBody();
}

// For testing purposes.  Sets the maximum number of seconds a line-in recording can be.
//#define MAKE_SMALL_FILES    10

// perform one iteration of recording work
int CLineRecorder::Record()
{
    // periodically check if there has been an overflow or clip
    if (cyg_current_time() >= m_tDAIState.nLastOverflowCheck+100)
    {
        // overflow check
        m_tDAIState.nLastOverflowCheck = cyg_current_time();
		if (m_tDAIState.nLastOverflow < (volatile long)DAIOverflowCounter) {
			m_tDAIState.nLastOverflow = (volatile long)DAIOverflowCounter;
			DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "!");
		} else {
			DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, ".");
		}
        // clip check
        int nClip = ADCGetClip();
        if (nClip & CLIP_L || nClip & CLIP_R)
            m_pUserInterface->NotifyADCClipDetected();
    }
    m_tDAIState.idxBuf = DAIFreeRecordBufferCount();
	if (m_tDAIState.idxMaxBuf < m_tDAIState.idxBuf) 
        m_tDAIState.idxMaxBuf = m_tDAIState.idxBuf;
	if (m_tDAIState.idxMinBuf > m_tDAIState.idxBuf) 
        m_tDAIState.idxMinBuf = m_tDAIState.idxBuf;
	// Dump any pending compressed output
    if (m_bCompressed)
	    OutputCompressedToFile();
    unsigned int nSamplesAvailable = 0;
    // this doesn't actually copy anything, just set the buffer pointer to the proper ring buffer.
	unsigned int nSamplesRemaining = DAIRead(&m_tDAIState.pBuffer, &nSamplesAvailable);
    // when we get a full buffer, process it.
	if (nSamplesRemaining == 0) {
		int samples = 1152;
        m_nSamplesRecorded += samples;
        if (m_nSamplesRecorded > m_nSampFreq)
        {
            m_nSamplesRecorded -= m_nSampFreq;
            ++m_nSecondsRecorded;
            ++m_nSecondsRecordedDiv60;
            if (m_nSecondsRecordedDiv60 == 60)
            {
                m_nSecondsRecordedDiv60 = 0;
                ++m_nMinutesRecorded;

                // notify user minutes of space remaining
                NotifyUserRecordingTimeRemaining(m_nMaxMinutes-m_nMinutesRecorded);

                if (m_nMinutesRecorded >= m_nMaxMinutes)
                {   
                    DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "LR:StopRecSpaceLow!\n"); 
                    m_pEventQueue->PutEvent( EVENT_SPACE_LOW, NULL );
                }
#ifndef MAKE_SMALL_FILES
                if (m_nMinutesRecorded >= FILESIZE_SANITY_MINUTES)
                {
                    DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "LR:NextTrackOnLargeFile\n"); 
                    CEventQueue::GetInstance()->PutEvent(EVENT_RECORDING_TIMELIMIT, (void*) NULL);
                }
#endif  // MAKE_SMALL_FILES
            }
            m_pEventQueue->PutEvent( EVENT_STREAM_PROGRESS, (void*)m_nSecondsRecorded );
#ifdef MAKE_SMALL_FILES
            if (m_nSecondsRecorded >= MAKE_SMALL_FILES)
            {
                DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "LR:NextTrackOnLargeFile\n"); 
                CEventQueue::GetInstance()->PutEvent(EVENT_RECORDING_TIMELIMIT, (void*) NULL);
            }
#endif  // MAKE_SMALL_FILES
        }
        if (m_bCompressed)
        {
		    ERESULT err = m_pPEM->EncodeBuffer(m_tDAIState.pBuffer);
    		DBASSERT(DBG_LINE_RECORDER, SUCCEEDED(err), "PEM Failed: 0x%08x\n", err);
        }
        else
            // since we release the uncompressed buffer back to DAI shortly, write to file here if uncompressed
            OutputUncompressedToFile(m_tDAIState.pBuffer,nSamplesAvailable);
        // mark the pcm buffer as free for refilling
        DAIReleaseBuffer();
	} else {
        // we didn't get a full buffer yet, so just delay and loop loop and get more.
		cyg_thread_delay(5);
	}
    return 0;
}

int CLineRecorder::EncodingThreadBody()
{
	cyg_flag_value_t flagval;
	while (1)
	{
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:wt\n");	
    	cyg_flag_setbits( &m_flgControl, m_tFlagVals.done );
        flagval = cyg_flag_wait( &m_flgControl, m_tFlagVals.start | m_tFlagVals.stop | m_tFlagVals.cancel | m_tFlagVals.shutdown | m_tFlagVals.pause | m_tFlagVals.running | m_tFlagVals.resume | m_tFlagVals.addToCore | m_tFlagVals.passThru, CYG_FLAG_WAITMODE_OR );
        m_nReturn = 0;
    	cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.done );
        if ( flagval & m_tFlagVals.running )
        {
            Record();            
        }
		else if ( flagval & m_tFlagVals.start  )
		{
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg start\n");	
			cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.start|m_tFlagVals.pause) );
            cyg_flag_setbits( &m_flgControl, m_tFlagVals.running );
			m_nReturn = StartRecordingAux();
		}
		else if ( flagval & m_tFlagVals.stop )
		{
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg stop\n");	
			cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.stop|m_tFlagVals.pause) );
            m_nReturn = StopRecordingAux();
		}
        else if ( flagval & m_tFlagVals.cancel )
        {
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg cancel\n");	
			cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.cancel) );
            m_nReturn = CancelRecordingAux();
        }
        else if ( flagval & m_tFlagVals.pause )
        {
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg ps\n");	
            cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.running) );
        }
        else if ( flagval & m_tFlagVals.resume )
        {
            cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.pause|m_tFlagVals.resume) );
            cyg_flag_setbits( &m_flgControl, m_tFlagVals.running );
        }
        else if ( flagval & m_tFlagVals.addToCore )
        {
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg ps\n");	
            cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.addToCore);
            m_nReturn = AddRecordingToPlayerCoreAux();
        }
		else if ( flagval & m_tFlagVals.shutdown )
		{
            DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:flg shdn\n");	
			cyg_flag_maskbits( &m_flgControl, ~(m_tFlagVals.shutdown|m_tFlagVals.pause) );
			break;
		}
        else if ( flagval & m_tFlagVals.passThru )
        {
            // clip check
            int nClip = ADCGetClip();
            if (nClip & CLIP_L || nClip & CLIP_R)
                m_pUserInterface->NotifyADCClipDetected();
            cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.passThru );
        }
		else {
			DEBUGP( DBG_LINE_RECORDER, DBGLEV_ERROR, "LR:unhandled flag value!\n");
		}
	}
    return 0;
}

void CLineRecorder::InitRegistry()
{
    int* settings = (int*) new unsigned char[ StateSize() + 1 ];
    SaveState((void*)settings, StateSize());
    CRegistry::GetInstance()->AddItem( RecorderRegKey, (void*)settings, REGFLAG_PERSISTENT, StateSize() );
}

int CLineRecorder::SaveToRegistry() 
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:SaveToReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( RecorderRegKey );
    if( ! buf )
    {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_WARNING, "LR:Couldn't Find Registry Key\n");
        return 0;
    }
    else
        SaveState( buf, StateSize() );
    return 1;
}

int CLineRecorder::RestoreFromRegistry() 
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_TRACE, "lr:RestFrReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( RecorderRegKey );
    if( !buf )
        InitRegistry();
    else
        RestoreState( buf, StateSize() );
    return 1;
}

int CLineRecorder::SaveState(void* savebuf, int nLen)
{
    tRcrdrState* state = (tRcrdrState*) savebuf;
    state->nSessionID = m_nCrntSession;
    return 1;
}

int CLineRecorder::RestoreState(void* savebuf, int nLen)
{
    tRcrdrState* state = (tRcrdrState*) savebuf;
    m_nCrntSession = state->nSessionID;
    return 1;
}

int CLineRecorder::StateSize()
{
    return sizeof (tRcrdrState) + 1;
}

char* CLineRecorder::GetSessionName(char* szName)
{
    return PopulateSessionString(szName);   
}

int CLineRecorder::SetBitrate(int bitrate)
{
    if (bitrate == 0)
    {
        DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "lr:bitrate %d => PCM\n",bitrate); 
        m_bCompressed = false;
    }
    else
        m_bCompressed = true;
    return m_nBitrate = bitrate;
}

int CLineRecorder::GetBitrate()
{
    return m_nBitrate;
}

int CLineRecorder::MinutesFreeOnDrive()
{
    long long nfree = 0;
#if SPACE_MANAGER_ENABLED
	// consult space mgr for available bytes
    nfree = CSpaceMgr::GetInstance()->BytesFromLow();
    // convert to MB
    nfree /= (DJ_BYTES_PER_KILOBYTE * DJ_KILOBYTES_PER_MEGABYTE);
#else
	// get bytes free
	nfree = pc_free64(0);
	// convert to megs free
	nfree /= (DJ_BYTES_PER_KILOBYTE * DJ_KILOBYTES_PER_MEGABYTE);
	// set aside 256 MB (arbitrarily)
	nfree -= 256;
	// threshold to zero
	if (nfree < 0)
		nfree = 0;
#endif
    // account for uncompressed m_nBitrate==0
    int nEffectiveBitrate;
    if (m_nBitrate == 0)
        // approx cd audio rate
        nEffectiveBitrate = 1400;
    else
        nEffectiveBitrate = m_nBitrate;

	// convert to minutes
    float nMegsPerMinute = ((float)nEffectiveBitrate) * 60.0 / ((float)DJ_BYTES_PER_KILOBYTE) / 8.0;

    return (int) ((float)nfree / nMegsPerMinute);
}

void CLineRecorder::NotifyUserRecordingTimeRemaining(int nMins)
{
    TCHAR tszMessage[256];
    tstrcpy(tszMessage, LS(SID_RECORDING));

    char szTemp[64];
    sprintf(szTemp, " %d ", m_nBitrate ? m_nBitrate : 1400);
    TCHAR tszTemp[64];
    CharToTcharN(tszTemp, szTemp, 64-1);
    tstrncat(tszMessage, tszTemp, 256-1);

    tstrncat(tszMessage, LS(SID_KBPS), 256-1);

    sprintf(szTemp, ", ");
    CharToTcharN(tszTemp, szTemp, 64-1);
    tstrncat(tszMessage, tszTemp, 256-1);

    tstrncat(tszMessage, LS(SID_TIME_REMAINING), 256-1);
    tstrncat(tszMessage, LS(SID_COLON_SPACE), 256-1);

    sprintf(szTemp, "%dh %dm", (nMins / 60), (nMins % 60));
    CharToTcharN(tszTemp, szTemp, 64-1);
    tstrncat(tszMessage, tszTemp, 256-1);
    
    m_pUserInterface->SetMessage(tszMessage, CSystemMessageString::REALTIME_INFO);
}

void CLineRecorder::NotifyUserRecordingStarted()
{
    DEBUGP( DBG_LINE_RECORDER, DBGLEV_INFO, "lr:NtfyUsrRecStart\n");      
	m_nMaxMinutes = MinutesFreeOnDrive();
    NotifyUserRecordingTimeRemaining(m_nMaxMinutes);
}

void CLineRecorder::SetUserInterface(IUserInterface *pUI)
{
	m_pUserInterface = pUI;
}

// get adc gain value
int CLineRecorder::GetGain()
{
    m_nGain = ADCGetRightGain();
    return m_nGain;
}

void CLineRecorder::SetGain(int iGain)
{
    m_nGain = iGain;
    if (m_nGain > ADC_MAX_GAIN)
        m_nGain = ADC_MAX_GAIN;
    if (m_nGain < ADC_MIN_GAIN)
        m_nGain = ADC_MIN_GAIN;
    SyncGain();
}

// get adc gain range
int CLineRecorder::GetGainRange()
{
    return ADC_MAX_GAIN;
}
// increment adc gain
void CLineRecorder::IncrementGain()
{
    // dvb (3/8/02) getting just the right should be ok b/c the levels are synched
    m_nGain = ADCGetRightGain();
    ++m_nGain;
    if (m_nGain > ADC_MAX_GAIN)
        m_nGain = ADC_MAX_GAIN;
    SyncGain();
}
// decrement adc gain
void CLineRecorder::DecrementGain()
{
    m_nGain = ADCGetRightGain();
    --m_nGain;
    if (m_nGain < ADC_MIN_GAIN)
        m_nGain = ADC_MIN_GAIN;
    SyncGain();
}
// sync gain to agree with local setting
void CLineRecorder::SyncGain()
{
    ADCSetGain(m_nGain,m_nGain);
    DEBUGP( DBG_LINE_RECORDER , DBGLEV_INFO, "ps:Gn %d\n",m_nGain); 
}
// toggle adc mic boost
void CLineRecorder::ToggleMicBoost()
{
    m_bMicBoostEnabled = !m_bMicBoostEnabled;
    SyncMicBoost();
}
// sync mic boost to local setting
void CLineRecorder::SyncMicBoost()
{
    if (m_bMicBoostEnabled)
    {
        ADCEnableMicBoost();
        DEBUGP( DBG_LINE_RECORDER , DBGLEV_INFO, "ps:BoostOn\n"); 
    }
    else
    {
        ADCDisableMicBoost();
        DEBUGP( DBG_LINE_RECORDER , DBGLEV_INFO, "ps:BoostOff\n"); 
    }
}

bool CLineRecorder::ReRecording()
{
    return m_bReRecording;
}

void CLineRecorder::ClearReRecordingFlag()
{
    m_bReRecording = false;
}

void CLineRecorder::MonitorClip(bool bOn)
{
    if (bOn)
        cyg_flag_setbits( &m_flgControl, m_tFlagVals.passThru );
    else
        cyg_flag_maskbits( &m_flgControl, m_tFlagVals.passThru );
}
