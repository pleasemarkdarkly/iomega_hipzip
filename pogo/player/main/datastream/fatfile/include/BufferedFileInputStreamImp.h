// BufferedFileInputStreamImp.h: interface for the CBufferedFatFileInputStreamImp class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_)
#define AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_

#include <main/datastream/fatfile/CacheMan.h>
#include <main/datastream/fatfile/CircIndex.h>
#include <main/datastream/fatfile/ConsumerMsgPump.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>

#include <datastream/input/InputStream.h>

#define FATFILE_BUFFEREDINPUT_IMP_ID  0x1F

#define BUFFER_COUNT (16*2+4) // I just removed one intro from the FILES_BEHIND count, and I'll give 6 of the bufs to extended (the other 2 are for gen mem)
#define BUFFER_SIZE (63 * 1024)
#define CACHE_SIZE (BUFFER_COUNT*BUFFER_SIZE)
#define CONSECUTIVE_HARD_ERRORS_BLOCKS_ALLOWED 3			// allow this many hard errors before rejecting the track.
#define BUFFERS_TO_SPARE 1

#define BUFFERS_TO_FILL_AT_LOW_PRIORITY (BUFFER_COUNT / 2)
#define HIGH_BUFFERING_THREAD_PRIO 4
#define NORMAL_BUFFERING_THREAD_PRIO 10

class IPlaylistEntry;
class CFatFileInputStream;
class CCircIndex;
class CCacheMan;
class CFileCache;
class CMsgPump;
class CProducerMsgPump;
class CConsumerMsgPump;

// (epg,10/18/2001): TODO: impl stubs for drive power management
extern void WakeDrive();
extern void SleepDrive();
extern bool IsDriveAsleep();
extern void NoOp();
// (epg,10/18/2001): TODO: impl stubs for hard error support
#define EREAD 0xff12
#define DEBUG_WAKE_DRIVE (NoOp())
#define DEBUG_SLEEP_DRIVE (NoOp())

class CBufferedFatFileInputStreamImp : public IInputStream
{
public:
    DEFINE_INPUTSTREAM( "Buffered Fat Input", FATFILE_BUFFEREDINPUT_IMP_ID );
	static CBufferedFatFileInputStreamImp* GetInstance();
	static void Destroy();
	// --> Playlist Navigation
	int SetActiveFile(IPlaylistEntry* pEntry);
	// --> IInputStream Class Interface
	// Seeks to the specified offset from the given point of origin.
	// Returns the offset if successful, or -1 if the seek failed.
	int Seek(InputSeekPos Origin, int Offset);
	// Reads the specified number of bytes into the given buffer.
	int Read(void* pBuffer, int dwBytes);
	// Returns the length of the stream, or -1 if the length is not known.
	int Length() const;
	// the client is finished with the current file.  somewhat empty notification for this class.
	ERESULT Close();
    ERESULT Open(const char*){ return INPUTSTREAM_NO_ERROR; }
    bool CanSeek() const { return true; } 
    int Ioctl(int, void*);
    int Position() const;
    int GetInputUnitSize() { return 512; }
	// Notify the stream that the user has either seeked or paused the track.
	void NotifyUserInterrupt() { ; }
	// Notify the stream that the system is running extra services that will require less aggressive tactics.
	// (used primarily to inform buffering that digifile decrypting is in progress)
	void SetConservativeMode() { m_pCacheMan->SetConservativeMode(); }
	void SetAggressiveMode() { m_pCacheMan->SetAggressiveMode(); }

	void NotifyPlaylistOrderChanged(IPlaylistEntry* pEntry);
	void		NotifyDecodedDataLow();		// decoded data is running low.  stop proc intensive reads
	void		NotifyDecodedDataHigh();	// decoded data is running high.  allow proc intensive reads
	void		NotifyEncodedDataLow();		// decoder feed buffers are low.  spin up and fill them
	void		ResumeCautiousBuffering();
	void SetNearEndOfTrack() { m_bNearEndOfTrack = true; };

    CProducerMsgPump* GetProducerMsgPump() { return m_pProducerMsgPump; }
	CFileCache* GetCache();
	bool NearEndOfTrack() { return m_bNearEndOfTrack; };
	bool m_bNearEndOfTrack;


private:
	CBufferedFatFileInputStreamImp();
	bool InitActiveCache();
	~CBufferedFatFileInputStreamImp();
	static CBufferedFatFileInputStreamImp* m_pInstance;
	CConsumerMsgPump*	m_pConsumerMsgPump;
	CProducerMsgPump*	m_pProducerMsgPump;
	CCacheMan*			m_pCacheMan;
	CFileCache*			m_pCache;
	unsigned char		m_BufferSpace[ CACHE_SIZE + 1 ];

    friend class CCacheMan;
};

#endif // AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_
