inline int CBufferingImp::GetInputUnitSize() 
{ 
    return 512; 
}

inline void CBufferingImp::SetNearEndOfTrack() 
{ 
    m_bNearEndOfTrack = true; 
}

inline CWriterMsgPump* CBufferingImp::GetWriterMsgPump() 
{ 
    return m_pWriterMsgPump; 
}

inline CReaderMsgPump* CBufferingImp::GetReaderMsgPump() 
{ 
    return m_pReaderMsgPump; 
}

inline bool CBufferingImp::IsNearEndOfTrack() 
{ 
    return m_bNearEndOfTrack; 
}

inline CBufferingImp* CBufferingImp::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CBufferingImp;
	return m_pInstance;
}


inline int CBufferingImp::Ioctl(int, void*)
{ return 0; }


// (epg,10/19/2001): empty stub to avoid a compiler warning
inline void NoOp() { return; }

