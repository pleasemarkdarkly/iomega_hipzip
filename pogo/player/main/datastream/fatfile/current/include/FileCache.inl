inline bool CFileCache::IsSkipEOF()
{
    return m_bSkipEOF;
}

inline bool CFileCache::ReachedBuffersToSkip()
{
    return (m_nSkipStart == m_nNextEmptyBuffer->Get());
}

inline IFileNameRef* CFileCache::GetFileNameRef()
{
    return m_pFileNameRef;
}

inline bool CFileCache::IsFileOpen()
{
    return m_bFileOpen;
}

inline int CFileCache::GetPosition()
{
    return m_nFileOffset;
}

inline BufferListIterator CFileCache::GetReadBufferIterator()
{
    return m_itNextFullBuffer;
}

inline bool CFileCache::IsCullable()
{
    return m_bCullable;
}

inline void CFileCache::SetCullable(bool bCullable)
{
    m_bCullable = bCullable;
}

inline int CFileCache::CountFullBuffers()
{
    int nCount = m_nChars / BUFFER_SIZE;
    if (m_nChars % BUFFER_SIZE)
        ++nCount;
    return nCount;
}

inline bool CFileCache::IsBOFBuffered()
{
    return ((m_nStartChar == 0) && (m_nChars > 0));
}

inline bool CFileCache::AreBuffersBOFRooted()
{
    return (m_nStartChar == 0);
}

inline int CFileCache::GetStartChar()
{
    return m_nStartChar;
}

inline BufferList* CFileCache::GetBufferList()
{
    return &m_lstBuffers;
}

inline bool CFileCache::IsCurrentCache()
{
    return (this == (*CCacheMan::GetInstance()->GetCurrentCache()));
}

inline bool CFileCache::FileIsCorrupt()
{
    return m_bFileCorrupt;
}

inline void CFileCache::SetFileCorrupt(bool bCorrupt)
{
    m_bFileCorrupt = bCorrupt;
}

inline bool CFileCache::IsEOFBuffered()
{
    return m_bEOF;
}

inline void CFileCache::SetFileEOF()
{
    m_bEOF = true;
}

inline int CFileCache::CacheCeiling()
{
    return m_nChars + m_nStartChar - 1;
}

inline bool CFileCache::OffsetIsBuffered (int nFileOffset)
{
    return ((nFileOffset >= m_nStartChar) && (nFileOffset <= m_nStartChar + m_nChars));
}

inline int CFileCache::GetFileOffset()
{
    return m_nFileOffset;
}

inline int CFileCache::GetBufferOffset()
{
    return m_nBufferOffset;
}

inline void CFileCache::SetFileOffset(int nOffset)
{
    m_nFileOffset = nOffset;
}

inline void CFileCache::SetBufferOffset(int nOffset)
{
    m_nBufferOffset = nOffset;
}

inline int CFileCache::GetPartialBufferIndex()
{
    return m_nPartialIndex;
}

inline int CFileCache::GetPartialBufferChars()
{
    return m_nPartialChars;
}

inline int CFileCache::GetCharCount()
{
    return m_nChars;
}

inline CCircIndex* CFileCache::GetNextFullBufferCircIndex()
{
    return m_nNextFullBuffer;
}

inline CCircIndex* CFileCache::GetNextEmptyBufferCircIndex()
{
    return m_nNextEmptyBuffer;
}

inline void CFileCache::SetPartialBufferIndex(int nIndex)
{
    m_nPartialIndex = nIndex;
}