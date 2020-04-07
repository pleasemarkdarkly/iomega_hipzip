// FatFile.cpp: definition for fat layer wrapper
// danc@iobjects.com 07/09/01
// (c) Interactive Objects

#include <fs/fat/sdapi.h>
#include <datastream/fatfile/FatFile.h>
#include <util/debug/debug.h>

#include <string.h> // strcpy

DEBUG_MODULE_S(FATFILE, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(FATFILE);

#define FAT_NO_FILE -1
#define FAT_ERROR (UCOUNT)~0

#define tolower(ch)    ( ((ch) >= 'A') && ((ch) <= 'Z') ? (ch) - 'A' + 'a' : ch )

CFatFile::CFatFile() : m_nSafeLen(0)
{
    m_FD = FAT_NO_FILE;
    m_iPosition = 0;
    m_iLength = 0;
}

CFatFile::~CFatFile() 
{
    Close();
}

bool CFatFile::Delete( const char* Filename ) {
    return pc_unlink((char*)Filename) == YES;
}

bool CFatFile::Open( const char* Filename, FileMode Mode ) 
{
    if( !Filename ) {
        return false;
    }
    
    int mode = 0;

    switch( Mode ) {
        case ReadOnly:
        {
            m_iFlags = PO_RDONLY;
            mode = PS_IREAD;
            break;
        }
        case ReadWrite:
        {
            m_iFlags = PO_RDWR | PO_CREAT | PO_NOSHAREANY;
            mode = PS_IREAD | PS_IWRITE;
            break;
        }
        case WriteCreate:
        {
            m_iFlags = PO_WRONLY | PO_CREAT | PO_TRUNC | PO_NOSHAREANY;
            mode = PS_IWRITE;
            break;
        }
        case WriteAppend:
        {
            m_iFlags = PO_WRONLY | PO_APPEND | PO_NOSHAREANY;
            mode = PS_IWRITE;
            break;
        }
        case WriteOnly:
        {
            m_iFlags = PO_WRONLY | PO_NOSHAREANY;
            mode = PS_IWRITE;
            break;
        }
    }

    strcpy( m_Filename, Filename );
    m_FD = (int) po_open( (char*)m_Filename, m_iFlags, mode );
    
    if( m_FD >= 0 ) {
        short ignored;
        m_iLength = po_lseek( (PCFD) m_FD, 0, PSEEK_END, &ignored );
        m_iPosition = po_lseek( (PCFD) m_FD, 0, PSEEK_SET, &ignored );

        return true;
    }
    else
    {
#if DEBUG_LEVEL != 0
        static const char* s_modes[] =
        {
            "ReadOnly",
            "WriteOnly",
            "ReadWrite",
            "WriteCreate",
            "WriteAppend"
        };
        int err = pc_get_error(tolower(m_Filename[0]) - 'a');
        DEBUG(FATFILE, DBGLEV_WARNING, "Failed to open file %s for mode %d (%s): %d -- %s\n",
                                       m_Filename, Mode, s_modes[Mode],
                                       err, GetFatErrorString(err));
#endif
        return false;
    }
}

bool CFatFile::Close() 
{
    if( m_FD == FAT_NO_FILE ) {
        return false;
    }
    
    if (po_close( (PCFD) m_FD ) == -1)
    {
#if DEBUG_LEVEL != 0

        int err = pc_get_error(tolower(m_Filename[0]) - 'a');

        DEBUG(FATFILE, DBGLEV_WARNING, "Failed to close file %s: %d -- %s\n",
                                       m_Filename, 
                                       err, GetFatErrorString(err));
#endif
        return false;
    }
    m_FD = FAT_NO_FILE;

    return true;
}

int CFatFile::Read( void* Buffer, int Count ) 
{
    int res = 0;
    int StartPos = m_iPosition;
    
    if( m_FD == FAT_NO_FILE ) {
        return false;
    }
    // dc- workaround fatlayer issues
#if 1
    while( Count > 0 ) {
        int BytesToRead = (Count > 57344 ? 57344 : Count);
        res = po_read( (PCFD) m_FD, (UCHAR*)Buffer, BytesToRead );

        if (res != FAT_ERROR) {
            m_iPosition += res;
        } else {
            return -1;
        }
        Count -= BytesToRead;
        Buffer = ((UCHAR*)Buffer) + BytesToRead;
    }
#else

    res = po_read( (PCFD) m_FD, (UCHAR*)Buffer, Count );

	if (res != -1) {
		m_iPosition += res;
	}
	else
		return -1;

#endif
    return m_iPosition - StartPos;
}

int CFatFile::Write( const void* Buffer, int Count )
{
    int res = 0;
    int StartPos = m_iPosition;
    
    if( m_FD == FAT_NO_FILE ) {
        return false;
    }
#if 1
    while( Count > 0 ) {
        int BytesToWrite = (Count > 57344 ? 57344 : Count);
        res = po_write( (PCFD) m_FD, (UCHAR*) Buffer, BytesToWrite );

        if (res != FAT_ERROR)
        {
            m_iPosition += res;
            if (m_iPosition > m_iLength)
                m_iLength = m_iPosition;
        } else {
            res = -1;
            break;
        }
        Count -= BytesToWrite;
        Buffer = ((UCHAR*)Buffer) + BytesToWrite;
    }
#else
    res = po_write( (PCFD) m_FD, (UCHAR*)Buffer, Count );

    if (res != -1)
    {
        m_iPosition += res;
        if (m_iPosition > m_iLength)
            m_iLength = m_iPosition;
    }
	else
		return -1;

#endif
    return m_iPosition - StartPos;;
}

bool CFatFile::Flush()
{
    if( m_FD == FAT_NO_FILE )
    {
        return false;
    }
    else {
        return po_flush((PCFD) m_FD);
    }
    
}

int CFatFile::Ioctl( int Key, void* Value )
{
    if( Key == FATFILE_IOCTL_TRUNCATE ) {
        int val = (int)Value;
        if( po_truncate( (PCFD) m_FD, val ) ) {
            return 1;
        }
        else {
            return -1;
        }
    }
    return 0;
}

int CFatFile::Seek( FileSeekPos Origin, int Offset ) 
{
    int NewPosition;
    short err;

    if( m_FD == FAT_NO_FILE ) {
        return 0;
    }
    
    NewPosition = (int) po_lseek( (PCFD) m_FD, Offset, (INT16) Origin, &err );

    if( err == 0 ) {
        m_iPosition = NewPosition;
    }

    return m_iPosition;
}

int CFatFile::GetOffset() const
{
    if( m_FD == FAT_NO_FILE ) {
        return 0;
    }
    return m_iPosition;
}

int CFatFile::Length() const
{
    if( m_FD == FAT_NO_FILE ) return 0;

    return m_iLength;
}

int CFatFile::Truncate( int nSize )
{
    if( m_FD == FAT_NO_FILE ) return 0;
    if (nSize > Length()) {
        DEBUGP( FATFILE, DBGLEV_INFO, "FF:Asked to truncate past eof\n"); 
        return Length();
    }
    po_truncate(m_FD, nSize);
    return nSize;
}

// Guard read access to the true end of the file.
void CFatFile::SetSafeLen( int nLen )
{
    m_nSafeLen = nLen;
}
int CFatFile::GetSafeLen()
{
    return m_nSafeLen ? m_nSafeLen : Length();
}

#undef tolower
