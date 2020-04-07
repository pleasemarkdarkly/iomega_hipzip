// CDDAInputStream.cpp: CDDA file input stream
// edwardm@iobjects.com 07/11/01
// (c) Interactive Objects


#include <datastream/cddafile/CDDAInputStream.h>

#include <datasource/cddatasource/CDDataSource.h>
#include <util/debug/debug.h>

DEBUG_MODULE_S(CDDAINPUTSTREAM, DBGLEV_DEFAULT | DBGLEV_INFO | DBGLEV_TRACE);
DEBUG_USE_MODULE(CDDAINPUTSTREAM);

CCDDAInputStream::CCDDAInputStream()
{
}

CCDDAInputStream::~CCDDAInputStream()
{
}

ERESULT
CCDDAInputStream::Open( const char* Source )
{
    return INPUTSTREAM_ERROR;
}

ERESULT
CCDDAInputStream::Open(CCDDataSource* pDataSource, int iLBAStart, int iLBALength)
{
    m_pDataSource = pDataSource;
    m_iLBAStart = iLBAStart;
    m_iLBAEnd = iLBAStart + iLBALength;
    m_iLBACurrent = iLBAStart;

    return INPUTSTREAM_NO_ERROR;
}

ERESULT
CCDDAInputStream::Close()
{
    return INPUTSTREAM_NO_ERROR;
}

int
CCDDAInputStream::Read( void* Buffer, int Count )
{
    // Calculate the nearest even multipule of sectors.
    int iSectors = Count / 2352;

    // m_iLBAEnd is non-inclusive; that is, if you read m_iLBAEnd you get the first audio
    //  sector of the next track (or some inter-track gap)
    if (iSectors + m_iLBACurrent >= m_iLBAEnd)
    {
        iSectors = m_iLBAEnd - m_iLBACurrent;
    }
    
    if (!iSectors)
        return 0;
	
    int iRead = m_pDataSource->Read(m_iLBACurrent, iSectors, (void*)Buffer);
    if (iRead > 0)
    {
        m_iLBACurrent += iRead;
        return (iRead * 2352);
    }
    else
    {
#ifdef TODO // check media status
        DEBUGP(CDDAINPUTSTREAM, DBGLEV_ERROR, "Couldn't read sector %ld: %ld\n", m_iLBACurrent, 1);
        /* Do a media check to see if there's a problem, like no cd */
        Cyg_ErrNo err = GetQuickMediaStatus(true);
        /* Return if there is no media or if there was a media change */
        if ((err == -ENOMED) || (err == -EMEDCHG))
            return -1;
#else
        // Let the caller know that something's gone wrong.
        return -1;
#endif
    }
}

int
CCDDAInputStream::Ioctl( int Key, void* Value )
{
    return 0;
}


int
CCDDAInputStream::Seek( InputSeekPos SeekOrigin, int Offset )
{
    switch (SeekOrigin)
    {
        case SeekCurrent:
            m_iLBACurrent += Offset / 2352;
            break;
        case SeekStart:
            m_iLBACurrent = m_iLBAStart + (Offset / 2352);
            break;
        case SeekEnd:
            m_iLBACurrent = m_iLBAEnd - (Offset / 2352);
            break;
        default:
            return -1;
    };
    if (m_iLBACurrent < m_iLBAStart)
        m_iLBACurrent = m_iLBAStart;
    else if (m_iLBACurrent >= m_iLBAEnd)
        m_iLBACurrent = m_iLBAEnd - 1;  // back off the end a little
	
    return Position();
}

