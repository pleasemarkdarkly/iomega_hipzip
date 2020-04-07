//
// HTTPInputStream.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{


#ifndef __HTTPINPUTSTREAM_H__
#define __HTTPINPUTSTREAM_H__

#include <datastream/input/InputStream.h>
#include <datasource/netdatasource/NetDataSource.h>

#define HTTP_INPUT_ID 0x4f

class CNetStream;

// CHTTPInputStream
//! Dadio(tm) provides a method for shoutcast streams and http
//! based streams to be played via the CHTTPInputStream. This
//! class uses a CNetStream object to do basic network i/o, and
//! internally implements a mini http client to perform
//! get requests and parse results.
class CHTTPInputStream : public IInputStream
{
public:
    DEFINE_INPUTSTREAM( "HTTP Input", HTTP_INPUT_ID );

    CHTTPInputStream();
    ~CHTTPInputStream();

    ERESULT Open( const char* Source );
	ERESULT CHTTPInputStream::Open( const char* Source, int iDataSourceInstanceID=-1);
    ERESULT Close();

    int Read( void* Buffer, int Count );
    int Ioctl( int Key, void* Value );

    // use some arbitrary size here - realistically
    // we'd like to sync it with the link layer transmission size,
    // but odds are the http header will unsync that
    int GetInputUnitSize() { return 128; }

    bool CanSeek() const   { return false;  }
    int Seek( InputSeekPos SeekOrigin, int Offset ) { return -1; }

    int Length() const     { return m_iLength; }
    int Position() const;
    
private:
    typedef int HTTPResponse;
    static const HTTPResponse HTTP_OK       = 200;
    static const HTTPResponse HTTP_REDIRECT = 302;

    void IssueRequest( const char* szServerName, const char* szMountPoint,
                       bool bTitleStreaming = false,
                       bool bIcyTitleStreaming = false,
                       int iUDPPort = 0);
    HTTPResponse ReadResponse( bool Redirected );

    // Network stream
    CNetStream* m_pDataStream;

    // Metadata interval for icy metadata
    int m_iMetadataInterval;

    // UDP port for metadata
    int m_iMetadataPort;

    // Some lead data
    char m_pLeadData[1024];
    int m_iLeadAmount;
    int m_iLeadPos;

    // HTTP header length
    int m_iHeaderLength;
    
    // Source location we are getting our data from (url)
    char m_pSource[256];
    // Length of the stream (if available)
    int m_iLength;
    // Is this a chunked stream?
    bool m_bChunked;
};

//@}

#endif // __HTTPINPUTSTREAM_H__
