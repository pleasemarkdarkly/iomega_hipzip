// HTTPInputStream.cpp: http input stream
// danc@iobjects.com 08/01/01
// (c) Interactive Objects

#include <datastream/netstream/NetStream.h>
#include <datastream/netstream/HTTPInputStream.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <datasource/datasourcemanager/DataSourceManager.h>

#include <util/debug/debug.h>

#include <string.h>   // string ops
#include <stdio.h>    // sprintf
#include <stdlib.h>   // atoi
#include <ctype.h>

DEBUG_USE_MODULE( NETSTREAM );

REGISTER_INPUTSTREAM( CHTTPInputStream, HTTP_INPUT_ID );

#define HTTP_DEFAULT_PORT 80

static const char* HTTP_REQUEST = "User-Agent: Dadio/1.0\r\nAccept: */*\r\n";
static const char* TITLE_STREAMING_HTTP_REQUEST = "User-Agent: Dadio/1.0\r\nAccept: */*\r\nx-audiocast-udpport: ";
static const char* ICY_TITLE_STREAMING_HTTP_REQUEST = "User-Agent: Dadio/1.0\r\nAccept: */*\r\nicy-metadata: 1\r\nx-audiocast-udpport: ";


CHTTPInputStream::CHTTPInputStream() 
{
    m_iLength = m_iHeaderLength = 0;
    m_pDataStream = NULL;
}

CHTTPInputStream::~CHTTPInputStream() 
{
    Close();
}

ERESULT CHTTPInputStream::Open( const char* Source ) 
{
    const char* path;
    char pServerName[64];
    unsigned int port;

    strcpy( m_pSource, Source );

    if ( m_pDataStream ) {
        delete m_pDataStream;
        m_pDataStream = NULL;
    }
    m_pDataStream = new CNetStream();
    
    int redir_count;
    for( redir_count = 0; redir_count < 5; redir_count++ ) {
#if 0
        // sscanf throws an exception (!) on my test cases for some reason
        int count = sscanf( m_pSource, "http://%[^:/]:%d", pServerName, &port );

        if( count < 1 ) {
            // couldn't match
            delete m_pDataStream;
            m_pDataStream = NULL;
            return -1;
        }
        else if( count == 1 ) {
            port = HTTP_DEFAULT_PORT;
        }
        path = strchr( m_pSource+7, '/' );
#else
        if( strncmp( m_pSource, "http://", 7 ) != 0 ) {
            delete m_pDataStream;
            m_pDataStream = NULL;
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Bad URL: %s\n", m_pSource);
            return INPUTSTREAM_ERROR;
        }
        char* col = strchr( m_pSource + 7, ':' );
        path = strchr( m_pSource + 7, '/' );

        if( col ) {
            int len = col - (m_pSource+7);
            memcpy( pServerName, m_pSource+7, col - (m_pSource+7) );
            pServerName[len] = 0;
            sscanf( col + 1, "%d", &port );
        }
        else if( path ) {
            int len = path - (m_pSource+7);
            memcpy( pServerName, m_pSource+7, path - (m_pSource+7) );
            pServerName[len] = 0;
            port = HTTP_DEFAULT_PORT;
        }
#endif

        if( !m_pDataStream->Open( pServerName, port ) )
        {
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Unable to open connection to %s port %d\n", pServerName, port);
            // couldn't open
            delete m_pDataStream;
            m_pDataStream=NULL;
            return INPUTSTREAM_ERROR;
        }

        IssueRequest( pServerName, path );

        HTTPResponse response = ReadResponse( redir_count > 0 );

        if( response == HTTP_OK ) {
            break;
        }
        if( response == HTTP_REDIRECT ) {
            // Get ready to try the next connection
            m_pDataStream->Close();
        }
        else {
            // error
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Error in http response on %s port %d: %d\n", pServerName, port, response);
            delete m_pDataStream;
            m_pDataStream=NULL;
            return INPUTSTREAM_ERROR;
        }
    }
    if( redir_count == 5 ) {
        // redir limit
        DEBUG(NETSTREAM, DBGLEV_ERROR, "Redirect limit reached on URL %s\n", m_pSource);
        delete m_pDataStream;
        m_pDataStream=NULL;
        return INPUTSTREAM_ERROR;
    }
    
    return INPUTSTREAM_NO_ERROR;
}



