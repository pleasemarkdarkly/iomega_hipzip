// Net.cpp: support routines for network stuff
// (c) Fullplay Media 2002

#include <pkgconf/system.h>
#if !defined(CYGPKG_NET)
#error "you must build this against a network enabled kernel"
#endif

#include <pkgconf/net.h>

#include <network.h>
#include <eth_drv.h>
#include <netdev.h>
// assumption: kernel supports bootp and dhcp
#include <bootp.h>

extern "C" {
#include <dhcp.h>
};

#include <io/net/Net.h>
#include <util/registry/Registry.h>
#include <util/debug/debug.h>

DEBUG_MODULE( NET );
DEBUG_USE_MODULE( NET );

// Registry key we use to store our state
// TODO fix this
static const RegKey NetworkSettingsRegKey = REGKEY_CREATE( 0x83, 0x01 );

#define IFACE_NAMELEN 5
#define IFACE_MAX     2
typedef struct interface_settings_s
{
    unsigned int uiAddress;
    unsigned int uiGatewayAddr;
    unsigned int uiSubnetMask;
    unsigned int uiDNSAddr;
    char iface_name[IFACE_NAMELEN];
    ConnectionMode eMode;
} interface_settings_t;

// The settings are saved to the registry, the interface_status (true if up, false if down) is not
typedef struct network_state_s 
{
    interface_settings_t interfaces[IFACE_MAX];
    struct eth_drv_sc* p_softc[IFACE_MAX];
    struct ether_drv_stats ethstats[IFACE_MAX];
    cyg_mutex_t settings_lock;
    // Callback support
    IntChgCB CB;
    void* pArg;
} network_state_t;

static network_state_t net_state;

// Registry state control
static void SaveStateToRegistry(void);
static bool RestoreStateFromRegistry(void);

// Internal support
static struct eth_drv_sc* GetNetworkController(const char* Interface);
static inline int GetInterfaceIndex(const char* Interface);

static void SaveStateToRegistry(void) 
{
    void* buf = CRegistry::GetInstance()->FindByKey( NetworkSettingsRegKey );
    if( !buf ) {
        CRegistry::GetInstance()->AddItem( NetworkSettingsRegKey, (void*)&(net_state.interfaces[0]),
            REGFLAG_PERSISTENT, sizeof( interface_settings_t ) * IFACE_MAX );
    } else {
        // 99.9% of the time this will be a memcpy of a buffer onto itself
        memcpy( buf, (void*)&(net_state.interfaces[0]), sizeof(interface_settings_t) * IFACE_MAX );
    }
}

static bool RestoreStateFromRegistry(void) 
{
    void* buf = CRegistry::GetInstance()->FindByKey( NetworkSettingsRegKey );
    if( !buf ) return false;
    
    memcpy( (void*)&(net_state.interfaces[0]), buf, sizeof(interface_settings_t)* IFACE_MAX );
    return true;
}

static struct eth_drv_sc* GetNetworkController(const char* Interface)
{
    for( cyg_netdevtab_entry_t* p = &__NETDEVTAB__[0]; p < &__NETDEVTAB_END__; p++ ) {
        struct eth_drv_sc* ret = (struct eth_drv_sc*) p->device_instance;
        if( strcmp( ret->dev_name, Interface ) == 0 ) {
            return ret;
        }
    }
    return NULL;
}
// Assumes interface is the form "prt#"
static inline int GetInterfaceIndex(const char* Interface)
{
    return (Interface[3] - '0');
}

// External routines
void InitializeNetwork(void) 
{
    // Initialize and restore our state; if we can't restore
    //  then make a new home for ourselves in the registry
    memset( &net_state, 0, sizeof(network_state_t) );
    if( !RestoreStateFromRegistry() ) {
        SaveStateToRegistry();
    }

    // Set our device softc pointers
    for( int i = 0; i < IFACE_MAX; i++ ) {
        if( net_state.interfaces[i].iface_name[0] != 0 ) {
            net_state.p_softc[i] = GetNetworkController( net_state.interfaces[i].iface_name );
        }
    }

    // Make underlying ecos init calls
    init_all_network_interfaces();

    net_state.CB = 0;
    
    cyg_mutex_init( &net_state.settings_lock );
}

