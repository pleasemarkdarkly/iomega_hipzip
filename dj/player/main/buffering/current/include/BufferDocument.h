#ifndef __PIS_INCLUDE_FC_DEFINED__ 
#define __PIS_INCLUDE_FC_DEFINED__

#include <main/buffering/CircIndex.h>
#include <main/buffering/BufferWorker.h>
#include <main/buffering/BufferWriter.h>
#include <util/debug/debug.h>
#include <main/buffering/BufferFactory.h>

class CBufferWorker;
class CBufferAccountant;
class IInputStream;
class tBufferConfig;

class CBufferDocument
{
public:
    // (epg,4/11/2002): TODO: remove these when time allows
    friend class CBufferWorker;
    friend class CBufferingImp;
    friend class CBufferReader;
    friend class CBufferWriter;

    CBufferDocument();
    ~CBufferDocument();

    void SetWorker(CBufferWorker* pWorker);

    void ReleaseBlocks();
    // designate this document's file source.
    void AttachFile(char* szUrl);
    // either delete the url or the fns entry, depending on subclassing.
    void DetachSource();
    
    // necessary?  seems like an addon interface.
    bool IsSourceCorrupt();
    void SetSourceCorrupt(bool);

    // track the rootedness of the source
    bool SourceAtDocEnd();
    // do the blocks start at the DocStart, and is there some data there?
    bool HaveDocStartData();
    // do the blocks start at the DocStart (maybe with no data)?
    bool WindowAtDocStart();
    
    // prepend blocks from those available to extend the window to the bof.
    void BackfillToDocStart();
    // notify that we've reached the end of the input.
    void SetDocEnd();
    // move the read point to the bof, re-aiming the blocks if req'd.
    void JumpToDocStart();

    // (epg,4/9/2002): todo: merge according to a subclass & optional interface.  ties in with factory
    // report on the attached source
    char* GetSourceUrl();

    // manage the window boundaries
    // return the index of the lowest byte held.
    int FirstWindowByte();
    // return the highest byte held in memory
    int LastWindowByte();
    // check if a source-index is held
    bool ByteInWindow (int nByte);

    // extend the current window to the desired index
	bool ForwardFill(int nByte);

    bool SeekInWindow(int nByte, int nCount, short iUnitDirection, bool bBorrowingInvalid);

    // throw away held data and move to nByte
    bool MoveWindow(int nByte);

    int InvalidateAndReAnchorWindow( unsigned int nSoughtFileOffset );
    
    // manage data that becomes dis-contiguous with the window, but might be needed later
    // does the orphaned data contain the doc-end?
    bool DocEndOrphan();
    // reinstate the source completion state as a result of doc-end orphan data
    // here, an orphan window containing doc-end is being reincorporated.
    void RecountDocEndOrphan();
    // during the course of filling blocks, we have run across some orhaned data that will substitute for source reads.
    bool ReachedOrphanWindow();
    // add the orphaned doc-end data back into the window.
    void ReclaimOrphanWindow(CBufferAccountant* pRef);
    
    // manage disk read errors
    int  HandleHardError(unsigned char* pBlock);
    // helpers for error recovery
    bool InsertBlockDeadSpace(unsigned char* pBlock, int &nBytesToRead, int &nBytesAlreadyRead);
    bool ReReadBeforeHardError(unsigned char* pBlock, int nPreReadPos, int &nNextGoodLBAStart);
    
    // return enum status result of reading the source and extending the window
    eBWResult WriteBlock();

    // release some blocks for other documents
    int GiveUpBlocks(int nRequested, bool bFromHead = false);
    
    // get a reference to the current buffer chunk.
    BlockListIterator GetReadBlockIterator();

    // manage the looping counters
    void IncrementNextValidBlock();
    void DecrementNextValidBlock();
    void IncrementNextInvalidBlock();
    void DecrementNextInvalidBlock();
    // reset the read + write buffer indexes to zero
    void ResetNextValidBlock();
    void ResetNextInvalidBlock();
    // return the next valid/invalid chunk index

    // is this document the active one?
    bool IsDocumentActive();
    
