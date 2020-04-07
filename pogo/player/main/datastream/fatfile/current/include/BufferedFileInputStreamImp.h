// BufferedFileInputStreamImp.h: interface for the CBufferedFatFileInputStreamImp class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_)
#define AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_

#include <main/datastream/fatfile/BufferedFileInputStream.h>

#include <main/datastream/fatfile/CacheMan.h>
#include <main/datastream/fatfile/CircIndex.h>
#include <main/datastream/fatfile/ConsumerMsgPump.h>
#include <main/datastream/fatfile/FileCache.h>
#include <main/datastream/fatfile/ProducerMsgPump.h>

#include <datastream/input/InputStream.h>

// forward decl
class IPlaylistEntry;
class CMsgPump;

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
	// singleton interface
    static CBufferedFatFileInputStreamImp* GetInstance();
	static void Destroy();
    // playlist synchronization
    bool SetSong( IMediaContentRecord* mcr );
    // Seeks to the specified offset from the given point of origin.
	// Returns the offset if successful, or -1 if the seek failed.
	int Seek(InputSeekPos Origin, int Offset);
	// Reads the specified number of bytes into the given buffer.
	int Read(void* pBuffer, int dwBytes);
	// Returns the length of the stream, or -1 if the length is not known.
	int Length() const;
	// the client is finished with the current file.  somewhat empty notification for this class.
	ERESULT Close();
    // stub to satisfy IInputStream, real action happens in SetSong.
    ERESULT Open(const char*);
    // this stream is seekable
    bool CanSeek() const;
    // low level interaction with the drive, not supported
    int Ioctl(int, void*);
    // returns the current read position within the stream
    int Position() const;
    // return the preferred read chunk for the underlying drive.  not relevant in buffered stream.
    int GetInputUnitSize();
	// notify the stream that the user has either seeked or paused the track.  used to trigger conservative
    // disk access mode.
	void NotifyUserInterrupt() { ; }
	// Notify the stream that the system is running extra services that will require less aggressive tactics.
	// (used primarily to inform buffering that digifile decrypting is in progress)
	void SetConservativeMode();
	void SetAggressiveMode();
    // notify the playlist synching mechanism that the ordering has changed.
	void NotifyPlaylistOrderChanged(const char* szFilename);
    // notify the buffering library of various real-time considerations.
	void NotifyDecodedDataLow();	
	void NotifyDecodedDataHigh();	
	void NotifyEncodedDataLow();	
    // common combination of commands to the producer thread: unpause, set cautious mode, and begin filling buffers.
	void ResumeCautiousBuffering();
    // notifies the stream that the eof is near
	void SetNearEndOfTrack();

    CProducerMsgPump* GetProducerMsgPump();
    CConsumerMsgPump* GetConsumerMsgPump();
    unsigned char* GetBufferSpace();
	bool IsNearEndOfTrack();

private:
	CBufferedFatFileInputStreamImp();
	bool InitActiveCache();
	~CBufferedFatFileInputStreamImp();
	
    bool m_bNearEndOfTrack;
	static CBufferedFatFileInputStreamImp* m_pInstance;
	CConsumerMsgPump* m_pConsumerMsgPump;
	CProducerMsgPump* m_pProducerMsgPump;
	CCacheMan* m_pCacheMan;

    friend class CCacheMan;
};

#include "BufferedFileInputStreamImp.inl"

#endif // AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_
