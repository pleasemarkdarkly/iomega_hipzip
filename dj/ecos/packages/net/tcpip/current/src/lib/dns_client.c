// dns_client.c: simple dns client
// danc@iobjects.com 08/12/01
// (c) Interactive Objects

#include <network.h>
#include <stdlib.h>   // malloc, free


#define DNS_PORT 53

// config

#define STR_LEN        128   // in bytes


#if (CYGPKG_NET_DNSC_CACHE==1)
// TODO make a mutex of some sort since this code isn't thread safe
typedef struct dns_cache_entry_s 
{
    char hostname[ STR_LEN ];
    int namelen;
    struct in_addr addr;
    unsigned int expiration_time;
} dns_cache_entry_t;

// actual cache table
static dns_cache_entry_t dns_cache[ CYGPKG_NET_DNSC_CACHE_TABLE_SIZE ];

#define _max_( x, y ) (x > y ? x : y)
static int dns_check_cache( const char* hostname, struct in_addr* addr ) 
{
    unsigned int current_time = cyg_current_time() * 100 ; // time(NULL);
    int len = strlen( hostname );
    int i;

    for( i = 0; i < CYGPKG_NET_DNSC_CACHE_TABLE_SIZE; i++ ) {
        if( dns_cache[i].namelen > 0 && dns_cache[i].expiration_time > current_time &&
            memcmp( dns_cache[i].hostname, hostname, _max_( len, dns_cache[i].namelen ) ) == 0 ) {
            memcpy( addr, &( dns_cache[i].addr ), sizeof( struct in_addr ) );
            return 0;
        }
    }
    return 1;
}

static int dns_update_cache( const char* hostname, const struct in_addr* addr, unsigned int ttl ) 
{
    // find a spot in the cache, post the update
    unsigned int current_time = cyg_current_time()*100; //time(NULL);
    int i, oldest = 0;
    int len = strlen( hostname );
    
    for( i = 0; i < CYGPKG_NET_DNSC_CACHE_TABLE_SIZE; i++ ) {
        if( dns_cache[i].expiration_time < current_time ) {
            break;
        }
        else if( dns_cache[i].expiration_time < dns_cache[oldest].expiration_time ) {
            oldest = i;
        }
    }
    
    // if none of the entries are expired, we'll overwrite the oldest one
    if( i == CYGPKG_NET_DNSC_CACHE_TABLE_SIZE ) {
        i = oldest;
    }
    memcpy( &( dns_cache[i].addr ), addr, sizeof( struct in_addr ) );
    memcpy( dns_cache[i].hostname, hostname, len );
    dns_cache[i].namelen = len;
    // TODO resolve units, factor in ttl
    dns_cache[i].expiration_time = cyg_current_time()*100 /*time(NULL)*/ + ttl;
    return 0;
}
#endif

void dns_flush_cache( void ) 
{
#if (CYGPKG_NET_DNSC_CACHE==1)
    memset( (void*) dns_cache, 0, sizeof( dns_cache_entry_t ) * CYGPKG_NET_DNSC_CACHE_TABLE_SIZE );
#endif
}


//
// Standard header for a DNS packet
//
typedef struct dns_header_s 
{
    unsigned short id;
    
    unsigned short flags;

    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
} dns_header_t;

// Values for the opcode field
#define OPCODE_QUERY   0
#define OPCODE_IQUERY  1
#define OPCODE_STATUS  2

// Values for the rcode field
#define RCODE_NOERROR  0
#define RCODE_FORMAT   1
#define RCODE_SERVER   2
#define RCODE_NAME     3
#define RCODE_NOTIMPL  4
#define RCODE_REFUSED  5

// Values for the rquery field
#define RQUERY_RECURSIVE 1

//
// Standard query
//
typedef struct dns_query_s
{
    //    const char* qname;
    unsigned short qtype;
    unsigned short qclass;
} dns_query_t;

//
// Standard response
//
typedef struct dns_response_s
{
    unsigned short rname;
    unsigned short rtype;
    unsigned short class;
    unsigned int   ttl;
    unsigned short rlen;
} __attribute__((packed)) dns_response_t;

// values for TYPE and QTYPE (not a full list)
#define TYPE_A      1    // host address
#define TYPE_NS     2    // authoritative name server
#define TYPE_CNAME  5    // cname for an alias
#define TYPE_SOA    6    // start of a zone authority
#define TYPE_WKS   11    // well known service description
#define TYPE_PTR   12    // domain name pointer
#define TYPE_HINFO 13    // host information
#define TYPE_MINFO 14    // mailbox or mail list info
#define TYPE_MX    15    // mail exchange
#define TYPE_TXT   16    // text strings

