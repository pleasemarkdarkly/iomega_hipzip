#include <datastream/dataplayfile/DPInputStream.h>
#include <datastream/dataplayfile/DPFile.h>

#define NULL 0

REGISTER_INPUTSTREAM(CDPInputStream,DPFILE_INPUT_ID);

CDPInputStream::CDPInputStream() 
{
    m_pFile = NULL;
}

CDPInputStream::~CDPInputStream() 
{
    Close();
}

int CDPInputStream::Open( const char* Source ) 
{
    if( m_pFile ) return 0;
    m_pFile = new CDPFile;

    return (int) m_pFile->Open( Source );
}

int CDPInputStream::Close() 
{
    if( m_pFile == NULL ) return 0;
    return (int) m_pFile->Close();
}

int CDPInputStream::Read( void* Buffer, int Count ) 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Read( Buffer, Count );
}

int CDPInputStream::Ioctl( int Key, void* Value )
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Ioctl( Key, Value );
}

int CDPInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Seek( (CDPFile::FileSeekPos)SeekOrigin, Offset );
}

int CDPInputStream::Length() const
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Length();
}

int CDPInputStream::Position() const 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->GetOffset();
}

