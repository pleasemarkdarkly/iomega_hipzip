// FileOutputStream.cpp: implementation for CFatFileOutputStream class
// danc@iobjects.com 07/09/01
// (c) Interactive Objects

#include <datastream/fatfile/FileOutputStream.h>
#include <datastream/fatfile/FatFile.h>

#define NULL 0

REGISTER_OUTPUTSTREAM( CFatFileOutputStream, FATFILE_OUTPUT_ID );

CFatFileOutputStream::CFatFileOutputStream() 
{
    m_pFile = NULL;
}

CFatFileOutputStream::~CFatFileOutputStream() 
{
    Close();
}

ERESULT CFatFileOutputStream::Open( const char* Source ) 
{
    if( m_pFile ) return 0;
    m_pFile = new CFatFile;

    return m_pFile->Open( Source, CFatFile::WriteCreate ) ? OUTPUTSTREAM_NO_ERROR : OUTPUTSTREAM_ERROR;
}

ERESULT CFatFileOutputStream::Close() 
{
    if( m_pFile == NULL ) return 0;
    bool bRetVal = m_pFile->Close();
    delete m_pFile;
    m_pFile = NULL;
    return bRetVal ? OUTPUTSTREAM_NO_ERROR : OUTPUTSTREAM_ERROR;
}

int CFatFileOutputStream::Write( const void* Buffer, int Count ) 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Write( Buffer, Count );
}

int CFatFileOutputStream::Ioctl( int Key, void* Value )
{
    if( m_pFile == NULL ) return 0;
    
    if( Key == FATFILE_OUTPUT_IOCTL_TRUNCATE ) {
        return m_pFile->Ioctl( FATFILE_IOCTL_TRUNCATE, Value );
    } else {
        return m_pFile->Ioctl( Key, Value );
    }
}

bool CFatFileOutputStream::Flush()
{
	if( m_pFile == NULL ) return 0;
	return m_pFile->Flush();
}

int CFatFileOutputStream::Seek( OutputSeekPos SeekOrigin, int Offset ) 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Seek( (CFatFile::FileSeekPos)SeekOrigin, Offset );
}
