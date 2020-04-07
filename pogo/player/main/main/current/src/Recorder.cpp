// general
#include <codec/mp3/pem/codec.h>
#include <codec/mp3/pem/pem.h>
#include <core/playmanager/PlayManager.h>
#include <core/events/SystemEvents.h>
#include <datasource/datasourcemanager/DataSourceManager.h>
#include <datastream/fatfile/FileOutputStream.h>
#include <main/main/AppSettings.h>
#include <main/main/FatHelper.h>
#include <main/main/PlaylistConstraint.h>
#include <main/main/Events.h>
#include <main/main/EventTypes.h>
#include <main/util/filenamestore/FileNameStore.h>
#include <main/ui/PlayerScreen.h>
#include <main/content/metakitcontentmanager/MetakitContentManager.h>
#include <main/datasource/fatdatasource/FatDataSource.h>
#include <devs/audio/dai.h>
#include <devs/audio/cs5332.h>
#include <stdio.h>      /* sprintf */
#include <util/eventq/EventQueueAPI.h>     // for main event queue
// self
#include <main/main/Recorder.h>
// registry
#include <util/registry/Registry.h>
static const RegKey RecorderRegKey = REGKEY_CREATE( RECORDER_REGISTRY_KEY_TYPE, RECORDER_REGISTRY_KEY_NAME );
// debugging
#include <util/debug/debug.h>
DEBUG_MODULE_S( DBG_RECORDER, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE(DBG_RECORDER);

#define RECORDER_STACK_SIZE (32768)
#define RECORDER_THREAD_PRIORITY (10)

CRecorder* CRecorder::m_pInstance = 0;

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

CRecorder* CRecorder::GetInstance()
{
    if (!m_pInstance)
        m_pInstance = new CRecorder;
    return m_pInstance;
}

void CRecorder::Destroy()
{
    delete m_pInstance;
    m_pInstance = 0;
}

CRecorder::CRecorder() : m_nReturn(0), m_bInSession(false), m_nCrntSession(0), m_nCrntTrack(0), m_nBitrate(128), m_nSampFreq(44100), m_nSecondsRecorded(0), m_nSamplesRecorded(0)

{
    m_pEventQueue  = CEventQueue::GetInstance();
    m_pPEM = CPEMWrapper::GetInstance();
    m_pThreadStack = new char[RECORDER_STACK_SIZE+1];
   	cyg_flag_init( &m_flgControl );
	m_pthdWork = new Cyg_Thread( RECORDER_THREAD_PRIORITY, // thread priority
								&CRecorder::ThreadEntry,
								reinterpret_cast<CYG_ADDRWORD>(this),
								"Recorder",
								(CYG_ADDRESS)m_pThreadStack,
								(cyg_ucount32)RECORDER_STACK_SIZE);
   	cyg_mutex_init( &m_mutRcrdr );
    m_pthdWork->resume();  
}

CRecorder::~CRecorder()
{
    delete m_pPEM;
    m_pPEM = NULL;
  	cyg_flag_destroy( &m_flgControl );
	delete m_pthdWork;
   	cyg_mutex_destroy( &m_mutRcrdr ); 
}

bool CRecorder::InSession()
{
    return m_bInSession; 
}

// check if there are any preexisting mp3 files in the candidate session directory.
bool CRecorder::SessionDirNonEmpty(char* url)
{
    DSTAT stat;
    int len = strlen(url);
    strcat(url,"/*.mp3");
    bool empty = true;
    if (pc_gfirst(&stat, url))
    {
        pc_gdone(&stat);
        empty = false;
    }
    // lop off the wildcard
    url[len] = 0;
    return !empty;
}

int CRecorder::OpenSession()
{
    if (m_bInSession)
        return 0;
    RestoreFromRegistry();
    char url[PLAYLIST_STRING_SIZE];
    VerifyOrCreateDirectory(RECORDINGS_PATH);
    strcpy(url,URL_STEM);
    strcat(url,RECORDINGS_PATH);
    CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->AddNodeByURL(url);
    bool bSessionFound = false;
    int nStartSession = m_nCrntSession;
    while (!bSessionFound)
    {
        ++m_nCrntSession;
        if (m_nCrntSession == nStartSession)
            break;
        // make sure we have the suitable parent directories where we will put the file
        VerifyOrCreateDirectory(OutputDir(url));
        if (SessionDirNonEmpty(url))
            continue;
        bSessionFound = true;
        // clear out any potential constraints previously existing (?)
        CPlaylistConstraint::GetInstance()->Constrain();
        // clear onscreen fields
        ((CPlayerScreen*)CPlayerScreen::GetPlayerScreen())->ClearTrack();
        m_bInSession = true;
        SaveToRegistry();
    }
    if (bSessionFound)
    {
        strcpy(url,URL_STEM);
        OutputDir(LongPathFromFullURL(url));
        CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore()->AddNodeByURL(url);
        return m_nCrntSession;
    }
    return m_nCrntSession = -1;
}

int CRecorder::CloseSession()
{
    if (!m_bInSession)
        return 0;
    m_bInSession = false;
    m_nCrntTrack = 0;
    SaveToRegistry();
    return m_nCrntSession;
}

IOutputStream* CRecorder::PrepOutStream()
{
    IOutputStream* out = new CFatFileOutputStream;
    char url[PLAYLIST_STRING_SIZE];
    if (SUCCEEDED(out->Open(OutputURL(url))))
        return out;
    delete out;
    return NULL;
}

int CRecorder::InitInputSystem()
{
    InitDAIForRecording();
}

int CRecorder::InitOutputSystem()
{
    InitBuffers();
}

void CRecorder::InitPlayerScreenTextFields()
{
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    TCHAR tszTemp[32];
    char szTemp[32];
    // genre
    CharToTchar(tszTemp,gc_szID3v1GenreTable[RECORDING_GENRE]);
    ps->SetSetText(tszTemp);
    // artist
    CharToTchar(tszTemp,RECORDING_ARTIST);
    ps->SetArtistText(tszTemp);
    // album
    CharToTchar(tszTemp,PopulateSessionString(szTemp));
    ps->SetAlbumText(tszTemp);
    // track
    CharToTchar(tszTemp,PopulateTrackString(szTemp));
    ps->SetTrackText(tszTemp);
    // bitrate
    sprintf(szTemp,"%03d",m_nBitrate);
    CharToTchar(tszTemp,szTemp);
    ps->SetBitrateText(tszTemp);
    // record icon
    ps->SetControlSymbol(CPlayerScreen::RECORD);
    // initial time
    ps->SetTime(0);
    // constraint dots
    ps->UpdateConstraintDots(false, false, true, false);
    // draw
    ps->Invalidate(ps->mReal);
    ps->Draw();
}

void InitSRAM()
{
    memset( (void*)SRAM_START, 0, SRAM_SIZE);
}

int CRecorder::StartRecordingAux()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:strtRcrdAux\n");
    // start a timer to flutter the control symbol on the playerscren.
    CPlayerScreen* ps = (CPlayerScreen*)CPlayerScreen::GetPlayerScreen();
    ps->StartFlutterTimer(CPlayerScreen::RECORD, CPlayerScreen::PAUSE);
    // zero out track level counters
    m_nSecondsRecorded = 0;
    m_nSamplesRecorded = 0;
    // increment track counter
    ++m_nCrntTrack;
    // prep output
    InitOutputSystem();
    m_pOutStream = PrepOutStream();
    // populate the player screen with info about the currently recording track.
    InitPlayerScreenTextFields();
    // init pem
    m_pPEM->Start(m_nBitrate, m_tRecordBufs.rdIn, m_tRecordBufs.wrOut);
    // prep input
    InitInputSystem();
    ps->KillFlutterTimer();
    ps->SetControlSymbol(CPlayerScreen::RECORD);
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
}

