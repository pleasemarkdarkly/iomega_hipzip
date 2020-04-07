// FileInputStream.cpp: file input wrapper for CFatFile
// danc@fullplaymedia.com 07/09/01
// (c) Fullplay Media Systems

#include <main/datastream/fatfile/BufferedFileInputStream.h>
#include <main/datastream/fatfile/BufferedFileInputStreamImp.h>
#include <datastream/fatfile/FatFile.h>

REGISTER_INPUTSTREAM(CBufferedFatFileInputStream,FATFILE_BUFFEREDINPUT_ID);

CBufferedFatFileInputStream* CBufferedFatFileInputStream::m_pInstance = 0;

CBufferedFatFileInputStream::CBufferedFatFileInputStream() 
{
    m_pImp = CBufferedFatFileInputStreamImp::GetInstance();
}

CBufferedFatFileInputStream::~CBufferedFatFileInputStream() 
{
    m_pInstance = 0;
    //m_pImp->Destroy();
}

ERESULT CBufferedFatFileInputStream::Open( const char* Source ) 
{
    return m_pImp->Open(Source);
}

ERESULT CBufferedFatFileInputStream::Close() 
{
    return m_pImp->Close();
}

int CBufferedFatFileInputStream::Read( void* Buffer, int Count ) 
{
    return m_pImp->Read(Buffer,Count);
}

int CBufferedFatFileInputStream::Ioctl( int Key, void* Value )
{
    return m_pImp->Ioctl(Key, Value);
}

int CBufferedFatFileInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    return m_pImp->Seek(SeekOrigin, Offset);
}

int CBufferedFatFileInputStream::Length() const
{
    return m_pImp->Length();
}

int CBufferedFatFileInputStream::Position() const 
{
    return m_pImp->Position();
}

bool CBufferedFatFileInputStream::SetSong( IMediaContentRecord* mcr )
{
    return m_pImp->SetSong(mcr);
}

CBufferedFatFileInputStream* CBufferedFatFileInputStream::GetInstance()
{
	if (!m_pInstance)
		m_pInstance = new CBufferedFatFileInputStream;
	return m_pInstance;
}

void CBufferedFatFileInputStream::Destroy()
{
	delete m_pInstance;
}

IInputStream* CBufferedFatFileInputStream::CreateInputStream( IMediaContentRecord* mcr )
{
    if (SetSong(mcr))
        return this;
    return (IInputStream*) NULL;
}
    