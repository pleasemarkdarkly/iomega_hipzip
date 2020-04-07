// FileInputStream.cpp: file input wrapper for CFatFile
// danc@iobjects.com 07/09/01
// (c) Interactive Objects

#include <main/datastream/fatfile/BufferedFileInputStream.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <datastream/fatfile/FatFile.h>

REGISTER_INPUTSTREAM(CBufferedFatFileInputStream,FATFILE_BUFFEREDINPUT_ID);

CBufferedFatFileInputStream::CBufferedFatFileInputStream() 
{
    m_pImp = CBufferedFatFileInputStreamImp::GetInstance();
}

CBufferedFatFileInputStream::~CBufferedFatFileInputStream() 
{
    m_pImp->Destroy();
}

ERESULT CBufferedFatFileInputStream::Open( const char* Source ) 
{
    return m_pImp->Open(Source);
/*    if( m_pImp ) return INPUTSTREAM_ERROR;
    m_pImp = new CFatFile;

    return m_pImp->Open( Source ) ? INPUTSTREAM_NO_ERROR : INPUTSTREAM_ERROR;
    */
}

ERESULT CBufferedFatFileInputStream::Close() 
{
    return m_pImp->Close();
    /*if( m_pImp == NULL ) return INPUTSTREAM_ERROR;
    bool bRetVal = m_pImp->Close();
    delete m_pImp;
    m_pImp = NULL;
    return bRetVal ? INPUTSTREAM_NO_ERROR : INPUTSTREAM_ERROR;
    */
}

int CBufferedFatFileInputStream::Read( void* Buffer, int Count ) 
{
    return m_pImp->Read(Buffer,Count);
    /*
    if( m_pImp == NULL ) return 0;
    return m_pImp->Read( Buffer, Count );
    */
}

int CBufferedFatFileInputStream::Ioctl( int Key, void* Value )
{
    return m_pImp->Ioctl(Key, Value);
    /*if( m_pImp == NULL ) return 0;
    return m_pImp->Ioctl( Key, Value );
    */
}

int CBufferedFatFileInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    return m_pImp->Seek(SeekOrigin, Offset);
    /*
    if( m_pImp == NULL ) return 0;
    return m_pImp->Seek( (CFatFile::FileSeekPos)SeekOrigin, Offset );
    */
}

int CBufferedFatFileInputStream::Length() const
{
    return m_pImp->Length();
    /*
    if( m_pImp == NULL ) return 0;
    return m_pImp->Length();
    */
}

int CBufferedFatFileInputStream::Position() const 
{
    return m_pImp->Position();
    /*
    if( m_pImp == NULL ) return 0;
    return m_pImp->GetOffset();
    */
}

