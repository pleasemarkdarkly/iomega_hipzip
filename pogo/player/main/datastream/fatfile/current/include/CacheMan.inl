// inline members of CCacheMan

inline void CCacheMan::Destroy()
{
	delete m_pInstance;
}

inline void CCacheMan::SetPastBuffersToFill(int nPast)
{   
    m_nRebufferPastBuffersToFill = nPast;
}

inline CCacheReferee* CCacheMan::GetCacheReferee()
{
    return m_pCacheReferee;
}

inline void CCacheMan::SetBorrowingEmpty(bool borrowing)
{
    m_bConsumerBorrowingEmpty = borrowing;
}

inline bool CCacheMan::IsBorrowingEmpty()
{
    return m_bConsumerBorrowingEmpty;
}

inline int CCacheMan::GetPastBuffersToFill()
{
    return m_nRebufferPastBuffersToFill;
}

inline bool CCacheMan::IsFirstSpinup()
{
    return m_bFirstSpinup;
}

inline void CCacheMan::SetFirstSpinup(bool first)
{
    m_bFirstSpinup = first;
}

inline CacheListIterator CCacheMan::GetCurrentCache()
{
    return CacheListIterator(m_itCurrentCache);
}

inline CacheListIterator CCacheMan::GetCacheHead()
{
    return m_lstCaches.GetHead();
}

inline void CCacheMan::AppendFreeBuffers(BufferList* plstBuffers)
{
    m_lstUnusedBuffers.Append(*plstBuffers);
}

inline int BufferCountFromCharCount(int nChars)
{
    int nBufs = nChars / BUFFER_SIZE;
    if (nChars % BUFFER_SIZE)
        ++nBufs;
    return nBufs;
}