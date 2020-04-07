//
// FileInputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __BUFFERED_FATFILEINPUTSTREAM_H__
#define __BUFFERED_FATFILEINPUTSTREAM_H__

#include <datastream/input/InputStream.h>

class CBufferedFatFileInputStreamImp;
class IMediaContentRecord;

// TODO: move this to FileInputStreamKeys.h
#define FATFILE_BUFFEREDINPUT_ID  0xF3

//! The CBufferedFatFileInputStream provides an abstract input
//! stream wrapper for the underlying CFatFile object.
class CBufferedFatFileInputStream : public IInputStream
{
  public:
    DEFINE_INPUTSTREAM( "Buf Fat Input", FATFILE_BUFFEREDINPUT_ID );
    
    static CBufferedFatFileInputStream* GetInstance();
	static void Destroy();

    CBufferedFatFileInputStream();
    ~CBufferedFatFileInputStream();

    ERESULT Open( const char* Source );
    ERESULT Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    // fat is always based on 512 byte sectors
    int GetInputUnitSize() { return 512;  }
    
    bool CanSeek() const   { return true; }
    int Seek( InputSeekPos SeekOrigin, int Offset );

    int Length() const;
    int Position() const;

    IInputStream* CreateInputStream( IMediaContentRecord* mcr );
    bool SetSong( IMediaContentRecord* mcr );

  private:
    static CBufferedFatFileInputStream* m_pInstance;
    CBufferedFatFileInputStreamImp* m_pImp;
};

//@}


#endif // __BUFFERED_FATFILEINPUTSTREAM_H__