bool InitializeInterface(const char* Interface)
{
    struct bootp bp;
    int idx = GetInterfaceIndex( Interface );
    interface_settings_t* iface = &( net_state.interfaces[idx] );

    if( iface->iface_name[idx] == 0 ) {
        DEBUG( NET, DBGLEV_ERROR, "Initialize called on interface %s, which is not configured\n", Interface );
        return false;
    }

    cyg_mutex_lock( &net_state.settings_lock );

    cyg_bool_t* up;
    cyg_uint8* dhcpstate;
    struct dhcp_lease* lease;
    struct bootp* pbp;
    if( idx ) {
#ifdef CYGHWR_NET_DRIVER_ETH1
        up = &eth1_up;
        dhcpstate = &eth1_dhcpstate;
        lease = &eth1_lease;
        pbp = &eth1_bootp_data;
#else
        DEBUG( NET, DBGLEV_ERROR, "Interface %s not available\n", Interface );
        return false;
#endif
    } else {
        up = &eth0_up;
        dhcpstate = &eth0_dhcpstate;
        lease = &eth0_lease;
        pbp = &eth0_bootp_data;
    }

    if( *up ) {
        // if we had a static IP before hand this should still be safe
        do_dhcp_release( Interface, pbp, dhcpstate, lease );
        // also notify the outside world that stuff changed
        if( net_state.CB ) {
            (net_state.CB)( Interface, net_state.pArg );
        }
    }

    if( iface->eMode == DHCP_ONLY || iface->eMode == STATIC_FALLBACK ) {
        // raise the interface and attempt DHCP
        //  one of few spots where we rely on knowing how many interfaces we support
        //  this will automatically post to the needs_attention semaphore and wake the
        //  management thread up
        cyg_thread_resume( dhcp_mgt_thread_h );

        *dhcpstate = DHCPSTATE_INIT;   // prime the pump
        *up = false;
        route_reinit();
        
        if( idx == 0 ) {
            init_eth0();
#ifdef CYGHWR_NET_DRIVER_ETH1
        } else {
            init_eth1();
#endif
        }
    }
    
    if( (iface->eMode == STATIC_FALLBACK && *up == 0) || iface->eMode == STATIC_ONLY ) {
        // We are doing a static ip, so issue a dhcp release, etc

        route_reinit();
        
        memset( &bp, 0, sizeof( struct bootp ) );

        bp.bp_op = BOOTREPLY;
        bp.bp_xid = 0x555;
        bp.bp_htype = HTYPE_ETHERNET;
        bp.bp_hlen = 6;
        //        bp.bp_ciaddr.s_addr = iface->uiAddress;     // client addr
        bp.bp_yiaddr.s_addr = iface->uiAddress;     // your addr
        bp.bp_siaddr.s_addr = iface->uiAddress;     // server addr
        //        bp.bp_giaddr.s_addr = iface->uiGatewayAddr; // gateway

        // set up the vendor specific area
        unsigned char* vp = &(bp.bp_vend[0]);
        unsigned char cookie[] = VM_RFC1048;
        
        memcpy( vp, cookie, sizeof( cookie ) );
        vp += sizeof( cookie );
        
        *vp++ = TAG_SUBNET_MASK;
        *vp++ = 4;
        memcpy( vp, &( iface->uiSubnetMask ), 4 );
        vp += 4;

        if( iface->uiGatewayAddr ) {
            *vp++ = TAG_GATEWAY;
            *vp++ = 4;
            memcpy( vp, &( iface->uiGatewayAddr ), 4 );
            vp += 4;
        }

        if( iface->uiDNSAddr ) {
            *vp++ = TAG_DOMAIN_SERVER;
            *vp++ = 4;
            memcpy( vp, &( iface->uiDNSAddr ), 4 );
            vp += 4;
        }

        *vp = TAG_END;
        
        if( !init_net( iface->iface_name, &bp ) ) {
            DEBUG( NET, DBGLEV_ERROR, "Static initialization failed for device %s\n", iface->iface_name );
            *up = false;
        } else {
            DEBUG( NET, DBGLEV_INFO, "Device %s static initialized\n", iface->iface_name );
            *up = true;
            *dhcpstate = 0;  // indicate external initialization
            show_bootp( iface->iface_name, &bp );
        }
    }
    cyg_mutex_unlock( &net_state.settings_lock );
    return *up;
}