char* CRecorder::PopulateSessionString( char* szSession )
{
    sprintf (szSession, "%s%03d", RECORDING_SESSION_DIR_NAME_STEM, m_nCrntSession);
    return szSession;
}

char* CRecorder::PopulateTrackString( char* szTrack )
{
    sprintf (szTrack, "%s%02d", RECORDING_FILE_NAME_STEM, m_nCrntTrack);
    return szTrack;
}

int CRecorder::AppendID3v1Tag(IOutputStream* pStream)
{
    int nSize = pStream->Seek(IOutputStream::SeekEnd, 0);
    if (!nSize)
        return 0;
    id3tag tag;
    memset(&tag, 0, sizeof(id3tag));
    // genre
    tag.genre = RECORDING_GENRE;
    // artist
    strcpy( (char*)tag.artist, RECORDING_ARTIST);
    // album
    char temp[32];
    PopulateSessionString( temp );
    strcpy( (char*)tag.album, temp);
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

int CRecorder::StopRecordingAux()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:stpRcrdAux\n");
    // flush all output to disk, and add a touch of silence for good measure.
    FinishOutput();

    // report on overflow stats
    DEBUGP( DBG_RECORDER, DBGLEV_INFO, "%d dropped ", DAIOverflowCounter);
	DEBUGP( DBG_RECORDER, DBGLEV_INFO, "%d/", m_tDAIState.idxMinBuf);
	DEBUGP( DBG_RECORDER, DBGLEV_INFO, "%d (min/max) free buffers\n", m_tDAIState.idxMaxBuf);

    // add an id3v1 tag to the end of the file
    int nFileSize = AppendID3v1Tag(m_pOutStream);

    // kill out stream
    m_pOutStream->Close();
    delete m_pOutStream;
    m_pOutStream = NULL;

    return nFileSize;
}

