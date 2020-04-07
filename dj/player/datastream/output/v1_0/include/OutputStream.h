//
// OutputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! OutputStreams provide an abstract way for Dadio(tm) components to output data.
//! The most basic example of an output stream is WaveOut, which writes pcm data
//! to the audio driver. A handful of precompiled output streams are provided with
//! Dadio(tm).

/** \addtogroup OutputStreams Output Streams */
//@{

#ifndef __OUTPUTSTREAM_H__
#define __OUTPUTSTREAM_H__

#include <util/ident/IdentifiableObject.h>
#include <util/registry/Registry.h>
#include <util/eresult/eresult.h>

//
//! The error zone for output stream error codes
//
#define OUTPUT_TYPE_ID   0x0d

#define MAKE_OUTPUTRESULT(x,y) MAKE_ERESULT(x,OUTPUT_TYPE_ID,y)

//! No error was encountered during the operation. \hideinitializer
const ERESULT OUTPUTSTREAM_NO_ERROR = MAKE_OUTPUTRESULT( SEVERITY_SUCCESS, 0x0000 );
//! A generic error was encountered during the operation. \hideinitializer
const ERESULT OUTPUTSTREAM_ERROR    = MAKE_OUTPUTRESULT( SEVERITY_FAILED,  0x0001 );

//! Interface for output streams. If you elect to not use the registration interface
//! for output streams, you still must use IDENTIFY_OBJECT in your class definition.
struct IOutputStream
{
    //! Enumeration for seek origin
    enum OutputSeekPos
    {
        SeekStart,
        SeekCurrent,
        SeekEnd,
    };

    virtual ~IOutputStream()  {}

    //! Attempt to open the given location
    virtual ERESULT Open( const char* Source ) = 0;

    //! Close the output stream if currently open
    virtual ERESULT Close() = 0;

    //! Write the given number of bytes from the buffer to the stream
    virtual int Write( const void* Buffer, int Count ) = 0;
    
    //! A generic interface to issue proprietary commands to the output
    //! stream.
    virtual int Ioctl( int Key, void* Value ) = 0;

    //! If possible, flush the output stream, forcing all buffered data
    //! to be written.
    virtual bool Flush() = 0;

    //! Give an indication as to how large of a unit we write out
    virtual int GetOutputUnitSize() = 0;

    //! Indicate whether or not this output stream supports seeking
    virtual bool CanSeek() = 0;
    //! Attempt to seek the output stream.
    virtual int Seek( OutputSeekPos SeekOrigin, int Offset ) = 0;

};


//
// Registration interface
//


//! Fill out the output stream class definition with
//! additional members required for output stream registration
//! and object identification. 
//! This should be used in your definition (header).
//! The usage would be like:
//!\code
//!  DEFINE_OUTPUTSTREAM( "iObjects wave output", 937 )
//!\endcode
//!
//!\arg  outname         A string name for your output stream
//!\arg  outID           A unique identifier for your output stream
//!\hideinitializer
#define DEFINE_OUTPUTSTREAM( outname, outID )                           \
        static IOutputStream* Create();                                 \
        IDENTIFY_OBJECT( OUTPUT_TYPE_ID, outID );                       \
        static const char* GetOutputName() { return outname; }

//! Create a static initializer to register your output stream with the
//! system registry.
//! This should be used in your implementation
//!\arg  classname       The name of your class (i.e. CWaveOutput)
//!\arg  outID           The unique identifer used in your DEFINE_OUTPUTSTREAM call
//!\hideinitializer
#define REGISTER_OUTPUTSTREAM( classname, outID )                                       \
        IOutputStream* classname::Create() { return (IOutputStream*) new classname; }   \
        static const CRegisterObject class_reg(                                         \
                ASSEMBLE_ID( OUTPUT_TYPE_ID, outID ),                                   \
                (void*) classname::Create );


//@}

#endif // __OUTPUTSTREAM_H__
