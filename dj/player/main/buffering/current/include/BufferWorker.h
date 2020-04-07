#ifndef __PIS_INCLUDE_CACHEMAN_DEFINED__ 
#define __PIS_INCLUDE_CACHEMAN_DEFINED__

#include <util/datastructures/SimpleList.h>
#include <main/buffering/DJConfig.h>
#include <main/buffering/BufferTypes.h>
#include <main/buffering/BufferFactory.h>

class CBufferAccountant;
class CBufferDocument;
class IPlaylistEntry;
class CBufferFactory;
class CDocOrderAuthority;
class IMediaContentRecord;

typedef SimpleList<unsigned char *> BlockList;
typedef SimpleListIterator<unsigned char*> BlockListIterator;
typedef SimpleList<CBufferDocument*> DocumentList;
typedef SimpleListIterator<CBufferDocument*> DocumentListIterator;

int BlockCountFromCharCount(int nChars);

class CBufferWorker
{
public:
    friend class CBufferFactory;
    friend class CBufferReader;
    friend class CBufferWriter;

    void SetReadPriority(int nPrio);
    void SetWritePriority(int nPrio);

    // track how many chunks to fill before the read point.
    void SetOldBlocksToWrite(int nPast);
    int GetOldBlocksToWrite();

    // (epg,4/9/2002): this is an optional component, factory-ize it.
    // allow the reader to utilize the least old chunk, which the user has verified is really valid, old data.
    // manage a borrowed invalid chunk
    void BorrowInvalidBlock(bool bBorrowing);
    void ReturnInvalidBlock();
    bool OwedInvalidBlock();

    // return the document accountant
    CBufferAccountant* GetDocAccountant();

    // random utility : return the index of the chunk in the file, based on the address of the chunk.
    int BlockIndexFromLocation(unsigned char* pBlock);

    // (epg,4/9/2002): optional component, factory treatment + implement later.
    // consult a document ordering authority to prioritize documents.
    void SyncDocumentOrder(IMediaContentRecord* mcr);

    // similar to sync, but assumes current track is ok.
    void ResyncDocumentOrder();

    // return the document most in need of filling.
    CBufferDocument* GetDocumentToWrite();
    // return the current playfile's document
    DocumentListIterator GetActiveDocument();

    // manage the list of documents
    // return the first document in the list.
    DocumentListIterator GetDocumentHead();
    // return a document iterator associated with the given document.
    DocumentListIterator GetDocumentByDocument(CBufferDocument* pDoc);

    // manage the pool of chunks
    // set up initial buffer pointers
    void InitUnusedBlockList();
    // append a list of blocks from a document to the list of freely available blocks
    void PutFreeBlocks(BlockList* plstBlocks);
    // return a list of new blocks to the caller
    int GetFreeBlocks(int nNeeded, BlockList* plstBlocks, bool bPrepend = false);
    // cull blocks from caches that don't need them
    void ReclaimDeadBlocks();
    // pass out free chunks to interested documents.
    void DistributeFreeBlocks();

    // manage set of document sources
    // return a list of sources in need of documents
    StringList* GetUrlsToDocument(CDocOrderAuthority* pDocOrderAuth, IMediaContentRecord* mcr);
    // attach list of source references as the workload of this worker
    void AttachDocumentSources(StringList* plstUrls);

    // count forward through the source list to see how far we can extend the initial window 
    void SetDistantDocumentsExpendable();

    // starting from the read point, how many contiguous, full blocks are available
    void ResetValidityCounts();

    // check on the status of the active document
    bool ActiveDocReady();
    bool ActiveDocValid();
    // get an active document ready for consumption.
    void PrepareActiveDoc();

    // close out all document sources
    void CloseAllSources();

    // close out all sources on the hard drive.
    void CloseSeekableSources();

    // close all files and invalidate all blocks
    void DetachAllSources();

    // how many chunks are needed to extend the window to the source end limit?
    int CurrentDocumentShortfall();

    void SetConfig(tBufferConfig* desc);
    tBufferConfig* GetConfig();

public:
    
    // only the factory may create a bufferworker.
    CBufferWorker();
	~CBufferWorker();
	
    // figure out if there is any work to do
    bool IsBlockToFill();

    // print a debug status on docs
    void ReportOnDocument(CBufferDocument* pDoc, int idx);
    void ReportOnAllDocuments(char* szCaption);

    // return a count of valid chunks held, and any fairly young invalid chunks
    int CountUsefulBlocks();

    // are we actively writing chunks, or waiting dormant.
	bool m_bAwake;

    // is the reader dipping into the youngest pre-readpoint buffer?
	bool m_bReaderUsingPrevBlock;

    // (epg,4/9/2002): very specific to impl.  can be encapsulated down?
    // counts of false semaphore inflations used to wake a sem-blocker
	int m_nFakeInvalidBlocks;
	int	m_nFakeValidBlocks;

    // how many chunks will we write before the re-anchor point?
	short m_nReAnchorPastBlocks;

    // a managed count utility
	CBufferAccountant* m_pAccountant;			

    // where blocks live
	unsigned char* m_bfBlockRoom;
    // how many blocks to break the room into
    int m_cBlocks;
    // how many total bytes to break into blocks
    int m_cBytes;
    // how many source docs can have windows open
    int m_cMaxSources;

    int m_nPastDocs;
    // currently unused chunks
    BlockList m_lstUnusedBlocks;

    // list of documents currently handled by this worker
    DocumentList m_lstDocuments;

    // the active document
    DocumentListIterator m_itrActiveDoc;

    // support for doc-endpoint windows
    bool m_bDocStartWindow;
    bool m_bDocEndWindow;
    int m_nDocStartWindowBlocks;
    int m_nDocEndWindowBlocks;

    // support for optional threadedness
    bool m_bInputThreaded;
    int m_nInputPriority;
    bool m_bOutputThreaded;
    int m_nOutputPriority;


    // are this workers' documents seekable?
    bool m_bSeekable;

    tBufferConfig* m_pConfig;

    // what order are the docs in?
    CDocOrderAuthority* m_pDocOrderAuthority;
};

#include "BufferWorker.inl"

#endif // __PIS_INCLUDE_CACHEMAN_DEFINED__
