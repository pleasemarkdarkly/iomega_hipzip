// NetStream.cpp: generic network stream interface
// danc@iobjects.com 08/01/01
// (c) Interactive Objects

#include <pkgconf/system.h>
#if !defined(CYGPKG_NET)
#error "You must build this against a network enabled kernel"
#endif

#include <pkgconf/net.h>

#include <network.h>

#if defined(CYGPKG_NET_DNSCLIENT)
#include <dns.h>
#endif  // CYGPKG_NET_DNSCLIENT

#include <stdio.h>
#include <datastream/netstream/NetStream.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <util/eventq/EventQueueAPI.h>
#include <core/events/SystemEvents.h>
#include <util/debug/debug.h> 
#include <util/timer/Timer.h> 


DEBUG_MODULE_S( NETSTREAM, DBGLEV_DEFAULT );
DEBUG_USE_MODULE( NETSTREAM );

#define NO_SOCKET -1


static cyg_flag_t	g_flagConnect;
static cyg_mutex_t  g_mutexConnect;
static int					g_connectParam_s;
static const struct sockaddr		*g_connectParam_name;
static socklen_t			g_connectParam_namelen;
static int                 g_retVal;
static int g_ConnectStarted = 0;

// g_flagConnect can take these values:
#define FLAG_START_CONNECT			0x01
#define FLAG_CONNECT_WAITING		0x02
#define FLAG_CONNECT_COMPLETE		0x04
#define FLAG_CONNECT_TIMEOUT		0x08
#define CONNECT_THREAD_STACKSIZE   8192*4

static cyg_handle_t threadhConnectThread;
static cyg_thread   threadConnectThread;
static char         tstackConnectThread[CONNECT_THREAD_STACKSIZE];
void connect_thread_entry( cyg_uint32 data );

static timer_handle_t g_timerHandle;
void timer_callback(void* arg);

void connect_thread_entry( cyg_uint32 data ) 
{
	cyg_flag_value_t flagValue;

	while (1)
	{
		// wait for the "start" flag
		flagValue = cyg_flag_wait(&g_flagConnect,FLAG_START_CONNECT,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);
		// indicate that we will be waiting for a response
		cyg_flag_setbits(&g_flagConnect,FLAG_CONNECT_WAITING);
		g_retVal = connect(g_connectParam_s,g_connectParam_name,g_connectParam_namelen);
		cyg_flag_maskbits(&g_flagConnect,0);
		cyg_flag_setbits(&g_flagConnect,FLAG_CONNECT_COMPLETE);
	}
}

int timeout_connect(int s, const struct sockaddr *name, socklen_t namelen)
{
    cyg_mutex_lock(&g_mutexConnect);
	cyg_flag_value_t flagValue;
	if ( (flagValue = cyg_flag_poll(&g_flagConnect,FLAG_CONNECT_WAITING,CYG_FLAG_WAITMODE_OR)) )
	{
        cyg_mutex_unlock(&g_mutexConnect);
		return -1;
	}
	else
	{
		g_connectParam_s = s;
		g_connectParam_name = name;
		g_connectParam_namelen = namelen;
		cyg_flag_setbits(&g_flagConnect,FLAG_START_CONNECT);
		register_timer_persist( timer_callback, 0, TIMER_MILLISECONDS(1000), 1, &g_timerHandle );
		resume_timer( g_timerHandle );
		flagValue = cyg_flag_wait(&g_flagConnect,FLAG_CONNECT_COMPLETE | FLAG_CONNECT_TIMEOUT,CYG_FLAG_WAITMODE_OR + CYG_FLAG_WAITMODE_CLR);
		unregister_timer(g_timerHandle);

		if (flagValue & FLAG_CONNECT_COMPLETE)
		{
			// connect completed before the timeout interval - return whatever connect returned
            cyg_mutex_unlock(&g_mutexConnect);
			return g_retVal;
		}
		else
		{
			// timed out
            cyg_mutex_unlock(&g_mutexConnect);
			return -1;
		}
	}
}

void timer_callback(void* arg)
{
	cyg_flag_setbits(&g_flagConnect,FLAG_CONNECT_TIMEOUT);
}

void start_connect_thread()
{
	if (!g_ConnectStarted)
	{
//		g_ConnectStarted = 1;
		++g_ConnectStarted;
        DBASSERT( NETSTREAM, g_ConnectStarted == 1, "Too many connection threads\n");

		cyg_flag_init(&g_flagConnect);
        cyg_mutex_init(&g_mutexConnect);

		cyg_thread_create( 10, connect_thread_entry, 0, "connect thread",
	                   (void*)tstackConnectThread, CONNECT_THREAD_STACKSIZE, &threadhConnectThread, &threadConnectThread);
		cyg_thread_resume(threadhConnectThread);
	}
}


CNetStream::CNetStream() 
{
    m_Socket = NO_SOCKET;
    m_iPosition = 0;
	m_pNDS = NULL;

//	start_connect_thread();
}

CNetStream::~CNetStream() 
{
    Close();
}

bool CNetStream::Open( unsigned int address, unsigned int port ) 
{
    if( m_Socket != NO_SOCKET ) {
        Close();
    }
    
    struct sockaddr_in sa;
    memset( &sa, 0, sizeof( struct sockaddr_in ) );

    sa.sin_addr.s_addr = address;
    sa.sin_family = AF_INET;
    sa.sin_port = htons((unsigned short)port);

    struct protoent* proto = getprotobyname( "tcp" );
    if( proto == NULL ) {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "getprotobyname() failed\n" );
        return false;
    }

    m_Socket = socket( AF_INET, SOCK_STREAM, proto->p_proto );

    if( m_Socket < 0 ) {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "socket() failed\n" );
        return false;
    }

	//DEBUG(NETSTREAM, DBGLEV_INFO, "**** CNetStream::Open: About to call connect.\n");

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt( m_Socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );

    if( connect( m_Socket, (const sockaddr*) &sa, sizeof( struct sockaddr_in )) ) {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "connect() failed\n" );
        Close();
        return false;
    }

    return true;
}