int CRecorder::AddRecordingToPlayerCoreAux()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:addToCoreAux\n");
    CPlayManager::GetInstance()->RefreshAllContent( IDataSource::DSR_ONE_PASS_WITH_METADATA, CONTENT_UPDATE_CHUNK_SIZE );


    /*
    // insert the new file into the FileNameStore
    char url[PLAYLIST_STRING_SIZE];
    CFileNameStore* store = CPlayManager::GetInstance()->GetContentManager()->GetFileNameStore();
    // get a reference to the parent directory
    strcpy(url, URL_STEM);
    OutputDir(LongPathFromFullURL(url));
    IFileNameRef* pDirRef = store->GetRefByURL(url);
    // prepare a file node for the new file
    char *szName, *szLongName;
    LookupFilenames(OutputURL(url),&szName,&szLongName);
    IFileNameNode* pNode = new CFileFileNameNode(szName,szLongName);
    // the node retains its own copy of the names, so deallocate.
    free(szName);
    free(szLongName);
    // get a reference to the new node
    IFileNameRef* pFileRef = new CStoreFileNameRef(pNode);
    // add the new node into the filename store
    (**((CStoreFileNameRef*)pDirRef))->AddChild(pNode);

    CFatDataSource* pDS = (CFatDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByURL("file://a:");
    // throw the new file into a list for processing into the content manager
    FileRefList* lstNewFiles = new FileRefList;
    lstNewFiles->PushBack(pFileRef);
    // create a media content record for the new file
    pDS->CreateMediaRecordsForNewFiles(lstNewFiles);
    */
}

// should be called immediately after stopping recording, unless we're stopping for usb (where it would be too slow and redundant anyway)
int CRecorder::AddRecordingToPlayerCore()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:addRecToCore\n");
    return CallThreadFunction(m_tFlagVals.addToCore);
}

int CRecorder::CallThreadFunction(int iFlagVal)
{
    cyg_mutex_lock( &m_mutRcrdr );
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:callThrdFn %d\n",iFlagVal);
	cyg_flag_wait( &m_flgControl, m_tFlagVals.done, CYG_FLAG_WAITMODE_OR );
	cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.done );
	cyg_flag_setbits( &m_flgControl, iFlagVal );
	cyg_flag_wait( &m_flgControl, m_tFlagVals.done, CYG_FLAG_WAITMODE_OR );
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:callThrdFn done\n",iFlagVal);
	cyg_mutex_unlock( &m_mutRcrdr );
	return m_nReturn;
}

int CRecorder::StartRecording()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:strtRcrdng\n");	
	return CallThreadFunction(m_tFlagVals.start);
}

int CRecorder::StopRecording()
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:stopRcrdng\n");	
    cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.running );
    return CallThreadFunction(m_tFlagVals.stop);
}

char* CRecorder::OutputDir(char* szURL)
{
    sprintf(szURL,"%s/%s%03d", RECORDINGS_PATH, RECORDING_SESSION_DIR_NAME_STEM, m_nCrntSession);
    return szURL;
}

char* CRecorder::OutputURL(char* szURL)
{
    sprintf(szURL + strlen(OutputDir(szURL)), "/%s%02d.mp3", RECORDING_FILE_NAME_STEM, m_nCrntTrack);
    return szURL;
}