// values for CLASS and QCLASS
#define CLASS_IN    1    // the internet
#define CLASS_CH    3    // chaos
#define CLASS_HS    4    // hesiod

static char* copy_hostname( char* dest, const char* src );
static int dns_resolve_internal( const char* hostname, struct in_addr* addr, const struct in_addr* server );

static struct in_addr dns_server = {0};

void dns_use_server( const struct in_addr* dns_srv ) 
{
    memcpy( &dns_server, dns_srv, sizeof( struct in_addr ) );
}


int dns_resolve( const char* hostname, struct in_addr* addr ) 
{
    return dns_resolve_internal( hostname, addr, &dns_server );
}

static int dns_resolve_internal( const char* hostname, struct in_addr* addr, const struct in_addr* server )
{
    struct timeval tv;
    dns_header_t* header;
    dns_query_t* query;
    dns_response_t* response=NULL;
    
    char* pkt, *p;
    int plen, len = strlen( hostname )+1;
    int sz, i;
    unsigned short id;
    struct sockaddr_in csa, ssa;
    int csock, ssock;
    
#ifdef CYGPKG_NET_DNSC_CACHE
    // avoid forcing the user to call flush_cache
    // by automatically flushing once, if needed
    static int needs_flush = 1;
    if( needs_flush ) {
        needs_flush = 0;
        dns_flush_cache();
    }
    
    if( dns_check_cache( hostname, addr ) == 0 ) {
        return 0;
    }
#endif
    
    if( server->s_addr == 0 ) {
        diag_printf(" ** SERVER ADDR NOT SET \n");
        return -1;
    }

    pkt = (char*) malloc( 512 );
    if( pkt == NULL ) {
        diag_printf(" ** MALLOC FAILED\n");
        return -1;
    }
    
    memset( pkt, 0, 512 );

    p = pkt;
    header = (dns_header_t*) p;
    p += sizeof( dns_header_t );

    id = (unsigned short) cyg_current_time()*10; //time( NULL );
    header->id        = id;
    header->flags     = (OPCODE_QUERY<<11) | (RQUERY_RECURSIVE<<0);
    header->qdcount   = htons(1);
    //    header->flags     = htons( header->flags );

    p = copy_hostname( p, hostname );

#if 0
    query = (dns_query_t*) p;
    p += sizeof( dns_query_t );
    
    query->qtype    = TYPE_A;
    query->qclass   = CLASS_IN;
#else
    // handle unaligned structures
    p[0] = p[2] = 0;
    p[1] = TYPE_A;
    p[3] = CLASS_IN;
    p += 4;
#endif
    plen = (int) (p - pkt);
    
    // start up a listener for the udp response
    ssock = socket( AF_INET, SOCK_DGRAM, 0 );
    if( ssock == -1 ) {
        free( pkt );
        diag_printf(" ** SOCKET FAILED\n");
        return -1;
    }
    
    memset( &ssa, 0, sizeof( struct sockaddr_in ) );
    ssa.sin_family = AF_INET;
    ssa.sin_port = htons( DNS_PORT );
    ssa.sin_addr.s_addr = INADDR_ANY;

    if( bind( ssock, (struct sockaddr*)&ssa, sizeof( ssa )) < 0 ) {
        free( pkt );
        close( ssock );
        diag_printf(" ** BIND FAILED\n");
        return -1;
    }

    // start up a client to ship off the request
    csock = socket( AF_INET, SOCK_DGRAM, 0 );
    if( csock == -1 ) {
        free( pkt );
        close( ssock);
        diag_printf(" ** NO SOCKET\n");
        return -1;
    }

    memset( &csa, 0, sizeof( struct sockaddr_in ) );
    csa.sin_family = AF_INET;
    csa.sin_port = htons( DNS_PORT );
    csa.sin_addr.s_addr = server->s_addr;
    
    sz = sizeof( csa );
    sendto( csock, pkt, plen, 0, (struct sockaddr*)&csa, sz );

    // set the receive timeout on the socket to two seconds
    tv.tv_sec  = 2;
    tv.tv_usec = 0;
    setsockopt( csock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv) );
    
    if( recvfrom( csock, pkt, 512, 0, (struct sockaddr*)&csa, &sz ) < 0 ) {
        close( csock ); close( ssock );
        free( pkt );
        diag_printf(" ** RECV BAILED\n");
        return -1;
    }

    // parse the response
    p = pkt;
    header = (dns_header_t*) p;
    p += sizeof( dns_header_t );

    header->qdcount   = ntohs( header->qdcount );
    header->ancount   = ntohs( header->ancount );
    header->nscount   = ntohs( header->nscount );
    header->arcount   = ntohs( header->arcount );
    //    header->fl.flags  = ntohs( header->fl.flags );

    // we are done with the sockets, close them now
    close( csock );
    close( ssock );
    
    // check the packet id and the response bit
    if( header->id != id || !(header->flags & 0x8000) ) {
        free( pkt );
        diag_printf(" ** FAILED HEADER, id 0x%08x, flags 0x%08x\n", header->id, header->flags );
        return -1;
    }

    // look to see if we got an actual answer to parse, or if the server was rude
    // and ignored our recursive query request
    if( header->ancount > 0 ) {
        // answer in packet
        if( header->qdcount != 1 ) {
            free( pkt );
            diag_printf(" ** BAD QDCOUNT\n");
            return -1;
        }
        // move past the question field
        p += sizeof( dns_query_t );
        // move past the name
        p += len+1;
    
        // we should be in the answer section now
        // step through the response until we find a host address
        for( i = 0; i < header->ancount; i++ ) {
            response = (dns_response_t*) p;
            p += sizeof( dns_response_t );

            response->rtype = ntohs( response->rtype );
            response->rlen = ntohs( response->rlen );

            // TODO probably unkosher to check the structure size
            if( response->rtype == TYPE_A && response->rlen == sizeof( struct in_addr ) ) {
                memcpy( addr, p, sizeof( struct in_addr ) );
                break;
            }

            p += response->rlen;
        }
    
        // we received an answer, but couldn't find an appropriate address record
        if( i == header->ancount ) {
            free(pkt);
            diag_printf(" ** NO ENTRY\n");
            return -1;
        }
    } else if( header->arcount != 0 ) {
#if 0
        // we're forced to do an iterative query here
        int z;
        
        // skip through question field and name
        p += sizeof( dns_query_t );
        p += len+1;

        diag_printf("** nscount = %d\n", header->nscount );
        // skip through the NS portion of the response
        for( z = 0; z < header->nscount; z++ ) {
            unsigned short data_length;
            // step through name
            while( *p ) p++;
            // step past null
            p++;
            // step past type (2) class (2) and ttl (4)
            p += 8;
            // the next two bytes are the data length; we need to step past
            // these two bytes plus their value (as a short)
            data_length = ((*(unsigned*)p)<<16) | (*(unsigned*)p+1);
            
            p += 2 + data_length;
            // this should deposit us at the next entry in the nameserver section
        }

        // now we arrive in the ar section, which actually has the nameserver names
        // and IP addresses.
        // be rude and only use the 'a' nameserver

        diag_printf(" ** I THINK AR SECTION STARTS AT %d\n", p-pkt);
        // skip resource name (2) type (2) class (2) ttl (4)
        p += 2 + 2 + 2 + 4;
        // TODO check type/class before skipping
        // verify length
        z = (*(unsigned*)p << 16) | (*(unsigned*)p+1);
        if( z != 4 ) {
            free( pkt );
            diag_printf(" ** BAD LENGTH FOR AR RECORD (%d)\n", z);
            return -1;
        }
        // skip past length
        p += 2;
        // get the powerup and win the game
        diag_printf(" ** iterative resolution, root nameserver ip %d.%d.%d.%d\n", *p, *(p+1),*(p+2),*(p+3));
        z = dns_resolve_internal( hostname, addr, (struct in_addr*) p );
        free( pkt );

        // short circuit the rest of the code; we dont want to update the cache with root name servers
        return z;
#else
        diag_printf(" ** Iteratitive DNS queries not supported\n");
        free( pkt );
        return -1;
#endif
    } else {
        // no answers and no referrals, so this header is bogus
        free( pkt );
        diag_printf(" ** EMPTY HEADER\n");
        return -1;
    }
    
    // cleanup
    free( pkt );
    
#ifdef CYGPKG_NET_DNSC_CACHE
    dns_update_cache( hostname, addr, ntohl(response->ttl) );
#endif
    
    return 0;
}

static char* copy_hostname( char* dest, const char* src ) 
{
    char* p = dest;
    int len, i;

    for( len=0, i = 0; ; i++ ) {
        if( src[i] == '.' || src[i] == '\0' ) {
            *p = len;
            len = 0;
            p = &dest[i+1];
            if( src[i] == '\0' ) {
                dest[i+1] = src[i];
                break;
            }
        }
        else {
            len++;
            dest[i+1] = src[i];
        }
    }
    dest[i+1] = src[i];

    return &dest[i+2];
}

