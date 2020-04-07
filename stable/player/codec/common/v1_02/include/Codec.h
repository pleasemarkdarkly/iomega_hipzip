//
// Codec.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Dadio(tm) relies on an abstract interface for dealing with arbitrary
//! codecs. This interface is the ICodec interface, as defined in
//! ./player/codec/common/include/Codec.h. The purpose of this document is
//! to define the ICodec interface and each of its member functions, and
//! to describe their expected behavior in the Dadio(tm) OS.

/** \addtogroup CodecInterface Codec Interface */
//@{

#ifndef __CODEC_H__
#define __CODEC_H__

#include <util/ident/IdentifiableObject.h>

#include <util/registry/Registry.h>
#include <util/eresult/eresult.h>
#include <new.h>    // placement new

//
// fdecl
//
class IInputStream;
class IMetadata;
class IDataSource;
class IPlaylistEntry;
typedef struct rbuf_writer_s rbuf_writer_t;

//
//! Define the codec error zone for eresult error codes
//
#define CODEC_TYPE_ID   0x0a

#define MAKE_CODECRESULT( x, y ) MAKE_ERESULT( x, CODEC_TYPE_ID, y )

//! No error was encountered during the operation. \hideinitializer
const ERESULT CODEC_NO_ERROR     = MAKE_CODECRESULT( SEVERITY_SUCCESS, 0x0000 );
//! DRM Authentication succeeded for the input stream. \hideinitializer
const ERESULT CODEC_DRM_SUCCESS  = MAKE_CODECRESULT( SEVERITY_SUCCESS, 0x0001 );
//! The codec was asked to decode a frame, but either
//! no input data is available, or no output space is available. \hideinitializer
const ERESULT CODEC_NO_WORK      = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0002 );
//! The input stream is not in the expected format. \hideinitializer
const ERESULT CODEC_BAD_FORMAT   = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0003 );
//! The decoder had an error on the bitstream. \hideinitializer
const ERESULT CODEC_DECODE_ERROR = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0004 );
//! An EOF was received from the input stream. \hideinitializer
const ERESULT CODEC_END_OF_FILE  = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0005 );
//! General purpose failure. \hideinitializer
const ERESULT CODEC_FAIL         = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0006 );
//! DRM Authentication failure. \hideinitializer
const ERESULT CODEC_DRM_FAIL     = MAKE_CODECRESULT( SEVERITY_FAILED,  0x0007 );

//! Used to propagate information about the bitstream out of the
//! codec to the system.
typedef struct stream_info_s
{
    unsigned long Duration;             //!< The duration of the track (in seconds)
    unsigned long SamplingFrequency;    //!< The sampling frequency of the track (in Hz)
    unsigned long Channels;             //!< The number of channels of audio (currently only 1 or
                                        //!< 2 are supported)
    unsigned long OutputChannels;       //!< The number of channels actually output (may differ
                                        //!< from Channels if channel duplication is supported)
    unsigned long Bitrate;              //!< The bitrate of the track (in bits/second)
} stream_info_t;

//! Structure created when a song is successfully set in the media player.
//! This is passed along through the EVENT_STREAM_SET event.
//! The structure and the metadata should be deleted after the event is received (which
//! is the default action if the event is passed to the play manager).
typedef struct set_stream_event_data_s
{
    char*           szURL;                  //!< URL of the entry that was set.
    IMetadata*      pMediaPlayerMetadata;   //!< Metadata retrieved from the codec during SetSong.
    stream_info_t   streamInfo;             //!< Playback parameters of the stream.
} set_stream_event_data_t;

//! Structure created when the media player stops or manually transitions between tracks.
//! Can be cast down to a set_stream_event_data_s. Forward to the play manager to clean up.
typedef struct change_stream_event_data_s : public set_stream_event_data_t
{
    class CPlayStream* pPreviousStream;
} change_stream_event_data_t;
#if 0
//! Structure created when the media player automatically transitions. If cast to a
//! set_stream_event_data_s, it can be handed off to the UI. Forward to the play manager
//! to clean it up.
typedef struct autoset_stream_event_data_s : public set_stream_event_data_t
{
	class CPlayStream* pPreviousStream;
} autoset_stream_event_data_t;
#endif

//
// ICodec interface
//  in addition to these routines, for your ICodec based class to
//  be available to the system, you must use the registration interface
//  macros below
//
//! Dadio(tm) relies on an abstract interface for dealing with arbitrary
//! codecs. This interface is the ICodec interface, as defined in
//! ./player/codec/common/include/Codec.h. The purpose of this document is
//! to define the ICodec interface and each of its member functions, and
//! to describe their expected behavior in the Dadio(tm) OS.
class ICodec : public IIdentifiableObject
{
public:
    virtual ~ICodec() {}

    //! Decode a frame of audio data. Calculate the current time
    //! offset within the stream (in seconds) and store this value in TimePos.
    virtual ERESULT DecodeFrame( unsigned long& TimePos ) = 0;