int CRecorder::InitDAIForRecording()
{
  	DAIOverflowCounter = 0;

    DAIInit();
    DAIResetRecord();
	DAISetSampleFrequency( m_nSampFreq );
	DAIEnable();

    m_tDAIState.nLastOverflowCheck = 0;
    m_tDAIState.nLastOverflow = 0;
    m_tDAIState.nRemainData = DAIRead(&m_tDAIState.pBuffer, &m_tDAIState.nAvailData);
	m_tDAIState.idxMinBuf = 1024;
	m_tDAIState.idxMaxBuf = 0;
}

int CRecorder::InitBuffers()
{
  	m_tRecordBufs.bfOut = rb_new(OUTBUF_SIZE);
	m_tRecordBufs.rdOut = rb_new_reader(m_tRecordBufs.bfOut);
	m_tRecordBufs.wrOut = rb_new_writer(m_tRecordBufs.bfOut);
}

int CRecorder::FreeBuffers()
{
	rb_free_writer(m_tRecordBufs.wrOut);
	rb_free_reader(m_tRecordBufs.rdOut);
	rb_free(m_tRecordBufs.bfOut);
}

#define BYTES_PER_OUTPUT_WRITE 16384

int CRecorder::OutputToFile(bool bFlushAll)
{
	unsigned int desired = rb_read_avail(m_tRecordBufs.rdOut);
	ERESULT err;

	if (!bFlushAll)
		desired = (desired >= BYTES_PER_OUTPUT_WRITE ) ? BYTES_PER_OUTPUT_WRITE : 0;

	if (desired) {
		unsigned int actual;
		err = rb_read_data(m_tRecordBufs.rdOut, desired, &actual);
		switch(err) {
		case RBUF_SPLIT:
        {
            m_pOutStream->Write(rb_read_ptr(m_tRecordBufs.rdOut), actual);
            rb_read_done(m_tRecordBufs.rdOut, actual);
			desired -= actual;
			rb_read_data(m_tRecordBufs.rdOut, desired, &actual);
			// fall through
        }
		case RBUF_SUCCESS:
        {
            m_pOutStream->Write(rb_read_ptr(m_tRecordBufs.rdOut), actual);
            rb_read_done(m_tRecordBufs.rdOut, actual);
			break;
        }
		default:
			DBASSERT(DBG_RECORDER, false, "rbuf failure");
		}
	}
}

int CRecorder::FinishOutput()
{
	m_pPEM->EncodeBuffer(m_bfSilence, 0, 1);
	OutputToFile(true);
}

void CRecorder::ThreadEntry(CYG_ADDRESS data)
{
	CRecorder* pThis = reinterpret_cast<CRecorder*>(data);
	pThis->EncodingThreadBody();
}

// perform one iteration of recording work
int CRecorder::Record()
{
    // periodically check if there has been an overflow or clip
    if (cyg_current_time() >= m_tDAIState.nLastOverflowCheck+100)
    {
        // overflow check
        m_tDAIState.nLastOverflowCheck = cyg_current_time();
		if (m_tDAIState.nLastOverflow < (volatile long)DAIOverflowCounter) {
			m_tDAIState.nLastOverflow = (volatile long)DAIOverflowCounter;
			DEBUGP( DBG_RECORDER, DBGLEV_INFO, "!");
		} else {
			DEBUGP( DBG_RECORDER, DBGLEV_INFO, ".");
		}
        // clip check
        int nClip = ADCGetClip();
        if (nClip & CLIP_L || nClip & CLIP_R)
            m_pEventQueue->PutEvent( EVENT_CLIP_DETECTED, NULL );
    }
    m_tDAIState.idxBuf = DAIFreeRecordBufferCount();
	if (m_tDAIState.idxMaxBuf < m_tDAIState.idxBuf) 
        m_tDAIState.idxMaxBuf = m_tDAIState.idxBuf;
	if (m_tDAIState.idxMinBuf > m_tDAIState.idxBuf) 
        m_tDAIState.idxMinBuf = m_tDAIState.idxBuf;
	// Dump any pending output
	OutputToFile();
	m_tDAIState.nRemainData = DAIRead(&m_tDAIState.pBuffer, &m_tDAIState.nAvailData);
	if (m_tDAIState.nRemainData == 0) {
		int samples = 1152;
        m_nSamplesRecorded += samples;
        if (m_nSamplesRecorded > m_nSampFreq)
        {
            m_nSamplesRecorded -= m_nSampFreq;
            ++m_nSecondsRecorded;
            m_pEventQueue->PutEvent( EVENT_STREAM_PROGRESS, (void*)m_nSecondsRecorded );
        }
		ERESULT err = m_pPEM->EncodeBuffer(m_tDAIState.pBuffer, &samples, 0);
		DBASSERT(DBG_RECORDER, SUCCEEDED(err), "PEM Failed: 0x%08x\n", err);
        DAIReleaseBuffer();
	} else {
		cyg_thread_delay(5);
	}
 
}

