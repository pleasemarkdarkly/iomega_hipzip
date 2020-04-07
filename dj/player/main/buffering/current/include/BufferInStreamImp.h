// BufferInStreamImp.h: interface for the CBufferingImp class.
//
//////////////////////////////////////////////////////////////////////
#if !defined(AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_)
#define AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_

#include <main/buffering/BufferInStream.h>

#include <main/buffering/BufferWorker.h>
#include <main/buffering/CircIndex.h>
#include <main/buffering/ReaderMsgPump.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/WriterMsgPump.h>
#include <datastream/fatfile/FatFile.h>

#include <datastream/input/InputStream.h>
#include <codec/common/Codec.h>

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


class CBufferingImp 
{
public:
    DEFINE_INPUTSTREAM( "Buffer Fat Input", FATFILE_BUFFEREDINPUT_IMP_ID );
	// singleton interface
    static CBufferingImp* GetInstance();
	static void Destroy();
    // playlist synchronization
    bool SetSong( IMediaContentRecord* mcr );
    // Seeks to the specified offset from the given point of origin.
	// Returns the offset if successful, or -1 if the seek failed.
	int Seek(bool bBuffered, eInputSeekPos Origin, int Offset);
	// Reads the specified number of bytes into the given buffer.
	int Read(bool bBuffered, void* pBlock, int dwBytes);
	// Returns the length of the stream, or -1 if the length is not known.
	int Length(bool bBuffered) const;
	// the client is finished with the current file.  somewhat empty notification for this class.
	ERESULT Close(bool bBuffered);
    // stub to satisfy IInputStream, real action happens in SetSong.
    ERESULT Open(const char*);
    // this stream is seekable
    bool CanSeek(bool bBuffered);
    // low level interaction with the drive, not supported
    int Ioctl(int, void*);
    // returns the current read position within the stream
    int Position(bool bBuffered);
    // return the preferred read chunk for the underlying drive.  not relevant in buffered stream.
    int GetInputUnitSize();
	// notify the stream that the user has either seeked or paused the track.  used to trigger conservative
    // disk access mode.
	void NotifyUserInterrupt() { ; }
	void NotifyPlaylistOrderChanged(char* szFilename);
    // notify the buffering library of various real-time considerations.
	void NotifyDecodedDataLow();	
	void NotifyDecodedDataHigh();	
	void NotifyEncodedDataLow();	
    // common combination of commands to the producer thread: unpause, set cautious mode, and begin filling blocks.
	void ResumeCautiousWriting();
    // notifies the stream that the eof is near
	void SetNearEndOfTrack();

    CWriterMsgPump* GetWriterMsgPump();
    CReaderMsgPump* GetReaderMsgPump();
    unsigned char* GetBlockSpace();
	bool IsNearEndOfTrack();
    bool OpenSimpleStream(const char* szUrl);
    void CloseSimpleStream();
    void NotifyStreamSet(IMediaContentRecord* mcr);
    void NotifyCDRemoved();

    // close and detach from sources no longer in the playlist (assumes crnt track inviolate)
    void ResyncWithPlaylist();
    // close and detach from all sources
    void DetachFromPlaylist();

    void PauseHDAccess();
    void ResumeHDAccess();

    void CloseFiles();
    void SetPrebufDefault(int nSeconds);
    int GetPrebufDefault();

    void SetPrebufCallback(SetIntFn* pfn);
public:

    void PauseThreads();
    void ResumeThreads();

	CBufferingImp();
	bool InitActiveDocument();
	~CBufferingImp();
	
    bool m_bNearEndOfTrack;
	
    static CBufferingImp* m_pInstance;
    IInputStream* m_pSimpleStream;
    bool m_bSimpleStreamOpen;
    int m_nSimpleStreamContentId;
    int m_nSimpleStreamOffset;
    int m_nCrntStreamId;
	CReaderMsgPump* m_pReaderMsgPump;
	CWriterMsgPump* m_pWriterMsgPump;
	CBufferWorker* m_pWorker;

    friend class CBufferWorker;
};

#include "BufferInStreamImp.inl"

#endif // AFX_PlaylistInputStream_H__C447959C_470D_4AC1_ACC4_7FD4DC4E7A3B__INCLUDED_
