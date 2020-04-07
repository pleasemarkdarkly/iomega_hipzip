// IPAddressString.cpp: A variant of PegString specialized to edit IP Addresses 
// danb@iobjects.com 04/26/02
// (c) Interactive Objects

#include <main/ui/IPAddressString.h>
#include <main/ui/Strings.hpp>

#include <network.h>
#include <arpa/inet.h>

#include <stdlib.h>

#include <util/debug/debug.h>
DEBUG_MODULE_S( IPADDRESSSTRING, DBGLEV_DEFAULT | DBGLEV_INFO );
DEBUG_USE_MODULE( IPADDRESSSTRING );

CIPAddressString::CIPAddressString(const PegRect &Rect, const TCHAR *Text, WORD wId,
    WORD wStyle, SIGNED iLen) :
    PegString(Rect, Text, wId, wStyle, iLen)
{
    if(!ValidateIPAddressString(Text))
        PegString::DataSet(LS(SID_BLANK_IP_ADDRESS));
}

CIPAddressString::CIPAddressString(SIGNED iLeft, SIGNED iTop, const TCHAR *Text, WORD wId, 
    WORD wStyle, SIGNED iLen) :
    PegString(iLeft, iTop, Text, wId, wStyle, iLen)
{
    if(!ValidateIPAddressString(Text))
        PegString::DataSet(LS(SID_BLANK_IP_ADDRESS));
}

CIPAddressString::CIPAddressString(SIGNED iLeft, SIGNED iTop, SIGNED iWidth, 
    const TCHAR *Text, WORD wId, WORD wStyle, SIGNED iLen) : 
    PegString(iLeft, iTop, iWidth, Text, wId, wStyle, iLen)
{
    if(!ValidateIPAddressString(Text))
        PegString::DataSet(LS(SID_BLANK_IP_ADDRESS));
}

CIPAddressString::~CIPAddressString()
{
}

SIGNED CIPAddressString::Message(const PegMessage &Mesg)
{
    switch(Mesg.wType)
    {
	default:
		break;
    }
    return PegString::Message(Mesg);
}

void CIPAddressString::DataSet(const TCHAR *Text)
{
    DEBUGP( IPADDRESSSTRING, DBGLEV_TRACE, "ipas:DataSet [%w]\n", Text);
    if(ValidateIPAddressString(Text))
        PegString::DataSet(Text);
    DEBUGP( IPADDRESSSTRING, DBGLEV_TRACE, "ipas:DataSet -\n");
}

void CIPAddressString::DataSet(unsigned int uiAddress)
{
    DEBUGP( IPADDRESSSTRING, DBGLEV_TRACE, "ipas:DataSet [0x%04x]\n", uiAddress);
    in_addr addr;
    addr.s_addr = uiAddress;
    TCHAR szAddr[32];
    CharToTchar(szAddr, inet_ntoa(addr));
    DataSet(szAddr);
    m_uiAddress = uiAddress;
}

bool CIPAddressString::ValidateIPAddressString(const TCHAR *Text)
{
    if (!Text)
        return false;

    DEBUGP( IPADDRESSSTRING, DBGLEV_TRACE, "ipas:ValidateIPAddressString [%w]\n", Text);
    in_addr addr;
    char szAddr[32];
    TcharToCharN(szAddr, Text, 31);
    DEBUGP( IPADDRESSSTRING, DBGLEV_TRACE, "ipas:ValidateIPAddressString [%s]\n", szAddr);
    return inet_aton(szAddr, &addr);
}

unsigned int CIPAddressString::GetIPAddress()
{
    in_addr addr;
    char szAddr[32];
    if (inet_aton(TcharToCharN(szAddr, DataGet(), 31), &addr))
        m_uiAddress = addr.s_addr;
    else
        m_uiAddress = 0;

    return m_uiAddress;
}

