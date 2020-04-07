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

#ifndef __CLINEINPUTSTREAM_H__
#define __CLINEINPUTSTREAM_H__

#include <datastream/input/InputStream.h>

#define LINE_INPUT_ID  0x04

class CLineInDataSource;

// CLineInputStream
//! The line input stream provides an inputstream wrapper to the ADC
//! audio input driver. Unlike other input streams, line input must
//! be read in real-time, otherwise input frames will be dropped.
class CLineInputStream : public IInputStream
{
public:

    CLineInputStream();
    ~CLineInputStream();

    ERESULT Open( const char* Source );
    ERESULT Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    // The input unit size depends on the size of an audio buffer
    int GetInputUnitSize();
    
    bool CanSeek() const   { return false; }
    int Seek( InputSeekPos SeekOrigin, int Offset ) { return -1; }

    // TODO fix length. how will everything handle getting 0 here?
    int Length() const   { return 0; }
    int Position() const { return m_iPosition; }
    
    IDENTIFY_OBJECT( INPUT_TYPE_ID, LINE_INPUT_ID );
private:
    void GetNextBuffer( bool bRelease = true );
    int m_iPosition;

    short* m_pCurrentBuffer, *m_pBufferEnd;
};

//@}

#endif	// __CLINEINPUTSTREAM_H__
