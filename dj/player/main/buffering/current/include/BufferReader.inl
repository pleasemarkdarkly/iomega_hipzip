inline void CBufferReader::ClearReadBlockLock()
{
    m_bReadBlockLocked = false;
    m_bQuickLockReady = false;
}
