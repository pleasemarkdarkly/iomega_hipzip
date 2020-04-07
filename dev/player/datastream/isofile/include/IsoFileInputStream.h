//
// IsoFileInputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//


//@{

#ifndef __ISOFILEINPUTSTREAM_H__
#define __ISOFILEINPUTSTREAM_H__

#include <datastream/input/InputStream.h>

// TODO: move this to IsoFileInputStreamKeys.h
#define ISOFILE_INPUT_ID  0x02

// CIsoFileInputStream
//! Dadio(tm) provides a precompiled class to access files found
//! on ISO9660 (data CD) filesystems.
class CIsoFileInputStream : public IInputStream
{
public:
    DEFINE_INPUTSTREAM( "Iso Input", ISOFILE_INPUT_ID );
    
    CIsoFileInputStream();
    ~CIsoFileInputStream();

    ERESULT Open( const char* Source );
    ERESULT Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    // TODO: this is wrong
    int GetInputUnitSize() { return 512;  }
    
    bool CanSeek() const   { return true; }
    int Seek( InputSeekPos SeekOrigin, int Offset );

    int Length() const;
    int Position() const;
    
private:
    int     m_fd;
    int     m_iPosition;
    long    m_lLength;
};

//@}

#endif // __ISOFILEINPUTSTREAM_H__
