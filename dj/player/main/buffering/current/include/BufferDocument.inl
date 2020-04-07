inline bool CBufferDocument::DocEndOrphan()
{
    return m_bDocEndOrphaned;
}

inline bool CBufferDocument::ReachedOrphanWindow()
{
    return (m_itOrphanBlockStart == m_itNextValidBlock && m_itNextValidBlock != 0);
}

inline char* CBufferDocument::GetSourceUrl()
{
    return m_szSourceUrl;
}

inline bool CBufferDocument::IsSourceOpen()
{
    return m_bSourceOpen;
}

inline BlockListIterator CBufferDocument::GetReadBlockIterator()
{
    return m_itNextValidBlock;
}

inline int CBufferDocument::CountValidBlocks()
{
    int nCount = m_nBytesValid / BUFFER_BLOCK_BYTES;
    if (m_nBytesValid % BUFFER_BLOCK_BYTES)
        ++nCount;
    return nCount;
}

inline bool CBufferDocument::HaveDocStartData()
{
    // (epg,10/1/2002): I think there may have been a hole here if we get a zero length file.  in that case, 
    // the startbyte IS zero, but we wouldn't have any valid bytes in the count, and I wasn't checking doc-end,
    // which would be the case if we just got zero back from the stream right away.  and then logically we should
    // return true, as we don't need to rewind and read the beginning of the file.
    return ((m_nStartByte == 0) && ((m_nBytesValid > 0 || m_bDocEnd)));
}

inline bool CBufferDocument::WindowAtDocStart()
{
    return (m_nStartByte == 0);
}

inline int CBufferDocument::FirstWindowByte()
{
    return m_nStartByte;
}

inline BlockList* CBufferDocument::GetBlockList()
{
    return &m_lstBlocks;
}

inline bool CBufferDocument::IsDocumentActive()
{
    return (this == (*m_pWorker->GetActiveDocument()));
}

inline bool CBufferDocument::IsSourceCorrupt()
{
    return m_bSourceCorrupt;
}

inline void CBufferDocument::SetSourceCorrupt(bool bCorrupt)
{
    m_bSourceCorrupt = bCorrupt;
}

inline bool CBufferDocument::SourceAtDocEnd()
{
    return m_bDocEnd;
}

inline void CBufferDocument::SetDocEnd()
{
    m_bDocEnd = true;
}

inline int CBufferDocument::LastWindowByte()
{
    return m_nBytesValid + m_nStartByte - 1;
}

inline bool CBufferDocument::ByteInWindow (int nFileOffset)
{
    return ((nFileOffset >= m_nStartByte) && (nFileOffset <= m_nStartByte + m_nBytesValid));
}

inline int CBufferDocument::GetReadByte()
{
    return m_nReadByte;
}

inline int CBufferDocument::GetBlockReadByte()
{
    return m_nBlockReadByte;
}

inline void CBufferDocument::SetReadByte(int nOffset)
{
    m_nReadByte = nOffset;
}

inline void CBufferDocument::SetBlockReadByte(int nOffset)
{
    m_nBlockReadByte = nOffset;
}

inline int CBufferDocument::GetPartialBlockChars()
{
    return m_nPartialChars;
}
inline BlockListIterator CBufferDocument::GetPartialBlockIterator()
{
    return m_itPartialBlock;
}


inline int CBufferDocument::GetWindowBytes()
{
    return m_nBytesValid;
}

inline void CBufferDocument::SetPartialBlockIterator(BlockListIterator itPartialBlock)
{
    m_itPartialBlock = itPartialBlock;
}