int CRecorder::EncodingThreadBody()
{
	cyg_flag_value_t flagval;
	while (1)
	{
        DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:wt\n");	
    	cyg_flag_setbits( &m_flgControl, m_tFlagVals.done );
        flagval = cyg_flag_wait( &m_flgControl, m_tFlagVals.start | m_tFlagVals.stop | m_tFlagVals.shutdown | m_tFlagVals.pause | m_tFlagVals.running | m_tFlagVals.addToCore , CYG_FLAG_WAITMODE_OR );
        m_nReturn = 0;
    	cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.done );
        if ( flagval & m_tFlagVals.running )
        {
            Record();            
        }
		else if ( flagval & m_tFlagVals.start  )
		{
            DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:flg start\n");	
			cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.start );
            cyg_flag_setbits( &m_flgControl, m_tFlagVals.running );
			m_nReturn = StartRecordingAux();
		}
		else if ( flagval & m_tFlagVals.stop )
		{
            DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:flg stop\n");	
			cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.stop );
            m_nReturn = StopRecordingAux();
		}
        else if ( flagval & m_tFlagVals.pause )
        {
            DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:flg ps\n");	
            cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.pause );
            cyg_flag_wait( &m_flgControl, m_tFlagVals.resume, CYG_FLAG_WAITMODE_OR );
            cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.resume );
        }
        else if ( flagval & m_tFlagVals.addToCore )
        {
            DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:flg ps\n");	
            cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.addToCore);
            m_nReturn = AddRecordingToPlayerCoreAux();
        }
		else if ( flagval & m_tFlagVals.shutdown )
		{
            DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:flg shdn\n");	
			cyg_flag_maskbits( &m_flgControl, ~m_tFlagVals.shutdown );
			break;
		}
		else {
			DEBUGP( DBG_RECORDER, DBGLEV_ERROR, "RC:unhandled flag value!\n");
		}
	}
}

void CRecorder::InitRegistry()
{
    int* settings = (int*) new unsigned char[ StateSize() + 1 ];
    SaveState((void*)settings, StateSize());
    CRegistry::GetInstance()->AddItem( RecorderRegKey, (void*)settings, REGFLAG_PERSISTENT, StateSize() );
}

int CRecorder::SaveToRegistry() 
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:SaveToReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( RecorderRegKey );
    if( ! buf )
    {
        DEBUGP( DBG_RECORDER, DBGLEV_WARNING, "RC:Couldn't Find Registry Key\n");
        return 0;
    }
    else
        SaveState( buf, StateSize() );
    return 1;
}

int CRecorder::RestoreFromRegistry() 
{
    DEBUGP( DBG_RECORDER, DBGLEV_TRACE, "rc:RestFrReg\n");
    void* buf = CRegistry::GetInstance()->FindByKey( RecorderRegKey );
    if( !buf )
        InitRegistry();
    else
        RestoreState( buf, StateSize() );
    return 1;
}

int CRecorder::SaveState(void* savebuf, int len)
{
    tRcrdrState* state = (tRcrdrState*) savebuf;
    state->nSessionID = m_nCrntSession;
    return 1;
}

int CRecorder::RestoreState(void* savebuf, int len)
{
    tRcrdrState* state = (tRcrdrState*) savebuf;
    m_nCrntSession = state->nSessionID;
    return 1;
}

int CRecorder::StateSize()
{
    return sizeof (tRcrdrState) + 1;
}

char* CRecorder::GetSessionName(char* szName)
{
    return PopulateSessionString(szName);   
}

int CRecorder::SetBitrate(int bitrate)
{
    return m_nBitrate = bitrate;
}

int CRecorder::GetBitrate()
{
    return m_nBitrate;
}
