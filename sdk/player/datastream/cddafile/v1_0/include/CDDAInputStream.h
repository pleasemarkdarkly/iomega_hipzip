//
// CDDAInputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//


//@{


#ifndef CCDDAINPUTSTREAM_H_
#define CCDDAINPUTSTREAM_H_

#include <datastream/input/InputStream.h>

#define CDDA_INPUT_ID  0x04

class CCDDataSource;

// CCDDAInputStream
//! Dadio(tm) provides a simple way to access CD audio tracks through
//! the CCDDAInputStream. The location of tracks is determined by the
//! data read back from the CD table of contents (TOC), and this input
//! stream simply performs raw audio reads from the drive.
class CCDDAInputStream : public IInputStream
{
public:


    CCDDAInputStream();
    ~CCDDAInputStream();

    //! The default open routine - this returns an error, since more information
    //! must be provided about the item being opened
    ERESULT Open( const char* Source );
    
    //! Open a stream using the specified LBA range on the given CD data source.
    //! This replaces the default IInputStream::Open call.
    ERESULT Open(CCDDataSource* pDataSource, int iLBAStart, int iLBALength);
    
    ERESULT Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    // A CDDA sector is 2352 bytes.
    int GetInputUnitSize() { return 2352;  }
    
    bool CanSeek() const   { return true; }
    int Seek( InputSeekPos SeekOrigin, int Offset );

    int Length() const   { return 2352*(m_iLBAEnd - m_iLBAStart);     }
    int Position() const { return 2352*(m_iLBACurrent - m_iLBAStart); }
    
    IDENTIFY_OBJECT( INPUT_TYPE_ID, CDDA_INPUT_ID );
private:

    CCDDataSource	*m_pDataSource;

    int         m_iLBAStart;
    int         m_iLBAEnd;
    int         m_iLBACurrent;
};

//@}

#endif	// CCDDAINPUTSTREAM_H_
