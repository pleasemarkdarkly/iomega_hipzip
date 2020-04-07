//
// FileInputStream.h
//
// Copyright (c) 1998 - 2001 Fullplay Media Systems (TM). All rights reserved
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
#include <main/buffering/BufferTypes.h>
#include <codec/common/Codec.h>

#include <util/datastructures/SimpleList.h>
#include <util/datastructures/SimpleVector.h>


class CBufferingImp;
class CBuffering;
class IMediaContentRecord;
class CBufferingRef;

// TODO: move this to FileInputStreamKeys.h
#define FATFILE_BUFFEREDINPUT_ID  0xF3

#define BUFFERING_REF_COUNT 3
typedef SimpleVector<CBufferingRef*> BufferingRefVector;

//! The CBuffering provides an abstract input
//! stream wrapper for the underlying CFatFile object.
class CBuffering
{
  public:
    ~CBuffering();

    static CBuffering* GetInstance();
    static void Destroy();

    ERESULT Open( int nContentId, const char* Source );
    ERESULT Close(int nContentId);

    int Read( int nContentId, void* Block, int Count );
    int Ioctl( int nContentId, int Key, void* Value );

    // fat is always based on 512 byte sectors
    int GetInputUnitSize(int nContentId) { return 512;  }
    
    bool CanSeek(int nContentId);
    int Seek( int nContentId, eInputSeekPos SeekOrigin, int Offset );

    int Length(int nContentId) ;
    int Position(int nContentId) ;

    IInputStream* CreateInputStream( IMediaContentRecord* mcr );

    // close and detach from sources no longer in the playlist (assumes crnt track inviolate)
    void ResyncWithPlaylist();
    // close and detach from all sources
    void DetachFromPlaylist();

    void NotifyStreamSet(IMediaContentRecord* mcr);
    void NotifyStreamAutoset();

    void NotifyCDRemoved();

    void PauseHDAccess();
    void ResumeHDAccess();

    // relinquish all file resources and shut down buffering.  currently recommended only when performing
    // memory intensive operations just prior to restarting the player (which is more reliable than the Restart fn below).
    // the initial motivation for this is for the software update process, which requires upwards of 2MB free mem.
    // (dvb,10/22/2002): this function appears to be broken.  epg says that DetachFromPlaylist() needs to be
    // called before this function, but that didn't seem to work for me.  Since I don't really need to use this
    // function (DetachFromPlaylist() does what I really need), I'm just dropping in this "fix me" note.
    // TODO: FIX THIS
    void Shutdown();

    // (epg,5/30/2002): re allocate memory for buffering and generally reverse the actions of Shutdown.
    // Caution: this is a fairly large allocation, and we have no great reason to use this fn, so it ISN'T
    // fully tested.
    void Restart();

    void CloseFiles();
    void SetPrebufDefault(int nSeconds);
    int GetPrebufDefault();

    // register a callback to be notified an integer percentage complete during prebuffering.
    void SetPrebufCallback(SetIntFn* pfn);
  private:

    typedef CBufferingRef CThis;
    bool SetSong( IMediaContentRecord* mcr );

    CBuffering();

    void RepairBufferingRefs();
    void* BufferingRefLocFromIndex(int i);

    // track two different streams
    BufferingRefVector m_vecBufferingRefs;

    static CBuffering* m_pInstance;
    bool IsContentBuffered(int nContentId);

    int m_nCurrentContentId;
    int m_nNextContentId;
    CBufferingImp* m_pImp;

    char * m_pBufferingRefPoolSpace;
};

class CBufferingRef : public IInputStream
{
public:
    DEFINE_INPUTSTREAM( "Buf Fat Input", FATFILE_BUFFEREDINPUT_ID );
    CBufferingRef();
    ~CBufferingRef();

    //! Open the input stream to the given source.
    ERESULT Open( const char* Source );

    //! Close the input stream if currently open.
    ERESULT Close();

    //! Read the specified number of bytes into Buffer.
    int Read( void* Buffer, int Count );

    //! A generic interface for controlling the input stream.
    int Ioctl( int Key, void* Value );

    //! Called by codecs to determine the minimum unit to read
    //! in from this input stream. Not all codecs have been
    //! updated to use this routine.
    int GetInputUnitSize();
    
    //! Allows the input stream to indicate whether or not seek is
    //! supported.
    bool CanSeek() const;
    
    //! Attempt to seek on the input stream.
    int Seek( InputSeekPos SeekOrigin, int Offset );

    //! Give back our current position within the stream.
    int Position() const;

    //! If available, return the length of the stream.
    int Length() const;

private:
    int m_nContentId;
    friend class CBuffering;
};

#endif // __BUFFERED_FATFILEINPUTSTREAM_H__
