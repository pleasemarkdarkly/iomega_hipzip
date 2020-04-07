// Net.h: support routines for network stuff
// (c) Fullplay Media 2002

#ifndef __IO_NET_H__
#define __IO_NET_H__

#ifdef __cplusplus
#define __ExternC extern "C"
#else
#define __ExternC
#endif

enum ConnectionMode
{
    DHCP_ONLY = 0,
    STATIC_ONLY,
    STATIC_FALLBACK,
};

// Load last known state from the registry, set up internal state
__ExternC void InitializeNetwork(void);

// Start up the specified interface
__ExternC bool InitializeInterface(const char* Interface);

// Configure the given interface to use the listed parameters
__ExternC void ConfigureInterface(const char* Interface, unsigned int uiAddress, unsigned int uiGatewayAddr, unsigned int uiSubnetMask, unsigned int uiDNSAddr, ConnectionMode eMode );

// Get configuration info for an interface
__ExternC bool GetInterfaceConfiguration(const char* Interface, unsigned int* uiAddress, unsigned int* uiGatewayAddr, unsigned int* uiSubnetMask, unsigned int* uiDNSAddr, ConnectionMode* eMode );

// Get the current IP address and MAC address for an interface
__ExternC bool GetInterfaceAddresses(const char* Interface, unsigned int* uiCurrentAddress, char* MACAddress );

// Set a callback for settings changes
typedef void (*IntChgCB)(const char* Interface, void* Arg);
__ExternC void SetInterfaceChangeCallback( IntChgCB cb, void* Arg );

// Return the link status for the given interface
__ExternC bool CheckInterfaceLinkStatus(const char* Interface);

// Return whether the interface is configured or not
__ExternC bool CheckInterfaceConfigurationStatus(const char* Interface);

// Issue an ICMP ping to determine if the remote host is routeable
__ExternC bool CheckRemoteHost(unsigned int uiRemoteHostAddr);

// Dont pollute namespace
#undef __ExternC

#endif // __IO_NET_H__
