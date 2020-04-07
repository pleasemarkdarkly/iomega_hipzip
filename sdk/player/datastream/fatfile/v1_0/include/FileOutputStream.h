//
// FileOutputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __FATFILEOUTPUTSTREAM_H__
#define __FATFILEOUTPUTSTREAM_H__

#include <datastream/output/OutputStream.h>

#define FATFILE_OUTPUT_ID  0x43

#define FATFILE_OUTPUT_IOCTL_TRUNCATE 0x01

// fdecl
class CFatFile;

//! The CFatFileOutputStream provides an abstract
//! output stream wrapper for the underlying CFatFile object
class CFatFileOutputStream : public IOutputStream
{
  public:
    CFatFileOutputStream();
    ~CFatFileOutputStream();
    
    ERESULT Open( const char* Source );
    ERESULT Close();

    int Write( const void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    bool Flush();

    int GetOutputUnitSize() { return 512; }

    bool CanSeek() { return true; }

    int Seek( OutputSeekPos SeekOrigin, int Offset );

    DEFINE_OUTPUTSTREAM( "Fat File Output Stream", FATFILE_OUTPUT_ID );

    // (epg,4/11/2002): MD filetag needs the fatfile!
    CFatFile* m_pFile;
private:
};

//@}

#endif // __FATFILEOUTPUTSTREAM_H__