// Assumption: after calling configure, people will want to call initialize again
void ConfigureInterface(const char* Interface, unsigned int uiAddress,
    unsigned int uiGatewayAddr, unsigned int uiSubnetMask, unsigned int uiDNSAddr, ConnectionMode eMode)
{
    int idx = GetInterfaceIndex( Interface );
    interface_settings_t* iface;

    cyg_mutex_lock( &net_state.settings_lock );

    iface = &( net_state.interfaces[idx] );
    // make sure this isn't a new index
    if( iface->iface_name[0] == 0 ) {
        net_state.p_softc[idx] = GetNetworkController( Interface );
        DBASSERT( NET, net_state.p_softc[idx] != NULL, "Associating with a non-existant interface\n");
        
        strcpy( iface->iface_name, Interface );
    }
    
    iface->uiAddress = uiAddress;
    iface->uiGatewayAddr = uiGatewayAddr;
    iface->uiSubnetMask = uiSubnetMask;
    iface->uiDNSAddr = uiDNSAddr;
    iface->eMode = eMode;

    SaveStateToRegistry();
    cyg_mutex_unlock( &net_state.settings_lock );
}

bool GetInterfaceConfiguration(const char* Interface, unsigned int* uiAddress,
    unsigned int* uiGatewayAddr, unsigned int* uiSubnetMask, unsigned int* uiDNSAddr, ConnectionMode* eMode )
{
    int idx = GetInterfaceIndex( Interface );

    if( idx < 0 ) return false;

    interface_settings_t* iface = &( net_state.interfaces[idx] );

    if( iface->iface_name[0] == 0 ) return false;
    
    if( uiAddress )
        *uiAddress = iface->uiAddress;
    if( uiGatewayAddr )
        *uiGatewayAddr = iface->uiGatewayAddr;
    if( uiSubnetMask )
        *uiSubnetMask = iface->uiSubnetMask;
    if( uiDNSAddr )
        *uiDNSAddr = iface->uiDNSAddr;
    if( eMode )
        *eMode = iface->eMode;
    
    return true;
}

bool GetInterfaceAddresses(const char* Interface, unsigned int* uiCurrentAddress, char* MACAddress )
{
    int idx = GetInterfaceIndex( Interface );
    if( idx < 0 ) return false;

    interface_settings_t* iface = &( net_state.interfaces[idx] );
    if( iface->iface_name[0] == 0 ) return false;

    cyg_uint8* dhcpstate;
    struct bootp* bp;
    if( idx ) {
#ifdef CYGHWR_NET_DRIVER_ETH1
        dhcpstate = &eth1_dhcpstate;
        bp = &eth1_bootp_data;
#else
        return false;
#endif
    } else {
        dhcpstate = &eth0_dhcpstate;
        bp = &eth0_bootp_data;
    }
    
    if( uiCurrentAddress ) {
        // Ask the dhcp state machine to see what the hell is going on
        if( *dhcpstate == DHCPSTATE_BOUND ) {
            // we have a dhcp address
            memcpy( uiCurrentAddress, &( bp->bp_yiaddr ), 4 );
        } else {
            // we have a static ip address
            *uiCurrentAddress = iface->uiAddress;
        }
    }
    if( MACAddress && net_state.p_softc[idx] ) {
        memcpy( MACAddress, &( net_state.p_softc[idx]->sc_arpcom.ac_enaddr[0] ), 6 );
    }
    return true;
}

void SetInterfaceChangeCallback( IntChgCB cb, void* Arg )
{
    net_state.CB = cb;
    net_state.pArg = Arg;
}

