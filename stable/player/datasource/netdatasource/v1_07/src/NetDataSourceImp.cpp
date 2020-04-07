//
// NetDataSourceImp.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <cyg/kernel/kapi.h>

#include "NetDataSourceImp.h"

#include <io/net/Net.h>
#include <core/events/SystemEvents.h>
#include <content/common/ContentManager.h>
#include <datastream/netstream/HTTPInputStream.h>
#include <datasource/netdatasource/NetDataSource.h>
#include <codec/codecmanager/CodecManager.h>
#include <util/eventq/EventQueueAPI.h>
#include <util/thread/Mutex.h>

#include <stdlib.h> /* malloc */
#include <stdio.h>  /* sscanf */

static CMutex linkStatusMutex;
static inline void lockmutex() 
{ linkStatusMutex.Lock();   }
static inline void unlockmutex()
{ linkStatusMutex.Unlock(); }

void NetworkBreakPoint()
{
	int blah = 23;
	int blahblah = blah+1;
}

void CNetDataSourceImp::TryStartLink( bool iForceInit ) 
{
	// this is called by a timer and asychronously by other threads - so we have a mutex to keep things
	// sane
	lockmutex();
    
    // if we failed twice to initialize the network then assume we aren't automatically initializing;
    //  check to see if manual initialization happened
    if( m_iNetworkInitFailCount == 2 ) {
        if( !CheckInterfaceConfigurationStatus(m_pName) ) {
            unlockmutex();
            return ;
        }
        else {
            m_bNetworkInitialized = true;
            m_iNetworkInitFailCount = 0;
        }
    }
    
	bool LinkStatus;

	LinkStatus = CheckInterfaceLinkStatus(m_pName);

	if (LinkStatus)
	{

		if( !m_bNetworkInitialized )
		{
            m_bNetworkInitialized = InitializeInterface( m_pName );
            if( !m_bNetworkInitialized ) {
                m_iNetworkInitFailCount++;
            } else {
                m_iNetworkInitFailCount = 0;
            }
		}

        SynchConnectedEvent();
	}
	else
	{
		// Link is down, but we think network is up
		if (m_bNetworkConnected)
		{
			if (m_iNetworkDownCount++ >= 4)
			{
				m_iNetworkDownCount = 0;
				m_bNetworkConnected = false;
                m_bNetworkInitialized = false;  // force the interface to be reinitialized
				NetworkBreakPoint();
				CEventQueue::GetInstance()->PutEvent(EVENT_NETWORK_DOWN,(void*)GetInstanceID());
			}
		}
	}

	/* Can't CheckInterfaceLinkStatus for wireless here, it will return false since the
	   interface gets restarted (everything proceeds normally afterwards, though) */

	unlockmutex();
}

void CNetDataSourceImp::SynchConnectedEvent()
{
    if( m_bNetworkInitialized && !m_bNetworkConnected ) {
        int iID = GetInstanceID();
        if( iID ) {
            CEventQueue::GetInstance()->PutEvent(EVENT_NETWORK_UP, (void*)iID);
            m_bNetworkConnected = true;
        }
    }
}

void CNetDataSourceImp::CheckMediaStatusCB( void* arg ) 
{
    reinterpret_cast<CNetDataSourceImp*>(arg)->TryStartLink();
}

void CNetDataSourceImp::SettingsChanged() 
{
    int iID = GetInstanceID();
    if( !iID ) return ;
    
	lockmutex();

    if( m_bNetworkConnected ) {
        if( m_bNetworkInitialized ) {
            // if we were up, trigger a down event
            CEventQueue::GetInstance()->PutEvent( EVENT_NETWORK_DOWN, (void*) iID );
        } else {
            m_bNetworkInitialized = true;
        }
        CEventQueue::GetInstance()->PutEvent( EVENT_NETWORK_UP, (void*) iID );
    }
    
	unlockmutex();
}
void CNetDataSourceImp::SettingsChangeCB( const char* Interface, void* arg )
{
    // Note: this setup wont support multiple networking interfaces, since the callback arg determines
    //  which interface actually handles the callback
    reinterpret_cast<CNetDataSourceImp*>(arg)->SettingsChanged();
}