ERESULT CHTTPInputStream::Open( const char* Source, int iDataSourceInstanceID ) 
{
    const char* path;
    char pServerName[64];
    unsigned int port;

	CNetDataSource *pNDS = (iDataSourceInstanceID >= 0 ? (CNetDataSource*)CDataSourceManager::GetInstance()->GetDataSourceByID(iDataSourceInstanceID) : NULL);

    strcpy( m_pSource, Source );

    if ( m_pDataStream ) {
        delete m_pDataStream;
        m_pDataStream = NULL;
    }
    m_pDataStream = new CNetStream();
    
    int redir_count;
    for( redir_count = 0; redir_count < 5; redir_count++ ) {
#if 0
        // sscanf throws an exception (!) on my test cases for some reason
        int count = sscanf( m_pSource, "http://%[^:/]:%d", pServerName, &port );

        if( count < 1 ) {
            // couldn't match
            delete m_pDataStream;
            m_pDataStream = NULL;
            return -1;
        }
        else if( count == 1 ) {
            port = HTTP_DEFAULT_PORT;
        }
        path = strchr( m_pSource+7, '/' );
#else
        if( strncmp( m_pSource, "http://", 7 ) != 0 ) {
            delete m_pDataStream;
            m_pDataStream = NULL;
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Bad URL: %s\n", m_pSource);
            return INPUTSTREAM_ERROR;
        }

        char* col = strchr( m_pSource + 7, ':' );
        path      = strchr( m_pSource + 7, '/' );

        if( col ) {
            int len = col - (m_pSource+7);
            memcpy( pServerName, m_pSource+7, col - (m_pSource+7) );
            pServerName[len] = 0;
            sscanf( col + 1, "%d", &port );
        }
        else if( path ) {
            int len = path - (m_pSource+7);
            memcpy( pServerName, m_pSource+7, path - (m_pSource+7) );
            pServerName[len] = 0;
            port = HTTP_DEFAULT_PORT;
        }
#endif

        if( !m_pDataStream->Open( pServerName, port,pNDS ) ) {
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Unable to open connection to %s port %d\n", pServerName, port);
            // couldn't open
			delete m_pDataStream;
			m_pDataStream=NULL;

            return INPUTSTREAM_ERROR;
        }

        IssueRequest( pServerName, path );

        HTTPResponse response = ReadResponse( redir_count > 0 );

        if( response == HTTP_OK ) {
            break;
        }
        if( response == HTTP_REDIRECT ) {
            // Get ready to try the next connection
            m_pDataStream->Close();
        }
        else {
            // error
            DEBUG(NETSTREAM, DBGLEV_ERROR, "Error in http response on %s port %d: %d\n", pServerName, port, response);
            delete m_pDataStream;
            m_pDataStream=NULL;
            return INPUTSTREAM_ERROR;
        }
    }
    if( redir_count == 5 ) {
        // redir limit
        DEBUG(NETSTREAM, DBGLEV_ERROR, "Redirect limit reached on URL %s\n", m_pSource);
        delete m_pDataStream;
        m_pDataStream=NULL;
        return INPUTSTREAM_ERROR;
    }
    
    return INPUTSTREAM_NO_ERROR;
}


ERESULT CHTTPInputStream::Close() 
{
    if( m_pDataStream ) {
        delete m_pDataStream;
        m_pDataStream = NULL;
    }
    return INPUTSTREAM_NO_ERROR;
}

int CHTTPInputStream::Read( void* Buffer, int Count ) 
{
    if( m_pDataStream == NULL )
    {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "Attempt to read from an unitialized data stream\n");
        return -1;
    }
    
    int bytes_left = Count;
    char* buf = (char*)Buffer;
    if( m_iLeadAmount ) {
        int bytes_to_copy = bytes_left;
        if( m_iLeadAmount < bytes_left ) {
            bytes_to_copy = m_iLeadAmount;
        }
        
        memcpy( buf, &( m_pLeadData[m_iLeadPos] ), bytes_to_copy );
        bytes_left -= bytes_to_copy;
        buf += bytes_to_copy;
        
        m_iLeadAmount -= bytes_to_copy;

        if (!bytes_left)
            return bytes_to_copy;
    }

    int copied = m_pDataStream->Read( buf, bytes_left );
	if (copied > 0) bytes_left -= copied;
        
    return (copied > 0 ? (Count - bytes_left) : copied);
}

int CHTTPInputStream::Ioctl( int Key, void* Value ) 
{
    return -1;
}

int CHTTPInputStream::Position() const 
{
    if( m_pDataStream == NULL ) return -1;

    // dumb hack - if we overread while checking the http headers,
    //  then we end up with a little bit of lead data on the stream
    //  so just subtract whatever buffered quantitiy we have from the
    //  stream position
    return m_pDataStream->Position() - m_iLeadAmount - m_iHeaderLength;
}


//
// HTTP protocol processing
//

