#ifndef __PIS_INCLUDE_CONSUMER_DEFINED__ 
#define __PIS_INCLUDE_CONSUMER_DEFINED__

#include <datastream/input/InputStream.h>
#include <main/buffering/BufferDocument.h>
#include <main/buffering/BufferTypes.h>

class CBufferReader
{
public:
	CBufferReader();
	~CBufferReader();
	
	int Seek(eInputSeekPos Origin, int Offset);
	int Read(void* pBlock,int dwBytes);
	static CBufferDocument* GetReadDocument() { return m_pCurrentReadDocument; }
	void SetCurrentReadDocument(CBufferDocument* pNewDocument);
    void ClearReadBlockLock();
    void SetWorker(CBufferWorker* pWkr);

    void SetPrebufDefault(int nSeconds);
    int GetPrebufDefault();

    // register a function that will be notified periodically of a percent-complete update during prebuffering periods
    void SetPrebufCallback(SetIntFn* pfn) { m_pfnNotifyPrebuf = pfn; }

    void SetQuickLockReady();

private:
    bool LockBlockForReading();
    int GetSafeFileLength(CBufferDocument* pDoc);
    bool FileOffsetFromOriginOffset(CBufferDocument* pDoc, int nFileSize, eInputSeekPos eOrigin, int nOffset, int &nFileOffset);
    void ReadBlockLimits(CBufferDocument *pDoc, int &nBlockFloor, int &nBlockCeiling);


	bool ShiftReadPoint(int nSoughtFileOffset, int nCharsToShift, short iUnitDirection, bool bBorrowingInvalid);
	void SetReadDocument(CBufferDocument* pDoc) { m_pCurrentReadDocument = pDoc; }
    SetIntFn* m_pfnNotifyPrebuf;
    bool m_bReadBlockLocked;
    static CBufferDocument*	m_pCurrentReadDocument;		// track this so the producer has a clue about where is more important to fill blocks.
    CBufferWorker* m_pWorker;
    // the current block count required to start 
    int m_nPrebufBlocks;
    // the time to prebuffer in good network conditions.
    int m_nPrebufDefault;
    int m_nPrebufMax;

    // if the doc window is reanchored entirely, then the seek code will set up all the read pointers, and take
    // care of all but the actual semaphore wait, which will occur from the reader context.  so if true,
    // this indicates that LockBlockForReading can return with only trivial locking.
    bool m_bQuickLockReady;
};

#include "BufferReader.inl"

#endif // __PIS_INCLUDE_CONSUMER_DEFINED__
