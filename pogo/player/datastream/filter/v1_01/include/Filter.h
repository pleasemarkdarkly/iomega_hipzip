//
// Filter.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! Dadio(tm) uses an asbtract interface for filters, which allow components
//! to examine and modify the current PCM audio stream. This is the IFilter
//! interface, as defined in ./player/datastream/filter/include/Filter.h. The
//! purpose of this document is to define the IFilter interface and each of its
//! member functions, and to describe their expected behavior in the Dadio(tm) OS.

/** \addtogroup FilterInterface Filter Interface */
//@{
#ifndef __FILTER_H__
#define __FILTER_H__

#include <util/ident/IdentifiableObject.h>
#include <util/registry/Registry.h>
#include <util/eresult/eresult.h>

//
//! Define the filter error zone for eresult error codes
//
#define FILTER_TYPE_ID      0x0b

#define MAKE_FILTERRESULT(x,y) MAKE_ERESULT(x, FILTER_TYPE_ID, y)

//! No error was encountered during the operation. \hideinitializer
const ERESULT FILTER_NO_ERROR  = MAKE_FILTERRESULT( SEVERITY_SUCCESS, 0x0000 );
//! No work can be processed; that is, either the output buffer is full, or the input
//! buffer is empty. \hideinitializer
const ERESULT FILTER_NO_WORK   = MAKE_FILTERRESULT( SEVERITY_FAILED , 0x0001 );
//! An EOF was received from the input buffer. \hideinitializer
const ERESULT FILTER_EOF       = MAKE_FILTERRESULT( SEVERITY_FAILED , 0x0002 );
//! A general error was encountered. \hideinitializer
const ERESULT FILTER_FAIL      = MAKE_FILTERRESULT( SEVERITY_FAILED , 0x0003 );
//! The filter is read-only. This should be returned from the Configure call. \hideinitializer
const ERESULT FILTER_READONLY  = MAKE_FILTERRESULT( SEVERITY_SUCCESS, 0x0004 );

typedef struct rbuf_reader_s rbuf_reader_t;
typedef struct rbuf_writer_s rbuf_writer_t;


//! A structure to define the properties of a stream being
//! passed through a filter.
typedef struct filter_stream_info_s 
{
    
    bool m_bIsPcm;                             //! State of the union
    char m_szStreamName[128];                  //! A name (URL) for this stream
    
    //! The filter can pump binary data or PCM
    union 
    {
        //! This structure holds information for PCM streams
        struct
        {
            unsigned long Channels;            //! Number of channels
            unsigned long SamplingFrequency;   //! Sample frequency
            unsigned long Bitrate;             //! Bitrate
            unsigned long Duration;            //! Duration
        } pcm;
        //! This structure holds information for binary streams
        //struct
        //{
        //} data;
    } un;
} filter_stream_info_t;


//! Dadio(tm) uses an asbtract interface for filters, which allow components
//! to examine and modify the current PCM audio stream. This is the IFilter
//! interface, as defined in ./player/datastream/filter/include/Filter.h. The
//! purpose of this document is to define the IFilter interface and each of its
//! member functions, and to describe their expected behavior in the Dadio(tm) OS.
struct IFilter : public IIdentifiableObject
{
public:
    virtual ~IFilter() {}

    //! Process a chunk of audio data. The size of a chunk depends
    //! on the input space, output space, and the amount of data the
    //! filter can process. This routine is called until the filter
    //! indicates that no work is available
    virtual ERESULT DoWork(bool bFlush = false) = 0;

    //! A generic Ioctl interface to allow custom configuration.
    virtual ERESULT Ioctl( int Key, void* Value ) = 0;

    //! Configure the filter for the given stream. If the filter
    //! modifies the structure of the stream at all, it needs to
    //! update the filter_stream_info_t structure.
    virtual ERESULT Configure( filter_stream_info_t& StreamInfo ) = 0;
    
    //! Give the media player an idea of the minimum input unit
    //! for this filter. For example, a filter that requires
    //! 128 samples per channel of 16 bit stereo PCM to operate would
    //! return (16bits / 8) * 128 * 2 = 512
    virtual int GetInputUnitSize() const = 0;

    //! Give the media player an idea of the minimum output unit
    //! for this filter. For the most part, this will either be
    //! equal to the input unit size, unless the filter changes
    //! the amount of data being pushed through.
    virtual int GetOutputUnitSize() const = 0;

    //! Called by the media player to assign a write handle for
    //! this filter. When the DoWork() call is issued, the filter
    //! will be expected to write data it processes to this handle.
    virtual int SetWriteBuf( rbuf_writer_t* WriteBuf ) = 0;

    //! Called by the media player to assign a read handle for
    //! this filter. When the DoWork() call is issued, the filter
    //! will be expected to read data it processes from this handle.
    virtual int SetReadBuf( rbuf_reader_t* ReadBuf ) = 0;

    //! Called by the media player to find out what write handle we
    //! are using. This is typically called at the end of a song,
    //! when the media player wants to clean up the playstream.
    virtual rbuf_writer_t* GetWriteBuf() const = 0;
    
    //! Called by the media player to find out what read handle we
    //! are using. This is typically called at the end of a song,
    //! when the media player wants to clean up the playstream.
    virtual rbuf_reader_t* GetReadBuf() const = 0;

    // If you elect not to use the registration interface, you will still
    // need to use the IDENTIFY_OBJECT() macro in your class definition
};

//@}


//
//! Registration interface
//

/** \addtogroup FilterRegistration Filter Registration */
//@{

//! Fill out the filter class definition with additional members
//! necessary for filters to work in Dadio(tm). Additionally, this
//! satisfies the underlying requirements of the IIdentifiableObject
//! baseclass. This macro should be used in your definition (header).
//! The usage would be like:
//!\code
//!  DEFINE_FILTER( "iObjects sample rate converter", 685 );
//!\endcode
//!\code
//!  DEFINE_FILTER( "iObjects disk writer", 691 );
//!\endcode
//!
//!\arg filtername      A string name for the filter
//!\arg filterID        A unique identifier for the filter
//!\hideinitializer
#define DEFINE_FILTER( filtername, filterID )                           \
        static IFilter* Create();                                       \
        static const char* GetFilterName() { return filtername; }       \
        IDENTIFY_OBJECT( FILTER_TYPE_ID, filterID );

//! Create a static initializer to register your filter with the
//! system registry.
//! This macro should be used in your implementation (source).
//!\arg classname       The name of your class (i.e. CSRCFilter)
//!\arg filterID        The unique identifier specified in DEFINE_FILTER
//!\hideinitializer
#define REGISTER_FILTER( classname,filterID )                                   \
        IFilter* classname::Create() { return (IFilter*) new classname; }       \
        static const CRegisterObject class_reg(                                 \
                ASSEMBLE_ID( FILTER_TYPE_ID, filterID ),                        \
                (void*) classname::Create );

//@}

#endif // __FILTER_H__
