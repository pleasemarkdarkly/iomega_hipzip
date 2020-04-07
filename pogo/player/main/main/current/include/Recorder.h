#include <cyg/kernel/kapi.h>
#include <cyg/kernel/thread.hxx>

// forward declares
class CFatFileOutputStream;
class CPEMWrapper;
class CEventQueue;
class IInputStream;
class IOutputStream;
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
    unsigned int nAvailData;
    unsigned int nRemainData;
    int idxBuf;
    int idxMinBuf;
    int idxMaxBuf;
};

struct tRcrdrFlagValues
{
    static const cyg_flag_value_t start = 1;			
	static const cyg_flag_value_t stop = 2;			
	static const cyg_flag_value_t done = 4;			
	static const cyg_flag_value_t pause = 8;			
	static const cyg_flag_value_t resume = 16;			
	static const cyg_flag_value_t shutdown = 32;       
    static const cyg_flag_value_t running = 64;
    static const cyg_flag_value_t addToCore = 128;
};

struct tRcrdrState
{
    int nSessionID;
};

class CRecorder
{
public:
    static CRecorder* GetInstance();
    static void Destroy();

    bool InSession();
    char* GetSessionName(char* szName);

    int OpenSession();
    int CloseSession();

    int StartRecording();
    int StopRecording();
    int AddRecordingToPlayerCore();

    int SetBitrate(int bitrate);
    int GetBitrate();

    int SetSampFreq(int sampfreq);
    int GetSampFreq();

    int SaveToRegistry();
    int RestoreFromRegistry();
private:
    CRecorder();
    ~CRecorder();
    int Record();
    // recorder thread side analogues to the interface functions
    int StartRecordingAux();
    int StopRecordingAux();
    int AddRecordingToPlayerCoreAux();
    int OpenSessionAux();
    int CloseSessionAux();
    // generate an output url based on session and track count
    char* OutputURL(char* szURL);
    // generate the appropriate directory path into szURL
    char* OutputDir(char* szURL);
    // dai interaction
    int InitDAIForRecording();
    tDAIRecordingState m_tDAIState;
    // ring buffer
    int InitBuffers();
    int FreeBuffers();
    int OutputToFile(bool bFlushAll = false);
    int FinishOutput();
    tRecordBuffers m_tRecordBufs;
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
    int m_nCrntSession;
    int m_nCrntTrack;
    int m_nBitrate;
    int m_nSampFreq;
    int m_nSecondsRecorded;
    int m_nSamplesRecorded;
    // singleton
    static CRecorder* m_pInstance;
};