CNetDataSourceImp::CNetDataSourceImp( int iInterface, bool iForceInit ) : IDataSource( NET_DATA_SOURCE_CLASS_ID )
{
    // dc- perform some configuration on the tcp layer. this throttles the timeout on a connection opening when the remote
    //     host is not available. No real clean way to implement this unfortunately.
    extern int tcptv_keep_init;
    tcptv_keep_init = 5;
    
    m_iInterface = iInterface;

    m_bNetworkInitialized = false;
	m_bNetworkConnected = false;

    m_pName = (m_iInterface == 1 ? "eth1" : "eth0");
    
	m_iNetworkDownCount = 0;
    m_iNetworkInitFailCount = 0;

    unsigned int Addr, Gateway, Subnet, DNS;
    ConnectionMode eMode;
    
    InitializeNetwork();
    SetInterfaceChangeCallback( CNetDataSourceImp::SettingsChangeCB, (void*)this );

    // If this isn't a registered interface, register it and set it to dhcp only for now
    if( !GetInterfaceConfiguration( m_pName, &Addr, &Gateway, &Subnet, &DNS, &eMode ) ) {
        ConfigureInterface( m_pName, 0, 0, 0, 0, DHCP_ONLY );
    }
    
    TryStartLink( iForceInit );
    
    register_timer( CNetDataSourceImp::CheckMediaStatusCB, (void*) this, TIMER_MILLISECONDS(1000), -1, &m_TimerHandle );
    resume_timer( m_TimerHandle );
}


CNetDataSourceImp::~CNetDataSourceImp() 
{
    suspend_timer( m_TimerHandle );
    unregister_timer( m_TimerHandle );
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CNetDataSourceImp::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    if (iMaxLength > 8)
    {
        strcpy(szRootURLPrefix, "http://");
        return true;
    }
    else
        return false;
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream* CNetDataSourceImp::OpenInputStream(const char* szURL)
{
	if (!IsInitialized()) {
        // somewhat cheap; if we are getting calls to use the interface, then reset the failed to init counter
        m_iNetworkInitFailCount = 0;
		return NULL;
    }
    
    if( strnicmp( szURL, "http", 4 ) == 0 ) {
        CHTTPInputStream* pStream = new CHTTPInputStream;
        if( FAILED(pStream->Open( szURL, GetInstanceID() ) ) ) {
            delete pStream;
            pStream = NULL;
        }
        return pStream;
    } else {
        return NULL;
    }
}

    
//! Asks the source the length of the media serial number, if available.
//! Returns 0 if no serial number is available, or the length in bytes.
int
CNetDataSourceImp::GetSerialNumberLength() const
{
    return 0;
}

//! Get the serial number from the media and copy it into the buffer.
//! Returns the number of bytes copied, or -1 if an error was occurred.
int
CNetDataSourceImp::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    return 0;
}

IMediaContentRecord* CNetDataSourceImp::GenerateEntry( IContentManager* pContentManager, const char* pURL, int iCodecID = 0 ) const
{
    // TODO determine if i can do it this way
    media_record_info_t mediaContent;
    memset((void*)&mediaContent, 0, sizeof(media_record_info_t));
    mediaContent.bVerified = true;

    if( ParseURL( pURL, mediaContent, iCodecID ) == 0 ) {
        return pContentManager->AddMediaRecord( mediaContent );
    }
    return 0;
}

int CNetDataSourceImp::ParseURL( const char* pURL, media_record_info_t& mediaContent, int iCodecID ) const
{
    if( strnicmp( pURL, "http", 4 ) == 0 ) {
        mediaContent.szURL = const_cast<char*>(pURL);
        mediaContent.iDataSourceID = GetInstanceID();

        if (!iCodecID)
        {
            const char* pEnd = pURL + strlen( pURL );
            const char* pExt = pEnd - 1;
            while( *pExt != '.' ) pExt--;

            mediaContent.iCodecID = 0;
            if( (pEnd - ++pExt) <= 3 ) {
                mediaContent.iCodecID = CCodecManager::GetInstance()->FindCodecID( pExt );
            }
            if( !mediaContent.iCodecID ) {
                // Give preferential treatment to mp3 (for shoutcast streams)
                mediaContent.iCodecID = CCodecManager::GetInstance()->FindCodecID( "mp3" );
            }
        }
        else
            mediaContent.iCodecID = iCodecID;
        
        return 0;
    }
    else {
        // other schemes should be populated here
        return -1;
    }
}

bool 
CNetDataSourceImp::IsInitialized() 
{
    // dont call this here (since IsInitialized() gets spammed)
    //  1. TryStartLink() is not thread safe
    //  2. The timer thread takes care of initializing the interface
    //	TryStartLink();
	return (m_bNetworkInitialized && m_bNetworkConnected); 
}