bool CNetStream::Open( const char* address, unsigned int port ) 
{
    struct in_addr addr;
    int a,b,c,d;
    
    if( sscanf( address, "%d.%d.%d.%d", &a,&b,&c,&d ) == 4 ) {
        addr.s_addr =
            ((a&0xff)<< 0) |
            ((b&0xff)<< 8) |
            ((c&0xff)<<16) |
            ((d&0xff)<<24);
    }
#if defined(CYGPKG_NET_DNSCLIENT)
    else if( dns_resolve( address, &addr ) != 0 ) {
        DEBUG(NETSTREAM, DBGLEV_WARNING, "dns_resolve() unable to resolve %s port %d\n", address, port);
        return false;
    }
#else
    else {
        return false;
    }
#endif
    return Open( addr.s_addr, port );
}

bool CNetStream::Open (const char* address, unsigned int port , CNetDataSource * pDataSource)
{
	m_pNDS = pDataSource;
	return Open(address,port);
}


bool CNetStream::Close() 
{
    if( m_Socket == NO_SOCKET ) return false;

	//DEBUG(NETSTREAM, DBGLEV_INFO, "**** CNetStream::Close: About to close socket.\n");

    if (close( m_Socket ) == -1)
    {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "Unable to close socket\n" );
    }
    m_iPosition = 0;
    m_Socket = NO_SOCKET;
    return true;
}

int CNetStream::Read( void* Buffer, int Count ) 
{
    DEBUGP(NETSTREAM, DBGLEV_TRACE, "CNetStream::Read %p, %d\n", &Buffer, Count);

    if( m_Socket == NO_SOCKET )
    {
        DEBUG(NETSTREAM, DBGLEV_ERROR, "Attempt to read from an unitialized socket\n");
        return -1;
    }

    if (m_pNDS && (!m_pNDS->IsInitialized())) return 0; 

    int bytes_read = 0;
    
    fd_set fset;
    FD_ZERO( &fset );
    FD_SET( m_Socket, &fset );
    struct timeval timeout;
    timeout.tv_sec  = 2;
    timeout.tv_usec = 0;
    
    while( bytes_read < Count ) {
	    int res = select( m_Socket+1, &fset, NULL, NULL, &timeout );
        if( res <= 0 ) {
            // be honest about partial buffers
            m_iPosition += bytes_read;
            return (bytes_read > 0 ? bytes_read : -1);
        }

        res = recv( m_Socket, (char*)Buffer+bytes_read, Count - bytes_read, 0 );
        if( res <= 0 ) {
            m_iPosition += bytes_read;
            return (bytes_read > 0 ? bytes_read : res);
        }

        bytes_read += res;
    }
    
    m_iPosition += bytes_read;
    DEBUGP(NETSTREAM, DBGLEV_TRACE, "CNetStream::Read %d\n", bytes_read);
    return bytes_read;
}

int CNetStream::Write( const void* Buffer, int Count ) 
{
    if( m_Socket == NO_SOCKET ) return -1;

	if (m_pNDS && (!m_pNDS->IsInitialized())) return -1;

	//DEBUG(NETSTREAM, DBGLEV_INFO, "**** CNetStream::Write: About to send\n");

    return send( m_Socket, Buffer, Count, 0 );
}

bool CNetStream::Flush() 
{
    if( m_Socket == NO_SOCKET ) return -1;

    return 1;
}

int CNetStream::Ioctl( int Key, void* Value ) 
{
    switch( Key ) {
        case NETSTREAM_IOCTL_SET_NONBLOCKING:
        {
            return -1;
        }
        case NETSTREAM_IOCTL_SET_RXTIMEOUT:
        {
            struct timeval tv;
            tv.tv_sec = *(int*)Value;
            tv.tv_usec = 0;
            return setsockopt( m_Socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
        }
        case NETSTREAM_IOCTL_SET_RXTWEAK:
        {
            // Tweak socket settings; triple the reserved mbufs for recv and
            //  up the low water mark a bit
            int m = 0;
            socklen_t optlen = sizeof(int);
            if( getsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, &m, &optlen ) == 0 ) {
                m *= 3;
                setsockopt( m_Socket, SOL_SOCKET, SO_RCVBUF, &m, optlen );
            } else return -1;

            // (dvb,10/24/2002): I removed this b/c setting the low water mark at 
            // 100 bytes makes certain size files fail to be completely b/c sometimes the 
            // socket would have < 100 bytes waiting to be read and a select() on 
            // socket will fail if the bytes available is less than the low water mark.
            //if( getsockopt( m_Socket, SOL_SOCKET, SO_RCVLOWAT, &m, &optlen ) == 0 ) {
            //    m = 100;
            //    setsockopt( m_Socket, SOL_SOCKET, SO_RCVLOWAT, &m, optlen );
            //}
            //else return -1;

            return 0;
        }
        case NETSTREAM_IOCTL_SET_TXTIMEOUT:
        {
            // not currently implemented
            return -1;
        }
        default:
        {
            return -1;
        }
    }
}

int CNetStream::Position() const 
{
    if( m_Socket == NO_SOCKET ) return -1;
    return m_iPosition;
}