    // return the list of blocks in this doc
    BlockList* GetBlockList();
    
    // check the read position
    int GetReadByte();
    int GetBlockReadByte();

    // set the read position
    void SetReadByte(int);
    void SetBlockReadByte(int);

    // manage a final partial chunk of DocEnd data
    void SetPartialBlockIterator(BlockListIterator itBlock);
    BlockListIterator GetPartialBlockIterator();

    int GetPartialBlockChars();

    // return the count of bytes held in the window
    int GetWindowBytes();

    // count how many chunks are so old as to be considered dead-wood.
    int CountDeadBlocks();
    // how many bytes of old data are in the window?
    int CountOldChars();

    // sync the invalid chunk list-iterator to the right index
    void SyncNextInvalidBlock();

    // have we read all we can from the source?
    bool AreAllBlocksValid();

    // debugging report
    void PrintStateReport(char* caption);

    void ZeroOutWriteBlock (int nStart, int nBytes);

    // is the source positioned where we want to read?
    bool SourceOffsetValid();

    // return true if the underlying input stream can seek.
    bool CanSourceSeek();

    // Report on source length.  If length is 0 and this is a net stream, return ARBITRARILY_LARGE_DOCSIZE.
    int BufferLength();

private:
    // normalize state to start values
    void Initialize();

    // put blocks after eof back into the central pool.
    void ReleasePostEOFBlocks();

    // place the file index at the next point to be buffered
    void ThrottleFileIndex();
    // (epg,4/9/2002): todo: merge according to a subclass & optional interface.  ties in with factory
    // store the attached source moniker (private, just a helper for attach file)
    void SetSourceUrl(char* szUrl);
    
    // manage the opened-state of the input source
    bool OpenSource();
    bool IsSourceOpen();
    void CloseSource();
    // report on source length
    int Length();

    // return the count of blocks in the window (seems overly specific, could be bytes in window, perhaps this is an internal only)
    int CountValidBlocks();

    // convert a chunk list index into a file byte
    int FilePositionFromBlockListIndex(int nListIndex);
    
    // is the invalid chunk iterator pointing off the end of the chunk list?
    bool m_bNextInvalidOffEnd;
    // is the valid chunk iterator pointing off the end of the chunk list?
    bool m_bNextValidOffEnd;

    // track the read point in the buffer, relative to the document start.
	int m_nReadByte;
	int	m_nBlockReadByte;

    // the manager of all documents
    CBufferWorker* m_pWorker;
    // the source for this document
    IInputStream* m_pInputStream;

    // maintain the barriers between valid and invalid chunks
    BlockListIterator m_itNextValidBlock;
    BlockListIterator m_itNextInvalidBlock;

    // maintain the active window
    int m_nStartByte;
	int	m_nBytesValid;

    // maintain the partial 
    BlockListIterator m_itPartialBlock;
	int	m_nPartialChars;
	
    // track an orphaned window on the source, in case it becomes important again
    BlockListIterator m_itOrphanBlockStart;
	short m_cOrphanBlocks;

    // track an orphaned partial doc-end chunk
	bool m_bDocEndOrphaned;
	int	m_cOrphanedEndDocBytes;

    // has the end of the document been reached?
	bool m_bDocEnd;
    // is the input source totally corrupt and unreadable?
	bool m_bSourceCorrupt;

    // track how many unreadable segments we encounter
    int m_nHardErrors;
    // track the position of the last null-segment inserted to punctuate a hard-error location
    int  m_nLastDeadSpace;

    // list of chunks
    BlockList m_lstBlocks;

    // the source identifier
    char* m_szSourceUrl;

    // track whether the source is opened
    bool m_bSourceOpen;
    // the length of the file as reported by the input stream
    int m_nLength;
    // return the count of chars available from the source.
    int m_nSourceBytes;
    // the number of bytes the (unseekable) stream is ahead of the desired write-point
    int m_nDroppedStreamBytes;
    // how many times have we tried to reopen this url after failing in the first block fill?
    int m_nReOpens;
};

#include "BufferDocument.inl"

#endif // __PIS_INCLUDE_FC_DEFINED__
