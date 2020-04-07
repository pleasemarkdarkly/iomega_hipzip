//
// NetStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __NETSTREAM_H__
#define __NETSTREAM_H__

// These are relatively local so key collisions dont matter
#define NETSTREAM_IOCTL_SET_NONBLOCKING  0x01
#define NETSTREAM_IOCTL_SET_RXTIMEOUT    0x02
#define NETSTREAM_IOCTL_SET_TXTIMEOUT    0x03
#define NETSTREAM_IOCTL_SET_RXTWEAK      0x04
// fdecl
class CNetDataSource;

// CNetStream class
//! Dadio(tm) provides a simple class to read and write to network streams.
//! This allows input streams and output streams to be derived from a common
//! base, in addition to allowing applications to abstractly interact with
//! network services.
class CNetStream 
{
public:
    CNetStream();
    ~CNetStream();

    //! Open a connection to the given address/port pair
    //! \param address   The network byte order address
    //! \param port      The host byte order port
    bool Open( unsigned int address, unsigned int port );


    //! Open a connection to the given address/port pair.
    //! address can be a pointer to a string representing a dotted
    //! notation IP, or can be a pointer to a string with a valid
    //! hostname. In the case of the latter, a DNS lookup will be performed
    //! to attempt to locate the host.
    bool Open( const char* address, unsigned int port );

	bool Open (const char* address, unsigned int port, CNetDataSource * pDataSource);


    //! Close the current connection
    bool Close();

    //! Read from the network stream
    int Read( void* Buffer, int Count );

    //! Write to the network stream.
    int Write( const void* Buffer, int Count );

    //! Flush the current stream. This is currently a no-op
    bool Flush();

    //! Issue a generic ioctl to the stream, not currently supported.
    int Ioctl( int Key, void* Value );

    //! Return the current offset (position) within the stream
    int Position() const;
private:
    int m_iPosition;
    int m_Socket;
	CNetDataSource * m_pNDS;
};

//@}

#endif // __NETSTREAM_H__