void CHTTPInputStream::IssueRequest( const char* szServerName, const char* szMountPoint,
                                     bool bTitleStreaming, bool bIcyTitleStreaming,
                                     int iUDPPort ) 
{
    const char* RequestHeader =
        bIcyTitleStreaming ? ICY_TITLE_STREAMING_HTTP_REQUEST :
        bTitleStreaming    ? TITLE_STREAMING_HTTP_REQUEST     : HTTP_REQUEST;

    int iRequestLength =
        4                                /* 'GET ' */
        + (szMountPoint ? strlen(szMountPoint) : 0)
        + 17                             /* 'HTTP/1.0\r\nHost: ' */
        + strlen( szServerName )
        + 2                              /* '\r\n' */
        + strlen( RequestHeader )
        + 2;                             /* '\r\n' */

    if( bTitleStreaming || bIcyTitleStreaming ) {
        iRequestLength += 7;             /* '%d\r\n' */
    }

    iRequestLength++;                    /* '\0' */
    char* pRequest = new char[ iRequestLength ];

    if( bTitleStreaming || bIcyTitleStreaming ) {
        sprintf( pRequest,
                 "GET %s HTTP/1.1\r\nHost: %s\r\n%s%d\r\n\r\n",
                 (szMountPoint ? szMountPoint : "/"),
                 szServerName,
                 RequestHeader, iUDPPort );
    }
    else {
        sprintf( pRequest,
                 "GET %s HTTP/1.1\r\nHost: %s\r\n%s\r\n",
                 (szMountPoint ? szMountPoint : "/"),
                 szServerName,
                 RequestHeader );
    }

        
    m_pDataStream->Write( pRequest, strlen( pRequest ) );

    delete [] pRequest;
}


CHTTPInputStream::HTTPResponse CHTTPInputStream::ReadResponse( bool Redirected ) 
{
    HTTPResponse res;
    int length = 1024;
    char* pResponse = new char[ length ];

    int bytes = m_pDataStream->Read( pResponse, length );

    if( bytes < 0 ) {
        DEBUG(NETSTREAM, DBGLEV_WARNING, "Unable to read header\n");
        delete [] pResponse;
        return -1;
    }

    char* start = pResponse;
    char* str;
    
    if( bytes && ( sscanf( pResponse, " %*s %d %*[^\n\r]", &res ) == 1 ) &&
        ( str = strstr( start, "\r\n") ) ) {

        for(;;) {
            if( str == start ) {
                // done parsing headers
                break;
            }
            if( str ) {
                *str = 0;
                str += 2;  // advance to next line
            }
            
            if( strncmp( start, "Content-Length:", 15 ) == 0 ) {
                // Content-length: some servers are nice enough to give us
                // a length indicator on the data they are sending
                char* length_str = start + 15;
                m_iLength = atoi( length_str );
            }
            else if( strncmp( start, "Location:", 9 ) == 0 ) {
                // Location: on a redirect, the server will inform us of the
                // new location for the resource
                char* source_str = start + 9;
                while ( *source_str && isspace(*source_str) ) {
                    ++source_str;
                }
                strcpy( m_pSource, source_str );
            }
            else if( strncmp( start, "icy-metaint:", 12 ) == 0 ) {
                char* meta_str = start + 12;
                m_iMetadataInterval = atoi( meta_str );
            }
            else if( strncmp( start, "x-audiocast-udpport:", 20 ) == 0 ) {
                char* port_str = start + 20;
                m_iMetadataPort = atoi( port_str );
            }
            else if( strncmp( start, "Transfer-Encoding:", 18 ) == 0 ) {
                // Transfer-Encoding: the stream can be sent in chunks
                // instead of a more normal stream
                if( strstr( start, "chunked" ) ) {
                    m_bChunked = true;
                }
            }
            
            // advance to next section
            DBASSERT( NETSTREAM, str, "CRLF not found in headers\n" );
            start = str;
            str = strstr( start, "\r\n" );
            // make sure we didn't overstep any boundaries
            if( start >= (pResponse + bytes) ) {
                DEBUG(NETSTREAM, DBGLEV_WARNING, "Unable to read header\n");
                delete [] pResponse;
                return -1;
            }
        }
        // move past the extra \r\n
        start += 2;
    }
    else {
        // Failed to read HTTP header.  If we've been redirected then this connection might
        // be regular data.  If not, we should bail.
        if ( Redirected ) {
            res = HTTP_OK;
        }
        else {
            delete [] pResponse;
            return -1;
        }
    }

    // determine the total length of the headers
    m_iHeaderLength = (start - pResponse);
    
    // we end up with a little residual data if we overread
    m_iLeadPos = 0;
    m_iLeadAmount = bytes - m_iHeaderLength;
    memcpy( m_pLeadData, start, m_iLeadAmount );
    
    delete [] pResponse;

    // set the netstream to nonblocking mode, set recv timeout to 2 sec
    // only do this for large files or streams of undefined length, since
    // the tcp stack has a bug that will cause it to junk unread mbufs on small files
    // when in nonblocking mode
    if( !Length() || (Length() > 65536) ) {
        int var = 2;
        m_pDataStream->Ioctl( NETSTREAM_IOCTL_SET_NONBLOCKING, (void*) &var );
        m_pDataStream->Ioctl( NETSTREAM_IOCTL_SET_RXTIMEOUT, (void*) &var );
        m_pDataStream->Ioctl( NETSTREAM_IOCTL_SET_RXTWEAK, (void*) &var );
    }

    return res;
}

