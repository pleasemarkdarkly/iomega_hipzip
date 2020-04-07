// inline members of CBufferWorker

inline void CBufferWorker::SetOldBlocksToWrite(int nPast)
{   
    m_nReAnchorPastBlocks = nPast;
}

inline CBufferAccountant* CBufferWorker::GetDocAccountant()
{
    return m_pAccountant;
}

inline void CBufferWorker::BorrowInvalidBlock(bool borrowing)
{
    m_bReaderUsingPrevBlock = borrowing;
}

inline bool CBufferWorker::OwedInvalidBlock()
{
    return m_bReaderUsingPrevBlock;
}

inline int CBufferWorker::GetOldBlocksToWrite()
{
    return m_nReAnchorPastBlocks;
}

inline DocumentListIterator CBufferWorker::GetActiveDocument()
{
    return DocumentListIterator(m_itrActiveDoc);
}

inline DocumentListIterator CBufferWorker::GetDocumentHead()
{
    return m_lstDocuments.GetHead();
}

inline void CBufferWorker::PutFreeBlocks(BlockList* plstBlocks)
{
    m_lstUnusedBlocks.Append(*plstBlocks);
}

inline int BlockCountFromCharCount(int nChars)
{
    int nBlocks = nChars / BUFFER_BLOCK_BYTES;
    if (nChars % BUFFER_BLOCK_BYTES)
        ++nBlocks;
    return nBlocks;
}

inline tBufferConfig* CBufferWorker::GetConfig()
{
    return m_pConfig;
}

inline void CBufferWorker::SetConfig(tBufferConfig* pConfig)
{
    m_pConfig = pConfig;
}
