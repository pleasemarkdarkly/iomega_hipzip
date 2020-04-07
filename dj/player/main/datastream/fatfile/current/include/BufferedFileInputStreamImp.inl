inline bool CBufferedFatFileInputStreamImp::CanSeek() const 
{ 
    return true; 
}

inline int CBufferedFatFileInputStreamImp::GetInputUnitSize() 
{ 
    return 512; 
}

inline void CBufferedFatFileInputStreamImp::SetConservativeMode() 
{ 
    m_pCacheMan->SetConservativeMode(); 
}

inline void CBufferedFatFileInputStreamImp::SetAggressiveMode() 
{ 
    m_pCacheMan->SetAggressiveMode(); 
}

inline void CBufferedFatFileInputStreamImp::SetNearEndOfTrack() 
{ 
    m_bNearEndOfTrack = true; 
}

inline CProducerMsgPump* CBufferedFatFileInputStreamImp::GetProducerMsgPump() 
{ 
    return m_pProducerMsgPump; 
}

inline CConsumerMsgPump* CBufferedFatFileInputStreamImp::GetConsumerMsgPump() 
{ 
    return m_pConsumerMsgPump; 
}

inline bool CBufferedFatFileInputStreamImp::IsNearEndOfTrack() 
{ 
    return m_bNearEndOfTrack; 
}

inline CBufferedFatFileInputStreamImp* CBufferedFatFileInputStreamImp::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CBufferedFatFileInputStreamImp;
	return m_pInstance;
}

inline int CBufferedFatFileInputStreamImp::Seek(InputSeekPos Origin, int Offset)
{
	return m_pConsumerMsgPump->Seek(Origin,Offset);
}

inline int CBufferedFatFileInputStreamImp::Read(void* pBuffer,int dwBytes)
{
	return m_pConsumerMsgPump->Read(pBuffer,dwBytes);
}


inline void CBufferedFatFileInputStreamImp::Destroy()
{
	delete m_pInstance;
}

inline ERESULT CBufferedFatFileInputStreamImp::Close()
{
    return INPUTSTREAM_NO_ERROR;
}

inline int CBufferedFatFileInputStreamImp::Position() const
{
    return (*m_pCacheMan->GetCurrentCache())->GetPosition();
}

inline int CBufferedFatFileInputStreamImp::Ioctl(int, void*)
{ return 0; }

inline int CBufferedFatFileInputStreamImp::Length() const 
{ 
    return (*m_pCacheMan->GetCurrentCache())->FileLength(); 
}

// (epg,10/19/2001): empty stub to avoid a compiler warning
inline void NoOp() { return; }

inline ERESULT CBufferedFatFileInputStreamImp::Open(const char* szFilename)
{
    diag_printf( "BIS:open url not supported\n" );
}
