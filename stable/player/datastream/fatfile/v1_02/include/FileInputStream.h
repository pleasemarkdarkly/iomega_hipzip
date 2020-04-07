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

#ifndef __FATFILEINPUTSTREAM_H__
#define __FATFILEINPUTSTREAM_H__

#include <datastream/input/InputStream.h>

// TODO: move this to FileInputStreamKeys.h
#define FATFILE_INPUT_ID  0x01

// fdecl
class CFatFile;

//! The CFatFileInputStream provides an abstract input
//! stream wrapper for the underlying CFatFile object.
class CFatFileInputStream : public IInputStream
{
  public:
    DEFINE_INPUTSTREAM( "Fat Input", FATFILE_INPUT_ID );
    
    CFatFileInputStream();
    ~CFatFileInputStream();

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

    CFatFile* m_pFile;
  private:
    int AbsFromRelOffset( InputSeekPos SeekOrigin, int Offset);
};

//@}


#endif // __FATFILEINPUTSTREAM_H__