    //! Initialize the codec to use the given input stream. If the
    //! codec can correctly interpret the stream format, it must
    //! populate the streamInfo structure with the appropriate
    //! information regarding the stream. If metadata is available and
    //! the pMetadata argument is non-null, then pMetadata should be
    //! populated with the available metadata for the stream.
    //! The codec can optionally query the data source for extended
    //! information, as may be needed for DRM content which requires serial
    //! numbers.
    virtual ERESULT SetSong( IDataSource* pDataSource, IInputStream* pInputStream,
                             stream_info_t& streamInfo, IMetadata* pMetadata = 0 ) = 0;


    //! Attempt to seek to the specified time offset within the file
    //! (in seconds). If the input stream does not support seeking, it
    //! is appropriate to return CODEC_FAIL here.
    virtual ERESULT Seek( unsigned long& secSeek ) = 0;

    //! Attempt to fetch metadata for a given track. If pInputStream
    //! is specified, it will attempt to open pInputStream and find
    //! metadata in that stream. Otherwise, it will provide metadata
    //! found for the currently set track.
    virtual ERESULT GetMetadata( IDataSource* pDataSource, IMetadata* pMetadata,
                                 IInputStream* pInputStream ) = 0;
    
    //! Return a handle to the write buffer for this codec.
    virtual rbuf_writer_t* GetWriteBuf() const = 0;

    //! Set the write buffer for this codec. The write buffer is used
    //! to store decoded data. After SetSong() has successfully
    //! completed, the codec will be associated with a write buffer for
    //! it to store decoded data in.
    virtual void SetWriteBuf( rbuf_writer_t* pW ) = 0;

    //! Indicate how much data will be produced by the codec per
    //! DecodeFrame() call. This is used to appropriately shape buffers
    //! after the codec in the data stream.
    virtual int GetOutputUnitSize() const = 0;

    //! Return a pointer to the current input stream.
    virtual IInputStream* GetInputStream() const = 0;

    // If you elect to not use the registration interface, you still
    // must use IDENTIFY_OBJECT() in your class definition

    //! Routine that can optionally be used to print diagnostic
    //! information about the codec after a stream has completed.
    virtual void Stats() = 0;
};

//
// struct CodecFunctionMap
//!  Short table of static functions that a codec must expose.
//!  This is auto-generated by the registration interface.
//

typedef struct CodecFunctionMap_s 
{
    ICodec* (*Create)(void* pAlloc, int iSize);
    const char* (*GetCodecName)();
    const char*const* (*GetExtensionList)();
    bool (*CanProbe)();
} CodecFunctionMap;

//@}


//
//! Registration interface
//

/** \addtogroup CodecRegistration Codec Registration */
//@{


//! Fill out the codec class definition with additional members
//! necessary for codecs to work in Dadio(tm). Additionally, this
//! satisfies the underlying requirements of the ident
//! (IIdentifiableObject) baseclass.
//! This macro should be used you in your class declaration (header).
//! The usage would be like:
//!\code
//!  DEFINE_CODEC( "iObjects MP3 Codec", 783, true, "mp3" );
//!\endcode
//!\code
//!  DEFINE_CODEC( "iObjects PCM Codec", 741, false, "raw", "pcm" );
//!\endcode
//!
//!\arg codecname   A string name for the codec
//!\arg codecID     A unique identifier for the codec
//!\arg can_probe   Either true or false indicating whether the
//!                 system can probe bitstreams against this
//!                 codec. In the case of PCM and similar pass
//!                 through codecs, this should be false
//!\arg extensions...   A variable length list of strings that
//!                     indicate which extensions are supported by
//!                     this codec. This is case insensitive.
//!\hideinitializer
#define DEFINE_CODEC( codecname, codecID, can_probe,extensions... )             \
        static ICodec* Create(void* pAlloc, int iSize);                         \
        IDENTIFY_OBJECT( CODEC_TYPE_ID, codecID );                              \
        static const char* GetCodecName() { return codecname; }                 \
        static const char*const* GetExtensionList() { return m_spExtensions; }  \
        static bool CanProbe() { return can_probe; }                            \
        private:                                                                \
        static const char* const m_spExtensions[] = { extensions, 0 };          \
        public:

//! Create a static initializer to register your codec with the
//! system registry, and provide some initialized data structures
//! to support generic instantiation.
//! This macro should be used in your implementation file (source).
//!\arg classname       The name of your class (i.e. CMP3Codec)
//!\arg codecID         The unique identifier specified in DEFINE_CODEC
//!\hideinitializer
// TODO: if the pool size is not large enough, should we default to heap?
#define REGISTER_CODEC( classname, codecID )                       \
        const char* const classname::m_spExtensions[];             \
        ICodec* classname::Create(void* pAlloc, int iSize) {       \
            if ((pAlloc)) {                                        \
                if( iSize < (signed)sizeof( classname ) )          \
                    return NULL;                                   \
                return (ICodec*) new(pAlloc) classname();          \
            } else {                                               \
                return (ICodec*) new classname();                  \
            }                                                      \
        }                                                          \
        static const CodecFunctionMap class_fmap = {    \
                 classname::Create,                     \
                 classname::GetCodecName,               \
                 classname::GetExtensionList,           \
                 classname::CanProbe,                   \
        };                                              \
        static const CRegisterObject class_reg(         \
                ASSEMBLE_ID( CODEC_TYPE_ID, codecID ),  \
                (void*) &class_fmap );

//@}

#endif // __CODEC_H__