// not thread safe
bool CheckInterfaceLinkStatus(const char* Interface)
{
    int idx = GetInterfaceIndex( Interface );
    if( net_state.p_softc[idx] ) {
        net_state.p_softc[idx]->funs->control( net_state.p_softc[idx], ETH_DRV_GET_IF_STATS,
            &( net_state.ethstats[idx] ), sizeof( struct ether_drv_stats ) );
    }
    return (bool)(net_state.ethstats[idx].operational == 3);
}

// not thread safe
bool CheckInterfaceConfigurationStatus(const char* Interface)
{
    int idx = GetInterfaceIndex( Interface );
    if (idx) {
#ifdef CYGHWR_NET_DRIVER_ETH1        
        return eth1_up;
#else
        return false;
#endif
    }
    else {
        return eth0_up;
    }
}

// from ecos 1.x net/tcpip/current/tests/ping_test.c
static int
inet_cksum(u_short *addr, int len)
{
    register int nleft = len;
    register u_short *w = addr;
    register u_short answer;
    register u_int sum = 0;
    u_short odd_byte = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while( nleft > 1 )  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( nleft == 1 ) {
        *(u_char *)(&odd_byte) = *(u_char *)w;
        sum += odd_byte;
    }

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    sum = (sum >> 16) + (sum & 0x0000ffff); /* add hi 16 to low 16 */
    sum += (sum >> 16);                     /* add carry */
    answer = ~sum;                          /* truncate to 16 bits */
    return (answer);
}
static bool
ping_host(int s, struct sockaddr_in *host)
{
    char pkt1[256], pkt2[256];
    struct icmp *icmp = (struct icmp *)pkt1;
    int icmp_len = 64;
    int seq, ok_recv, bogus_recv;
    cyg_tick_count_t *tp;
    long *dp;
    struct sockaddr_in from;
    int i, len, fromlen;
    
    ok_recv = 0;
    bogus_recv = 0;

    // you actually do have to loop here, since the ecos icmp implementation is somehow buggy and appears
    //  to drop the first reply or two
    for (seq = 0;  seq < 5;  seq++) {
        // Build ICMP packet
        icmp->icmp_type = ICMP_ECHO;
        icmp->icmp_code = 0;
        icmp->icmp_cksum = 0;
        icmp->icmp_seq = seq;
        icmp->icmp_id = 0x1234;
        // Set up ping data
        tp = (cyg_tick_count_t *)&icmp->icmp_data;
        *tp++ = cyg_current_time();
        dp = (long *)tp;
        for (i = sizeof(*tp);  i < icmp_len;  i += sizeof(*dp)) {
            *dp++ = i;
        }
        // Add checksum
        icmp->icmp_cksum = inet_cksum( (u_short *)icmp, icmp_len+8);
        // Send it off
        if (sendto(s, icmp, icmp_len+8, 0, (struct sockaddr *)host, sizeof(*host)) < 0) {
            perror("sendto");
            return false;
        }
        // Wait for a response
        fromlen = sizeof(from);
        len = recvfrom(s, pkt2, sizeof(pkt2), 0, (struct sockaddr *)&from, (socklen_t*)&fromlen);
        if (len < 0) {
            perror("recvfrom");
        } else {
            if( from.sin_addr.s_addr == host->sin_addr.s_addr ) {
                ok_recv++;
            } else {
                bogus_recv++;
            }
        }
    }
    return (ok_recv ? true : false);
}

bool CheckRemoteHost(unsigned int uiRemoteHostAddr)
{
    struct protoent* proto = getprotobyname("icmp");
    if( !proto ) return false;

    struct timeval tv;
    struct sockaddr_in host;
    int s;

    s = socket(AF_INET, SOCK_RAW, proto->p_proto);
    if( !s ) {
        DEBUG( NET, DBGLEV_ERROR, "Net: out of sockets\n");
        return false;
    }
    
    tv.tv_sec = 0; tv.tv_usec = 200*1000; // 200ms pings
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    host.sin_family = AF_INET;
    host.sin_addr.s_addr = uiRemoteHostAddr;
    host.sin_port = 0;

    bool res = ping_host( s, &host );
    close(s);
    return res;
}
