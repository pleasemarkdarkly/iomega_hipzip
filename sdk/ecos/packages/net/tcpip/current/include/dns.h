// dns.h: interface to dns routines
// danc@iobjects.com 08/12/01
// (c) Interactive Objects

#ifndef __DNS_H__
#define __DNS_H__

#include <pkgconf/net.h>

#ifdef CYGPKG_NET_DNSCLIENT

// ugh, awful header pollution, but i need in_addr
#include <network.h>

#ifdef __cplusplus
extern "C" {
#endif

    void dns_use_server( const struct in_addr* dns_server );
    int dns_resolve( const char* hostname, struct in_addr* addr );
    void dns_flush_cache( void );

#ifdef __cplusplus
};
#endif

#endif  // CYGPKG_NET_DNSCLIENT

#endif // __DNS_H__
