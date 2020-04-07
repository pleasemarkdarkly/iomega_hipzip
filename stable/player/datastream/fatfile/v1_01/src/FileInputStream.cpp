// FileInputStream.cpp: file input wrapper for CFatFile
// danc@iobjects.com 07/09/01
// (c) Interactive Objects

#include <datastream/fatfile/FileInputStream.h>
#include <datastream/fatfile/FatFile.h>

#define NULL 0

REGISTER_INPUTSTREAM(CFatFileInputStream,FATFILE_INPUT_ID);

CFatFileInputStream::CFatFileInputStream()
{
    m_pFile = NULL;
}

CFatFileInputStream::~CFatFileInputStream() 
{
    Close();
}

ERESULT CFatFileInputStream::Open( const char* Source ) 
{
    if( m_pFile ) return INPUTSTREAM_ERROR;
    m_pFile = new CFatFile;

    return m_pFile->Open( Source, CFatFile::ReadOnly ) ? INPUTSTREAM_NO_ERROR : INPUTSTREAM_ERROR;
}

ERESULT CFatFileInputStream::Close() 
{
    if( m_pFile == NULL ) return INPUTSTREAM_ERROR;
    bool bRetVal = m_pFile->Close();
    delete m_pFile;
    m_pFile = NULL;
    return bRetVal ? INPUTSTREAM_NO_ERROR : INPUTSTREAM_ERROR;
}

int CFatFileInputStream::Read( void* Buffer, int Count ) 
{
    // restrict access to unsafe region
    if (m_pFile->GetOffset() + Count > m_pFile->GetSafeLen())
        Count = m_pFile->GetSafeLen() - m_pFile->GetOffset();
    if (Count < 0)
        Count = 0;

    if( m_pFile == NULL ) return 0;
    return m_pFile->Read( Buffer, Count );
}

int CFatFileInputStream::Ioctl( int Key, void* Value )
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->Ioctl( Key, Value );
}

int CFatFileInputStream::Seek( InputSeekPos SeekOrigin, int Offset ) 
{
    // restrict access to unsafe region
    int nAbsOff = AbsFromRelOffset(SeekOrigin,Offset);
    if (nAbsOff > m_pFile->GetSafeLen())
    {
        SeekOrigin = SeekStart;
        Offset = m_pFile->GetSafeLen();
    }
    if( m_pFile == NULL ) return 0;
    return m_pFile->Seek( (CFatFile::FileSeekPos)SeekOrigin, Offset );
}

int CFatFileInputStream::Length() const
{
    if( m_pFile == NULL ) return 0;
    // report the safe length if available
    if (m_pFile->GetSafeLen())
        return m_pFile->GetSafeLen();
    return m_pFile->Length();
}

int CFatFileInputStream::Position() const 
{
    if( m_pFile == NULL ) return 0;
    return m_pFile->GetOffset();
}

// compute a variable origin offset into an offset from BOF
int CFatFileInputStream::AbsFromRelOffset( InputSeekPos SeekOrigin, int Offset)
{
    switch (SeekOrigin)
    {
        case SeekStart:
            return Offset;
        case SeekCurrent:
            return m_pFile->GetOffset() + Offset;
        case SeekEnd:
            return m_pFile->Length() + Offset;
    }
    return 0;
}
