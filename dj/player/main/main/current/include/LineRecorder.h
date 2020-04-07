#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>

#define FILESIZE_SANITY_MINUTES (60*3)

// forward declares
class CFatFileOutputStream;
class CPEMWrapper;
class CEventQueue;
class IInputStream;
class IOutputStream;
class IUserInterface;
struct rbuf_s;
struct rbuf_reader_s;
struct rbuf_writer_s;
typedef struct rbuf_s rbuf_t;
typedef struct rbuf_reader_s rbuf_reader_t;
typedef struct rbuf_writer_s rbuf_writer_t;

#define OUTBUF_SIZE (32768)
struct tRecordBuffers
{
  	rbuf_t *bfIn;
	rbuf_reader_t *rdIn;
	rbuf_writer_t *wrIn;
	rbuf_t *bfOut;
	rbuf_reader_t *rdOut;
	rbuf_writer_t *wrOut;
};

struct tDAIRecordingState
{
    unsigned int nLastOverflow;
    cyg_tick_count_t nLastOverflowCheck;
    short* pBuffer;
    int idxBuf;
    int idxMinBuf;
    int idxMaxBuf;
};

struct tRcrdrFlagValues
{
    static const cyg_flag_value_t start     = 1;			
	static const cyg_flag_value_t stop      = 2;			
	static const cyg_flag_value_t done      = 4;			
	static const cyg_flag_value_t pause     = 8;			
	static const cyg_flag_value_t resume    = 16;			
	static const cyg_flag_value_t shutdown  = 32;       
    static const cyg_flag_value_t running   = 64;
    static const cyg_flag_value_t addToCore = 128;
    static const cyg_flag_value_t cancel    = 256;
    static const cyg_flag_value_t passThru  = 512;
};

struct tRcrdrState
{
    int nSessionID;
};

class CLineRecorder
{
public:
    static CLineRecorder* GetInstance();
    static void Destroy();

    bool InSession();
    char* GetSessionName(char* szName);

    int OpenSession();
    int CloseSession();

    int StartRecording(int nThrottledCrntTrack = 0);
    int PauseRecording();
    int ResumeRecording();
    int StopRecording();
    int CancelRecording();
    int AddRecordingToPlayerCore();

    int SetBitrate(int bitrate);
    int GetBitrate();

    int SetSampFreq(int sampfreq);
    int GetSampFreq();

    int SaveToRegistry();
    int RestoreFromRegistry();

	void SetUserInterface(IUserInterface *pUI);

    // get adc gain value
    int GetGain();
    // set adc gain value
    void SetGain(int iGain);
    // get adc gain range
    int GetGainRange();
    // increment adc gain
    void IncrementGain();
    // decrement adc gain
    void DecrementGain();
    // toggle adc mic boost
    void ToggleMicBoost();
    // are we re-recording an old track?
    bool ReRecording();
    // clear out previous re-recording flag, and see if a new one is warranted.
    void ClearReRecordingFlag();
    // sync gain to agree with local setting
    void SyncGain();
    // sync mic boost to local setting
    void SyncMicBoost();

	// dai interaction
    int InitDAIForRecording();

    // monitor the clip status
    void MonitorClip(bool bOn);
    
private:
    CLineRecorder();
    ~CLineRecorder();
    int Record();
    // format and output a message informing the user how much recording time remains.
    void NotifyUserRecordingTimeRemaining(int nMins);
	// display a sysmsg string on the playerscreen to inform the user that 
	// recording has begun, and how much time is free in the current recording mode.
	void NotifyUserRecordingStarted();
	// using the current settings, consult the space manager and calculate the available
	// space in minutes on the harddrive.
	int MinutesFreeOnDrive();
    // recorder thread side analogues to the interface functions
    int StartRecordingAux();
    int StopRecordingAux();
    int CancelRecordingAux();
    int AddRecordingToPlayerCoreAux();
    int OpenSessionAux();
    int CloseSessionAux();
    // generate an output url based on session and track count
    char* OutputURL(char* szURL);
    // generate the appropriate directory path into szURL
    char* OutputSessionDir(char* szURL);
    char* OutputRecordingDir(char* szURL, int iActiveTrack);
    // attach an mft to the current file
    int AddMetadataFileTag(IOutputStream* pStream);
    // return the currently active track number.
    int GetActiveTrackNumber();

    tDAIRecordingState m_tDAIState;
    // ring buffer
    int InitBuffers();
    int FreeBuffers();
    int OutputCompressedToFile(bool bFlushAll = false);
    void OutputUncompressedToFile(short* pBuf,int nSamples);
    int FinishCompressedOutput();
    tRecordBuffers m_tEncodeBufs;
    // encoder
  	CPEMWrapper *m_pPEM;
    short m_bfSilence[1152*2];
    // input handling
    int InitInputSystem();
    // output handling
    int InitOutputSystem();
    IOutputStream* PrepOutStream();
    IOutputStream* m_pOutStream;
    int AppendID3v1Tag(IOutputStream* pStream);
    char* PopulateSessionString( char* szSession );
    char* PopulateTrackString( char* szTrack );
    bool SessionDirNonEmpty(char* url);
    // player screen integration
    void InitPlayerScreenTextFields();
    CEventQueue* m_pEventQueue;
    // thread control
    int CallThreadFunction(int iFlagVal);
  	static void ThreadEntry(CYG_ADDRESS data);
    int EncodingThreadBody();

    char* m_pThreadStack;
	cyg_flag_t m_flgControl;
	Cyg_Thread*	m_pthdWork;
   	cyg_mutex_t	m_mutRcrdr;
    int m_nReturn;
    // flag values
    tRcrdrFlagValues m_tFlagVals;
    // state persistence
    void InitRegistry();
    int SaveState(void* savebuf, int len);
    int RestoreState(void* savebuf, int len);
    int StateSize();
	// modal state
    bool m_bInSession;
    bool m_bReRecording;
    int m_nCrntSession;
    int m_nCrntTrack;
    // an overriding track number
    int m_nThrottledCrntTrack;
    // if true, then the bitrate indicates the level of compression.  if not, then the target is uncompressed pcm.
    bool m_bCompressed;
    int m_nBitrate;
    int m_nGain;
    int m_bMicBoostEnabled;
    int m_nSampFreq;
    int m_nSecondsRecorded;
    int m_nSecondsRecordedDiv60;
    int m_nMinutesRecorded;
    int m_nSamplesRecorded;
    // url for track recorded but not yet added to core
    char* m_szTrackURLPendingCoreAddition;
    // singleton
    static CLineRecorder* m_pInstance;
	// ui
	IUserInterface* m_pUserInterface;
    // free space tracking
    int m_nMaxMinutes;
};
