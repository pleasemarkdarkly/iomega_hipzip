//
// InputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//! InputStreams provide an abstract way for Dadio(tm) to read in data. They
//! are used in various parts of the system, and are required for decoding.
//! Several precompiled input streams are provided for use. Input streams
//! provide an optional registration interface also.

/** \addtogroup InputStreams Input Streams */
//@{

#ifndef __INPUTSTREAM_H__
#define __INPUTSTREAM_H__

#include <util/eresult/eresult.h>
#include <util/ident/IdentifiableObject.h>
#include <util/registry/Registry.h>

//
//! The error zone for input stream error codes
//

#define INPUT_TYPE_ID   0x0c

#define MAKE_INPUTRESULT(x,y) MAKE_ERESULT(x,INPUT_TYPE_ID,y)

//! No error was encountered during the operation. \hideinitializer
const ERESULT INPUTSTREAM_NO_ERROR = MAKE_INPUTRESULT( SEVERITY_SUCCESS, 0x0000 );
//! A generic error was encountered during the operation. \hideinitializer
const ERESULT INPUTSTREAM_ERROR    = MAKE_INPUTRESULT( SEVERITY_FAILED,  0x0001 );

// IInputStream interface
//! The IInputStream interface defines the basic interface for read only stream
//! access. Additionally, support is provided for seekable streams.
struct IInputStream : public IIdentifiableObject
{
    //! Enumeration for seek origin
    enum InputSeekPos
    {
        SeekStart,
        SeekCurrent,
        SeekEnd,
    };
    
    virtual ~IInputStream() {}

    //! Open the input stream to the given source.
    virtual ERESULT Open( const char* Source ) = 0;

    //! Close the input stream if currently open.
    virtual ERESULT Close() = 0;

    //! Read the specified number of bytes into Buffer.
    virtual int Read( void* Buffer, int Count ) = 0;

    //! A generic interface for controlling the input stream.
    virtual int Ioctl( int Key, void* Value ) = 0;

    //! Called by codecs to determine the minimum unit to read
    //! in from this input stream. Not all codecs have been
    //! updated to use this routine.
    virtual int GetInputUnitSize() = 0;
    
    //! Allows the input stream to indicate whether or not seek is
    //! supported.
    virtual bool CanSeek() const = 0;
    
    //! Attempt to seek on the input stream.
    virtual int Seek( InputSeekPos SeekOrigin, int Offset ) = 0;

    //! Give back our current position within the stream.
    virtual int Position() const = 0;

    //! If available, return the length of the stream.
    virtual int Length() const = 0;
};


//! Registration for input streams is optional. If you elect not to
//! register your input streams, you still must use the IDENTIFY_OBJECT()
//! macro in your class definition (header) in order to satisfy the
//! requirements of the IIdentifiableObject base class.
//! Otherwise, this macro is used in your definition to complete your
//! input stream definition.
//! \arg isname  String name for your input stream.
//! \arg isID    Unique identifier for this input stream.
//!\hideinitializer
#define DEFINE_INPUTSTREAM( isname, isID )                              \
        static IInputStream* Create();                                  \
        IDENTIFY_OBJECT( INPUT_TYPE_ID, isID );                         \
        static const char* GetInputStreamName() { return isname; }

//! Create a static initializer to register your input stream with the
//! system registry.
//! This macro should be used in your implementation (source) file.
//! \arg classname       The name of your input stream class (i.e. CMemInputStream)
//! \arg isID            The unique identifier used in DEFINE_INPUTSTREAM
//!\hideinitializer
#define REGISTER_INPUTSTREAM( classname, isID )                         \
        IInputStream* classname::Create() {                             \
                return (IInputStream*) new classname;                   \
        }                                                               \
        static const CRegisterObject class_reg(                         \
                ASSEMBLE_ID( INPUT_TYPE_ID, isID ),                     \
                (void*) classname::Create );

//@}

#endif // __INPUTSTREAM_H__
