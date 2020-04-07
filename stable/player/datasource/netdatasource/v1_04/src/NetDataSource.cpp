//
// NetDataSource.cpp
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

#include <datasource/netdatasource/NetDataSource.h>
#include "NetDataSourceImp.h"
#include <pkgconf/system.h>

#if !defined(CYGPKG_NET)
#error "You must build this against a network enabled kernel"
#else
#include <pkgconf/net.h>
#endif

#include <network.h>
#include <eth_drv.h>
#include <netdev.h>

static bool s_bOnce = false;

CNetDataSource* CNetDataSource::Open( int iInterface, bool iForceInit )
{
    /* If there is no network interface up and running yet */
    if( !s_bOnce ) {
        init_all_network_interfaces();
        s_bOnce = true;
    }

    if( iInterface == 0 ) {
#if (CYGHWR_NET_DRIVER_ETH0==1)
        return new CNetDataSource( iInterface, iForceInit );
#endif
    }
    else if( iInterface == 1 ) {
#if (CYGHWR_NET_DRIVER_ETH1==1)
        return new CNetDataSource( iInterface, iForceInit );
#endif
    }
    
    return NULL;
}

CNetDataSource::CNetDataSource( int iInterface, bool iForceInit )
    : IDataSource( NET_DATA_SOURCE_CLASS_ID )
{
    m_pImp = new CNetDataSourceImp(iInterface, iForceInit);
}

CNetDataSource::~CNetDataSource() 
{
    delete m_pImp;
}

void CNetDataSource::SetInstanceID(int iDataSourceID)
{
    m_pImp->SetInstanceID(iDataSourceID);
}

int CNetDataSource::GetInstanceID() const
{
    return m_pImp->GetInstanceID();
}

// Copies the string the data source uses to prefix its URLs into the given string provided.
bool
CNetDataSource::GetRootURLPrefix(char* szRootURLPrefix, int iMaxLength) const
{
    return m_pImp->GetRootURLPrefix(szRootURLPrefix, iMaxLength);
}

//! Asks the source to open this URL for reading.
//! Returns 0 if the URL was unable to be opened, otherwise
//! it returns the proper subclass of IInputStream for this file type.
IInputStream* CNetDataSource::OpenInputStream(const char* szURL)
{
    return m_pImp->OpenInputStream(szURL);
}

IMediaContentRecord* CNetDataSource::GenerateEntry( IContentManager* pContentManager, const char* pURL, int iCodecID ) const
{
    return m_pImp->GenerateEntry(pContentManager, pURL, iCodecID);
}

int
CNetDataSource::GetSerialNumberLength() const
{
    return m_pImp->GetSerialNumberLength();
}

int
CNetDataSource::GetSerialNumber( char* pBuffer, int iBufferLen ) const
{
    return m_pImp->GetSerialNumber( pBuffer, iBufferLen );
}

//! Returns true if the network is initialized, false otherwise.
bool CNetDataSource::IsInitialized()
{
    return m_pImp->IsInitialized();
}
