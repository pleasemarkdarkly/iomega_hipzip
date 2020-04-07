//
// WaveOut.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

/** \addtogroup WaveOut Wave Output */

//@{

#ifndef __WAVEOUT_H__
#define __WAVEOUT_H__

#include <datastream/output/OutputStream.h>
#include <datastream/waveout/WaveOutKeys.h>

//! PCM is written to the audio driver through the
//! CWaveOutStream class. This class does not perform
//! any operations on the audio data, and assumes that
//! any necessary sample rate conversion has already been
//! applied.
class CWaveOutStream : public IOutputStream
{
  public:
    DEFINE_OUTPUTSTREAM( "iObjects WaveOutStream", WAVEOUT_KEY );
    CWaveOutStream();
    ~CWaveOutStream();

    ERESULT Open( const char* Source );
    ERESULT Close();

    int Write( const void* Buffer, int Count );

    //! CWaveOutStream supports configuration of hardware
    //! sample rate conversion through an ioctl.
    int Ioctl( int Key, void* Value );

    bool Flush();

    int GetOutputUnitSize();

    bool CanSeek() { return false; }
    int Seek( OutputSeekPos SeekOrigin, int Offset ) { return 0; }
private:
    void GetNextBuffer( bool WriteCurrent = true);

	bool open;

    unsigned int m_uiSamplesPerTick;

   
    static short* m_pBufferStart, *m_pBufferEnd;
    static short* m_pBuffer;

	static unsigned int refcount;
};

//@}

#endif // __WAVEOUT_H__
