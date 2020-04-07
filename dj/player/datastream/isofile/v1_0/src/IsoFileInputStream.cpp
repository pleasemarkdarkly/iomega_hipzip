// IsoFileInputStream.cpp: ISO file input stream
// edwardm@iobjects.com 07/21/01
// (c) Interactive Objects

#include <datastream/isofile/IsoFileInputStream.h>
#include <util/debug/debug.h>

#include <cyg/fileio/fileio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>      /* errno */
    
REGISTER_INPUTSTREAM(CIsoFileInputStream, ISOFILE_INPUT_ID);

DEBUG_MODULE_S( ISOFILEINPUTSTREAM, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( ISOFILEINPUTSTREAM );

#define NO_FILE -1

#define SHOW_RESULT( _fn, _res ) \
DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "<FAIL>: " #_fn "() returned %d %s\n", _res, _res<0?strerror(errno):"");

CIsoFileInputStream::CIsoFileInputStream() 
    : m_fd(NO_FILE)
{
    m_iPosition = 0;
}

CIsoFileInputStream::~CIsoFileInputStream() 
{
    Close();
}

ERESULT CIsoFileInputStream::Open( const char* Source ) 
{
    DEBUG(ISOFILEINPUTSTREAM, DBGLEV_TRACE, "Opening file %s\n", Source);

    if (m_fd != NO_FILE)
    {
        DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "Can't open %s; another file is already open\n", Source);
        return INPUTSTREAM_ERROR;
    }

    m_fd = open(Source, O_RDONLY);
    if (m_fd < 0)
    {
        DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "Failed to open file %s: %s\n", Source, strerror(errno));
        return INPUTSTREAM_ERROR;
    }

    m_lLength = Length();
    
    return INPUTSTREAM_NO_ERROR;
}

ERESULT CIsoFileInputStream::Close() 
{
    if (m_fd == NO_FILE)
    {
        return INPUTSTREAM_ERROR;
    }

    int err = close(m_fd);
    m_fd = NO_FILE;
    m_iPosition = 0;
    if (err < 0)
    {
        SHOW_RESULT(open, m_fd);
        return INPUTSTREAM_ERROR;
    }

    return INPUTSTREAM_NO_ERROR;
}

int CIsoFileInputStream::Read( void* Buffer, int Count ) 
{
    if ((m_fd == NO_FILE) || (m_iPosition >= m_lLength))
        return 0;
    int iBytesRead = read(m_fd, Buffer, Count);


    if (iBytesRead <= 0)
    {		
		DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "data cd read error %d\n",iBytesRead);
		return -1;
    }
	else
	{
		m_iPosition += iBytesRead;
	}

    return iBytesRead;
}

int CIsoFileInputStream::Ioctl( int Key, void* Value )
{
    if (m_fd == NO_FILE)
        return 0;
    return 0;
}

int CIsoFileInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    int pos;
    if (m_fd == NO_FILE)
    {
        DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "No file\n");
        return -1;
    }

    switch (SeekOrigin)
    {
        case SeekCurrent:
            pos = lseek(m_fd, Offset, SEEK_CUR);
            break;
        case SeekStart:
            pos = lseek(m_fd, Offset, SEEK_SET);
            break;
        case SeekEnd:
            pos = lseek(m_fd, Offset, SEEK_END);
            break;
        default:
            return -1;
    };

    m_iPosition = pos;
    return m_iPosition;
}

int CIsoFileInputStream::Length() const
{
    struct stat buf;
    if (fstat(m_fd, &buf) == 0)
        return buf.st_size;
    else
        return -1;
}

int CIsoFileInputStream::Position() const 
{
    if( m_fd == NO_FILE )
    {
        DEBUG(ISOFILEINPUTSTREAM, DBGLEV_ERROR, "No file\n");
        return -1;
    }
    return m_iPosition;
}

