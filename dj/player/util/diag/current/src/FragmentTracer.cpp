// FragmentTracer.cpp: tool to track memory fragmentation
// danc@fullplaymedia.com
// 10/31/02

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <stdio.h>
#include "FragmentTracer.h"
#include <util/debug/debug.h>

DEBUG_MODULE(FRAG);
DEBUG_USE_MODULE(FRAG);

// State of an allocation
enum alloc_state
{
    FREE=0,
    ALLOCATED,
    RELEASED
};
// Object to represent a single allocation (20 bytes)
struct allocation_s
{
    void* _base_addr;    // location of this allocation
    void* _alloc_addr;   // address that called the allocation routine
    void* _free_addr;    // address that called the free routine
    unsigned int _len;   // length of this allocation
    alloc_state _state;  // state of this allocated block
};
typedef struct allocation_s allocation_t;
// Table size for our allocation list
#define MAX_ALLOCATIONS (128*1024)

// Table of allocation entries and current free index
static allocation_t allocation_table[MAX_ALLOCATIONS];
static int allocation_index = 0;
static cyg_mutex_t lock;

#define _INIT()     cyg_mutex_init( &lock )
#define _LOCK()     cyg_mutex_lock( &lock )
#define _UNLOCK()   cyg_mutex_unlock( &lock )

class init_class 
{
public:
    init_class() 
        {
            _INIT();
            memset( allocation_table, 0, MAX_ALLOCATIONS * sizeof(allocation_t) );
        };
};
static init_class _initme;

static void dump_alloc_table( void ) 
{
    DEBUGP( FRAG, DBGLEV_WARNING, "Dumping allocation table\n");
    DEBUGP( FRAG, DBGLEV_WARNING, "[%8s]\t%10s\t%10s\t%10s\t%8s\t%s\n",
        "Index", "Base addr", "All. Addr", "Free Addr", "Length", "State");
    for( int i = 0; i < MAX_ALLOCATIONS; i++ ) {
        allocation_t* p = &( allocation_table[i] );
        DEBUGP( FRAG, DBGLEV_WARNING, "[%08d]\t%p\t%p\t%p\t%08d\t%c\n",
            i, p->_base_addr, p->_alloc_addr, p->_free_addr, p->_len, (p->_state == FREE? 'f' : (p->_state == ALLOCATED ? 'a' : 'r')) );
    }
}

// Given a base address, find the allocation in our table (binary search)
static int find_allocation( void* base_addr ) 
{
    int res;
    
    for( res = 0; res < allocation_index; res++ ) {
        if( allocation_table[res]._base_addr == base_addr ) {
            break;
        }
        if( allocation_table[res]._len == 0 ) {
            break;
        }
    }
    if( allocation_table[res]._base_addr == base_addr ) {
        return res;
    }
    return -1;
}

static int add_allocation( void* base_addr, void* alloc_addr, unsigned int len ) 
{
    if( allocation_index == MAX_ALLOCATIONS ) {
        dump_alloc_table();
        DBASSERT( FRAG, 0, "no more room for allocations\n");
        return -1;
    }
    
    allocation_table[allocation_index]._base_addr = base_addr;
    allocation_table[allocation_index]._alloc_addr = alloc_addr;
    allocation_table[allocation_index]._len = len;
    allocation_table[allocation_index]._state = ALLOCATED;
    allocation_index++;

    return 0;
}

static int mark_free( void* base_addr, void* free_addr ) 
{
    int idx = find_allocation( base_addr );
    if( idx < 0 ) return -1;

    allocation_table[idx]._state = RELEASED;
    allocation_table[idx]._free_addr = free_addr;
    return 0;
}

// Interface to actual calls

extern "C" void* __wrap_malloc(int size);
extern "C" void  __wrap_free(void* p);
extern "C" void* __wrap_realloc(void* p, int size);
extern "C" void* __wrap_calloc(int nmemb, int size);
extern "C" void* __real_malloc(int c);
extern "C" void  __real_free(void* p);
extern "C" void* __real_realloc(void* p, int c);

static void* __allocate( size_t size, void* addr ) 
{
    void* p = __real_malloc( size );
    if( p ) {
        _LOCK();
        add_allocation( p, addr, size );
        _UNLOCK();
    }
    return p;
}

static void __release( void* p, void* addr ) 
{
    _LOCK();
    mark_free( p, addr );
    _UNLOCK();
    
    __real_free( p );
}

void* operator new(size_t size)     { return __allocate( size, __builtin_return_address(0) ); }
void* operator new[] (size_t size)  { return __allocate( size, __builtin_return_address(0) ); }
void* __wrap_malloc( int size )     { return __allocate( size, __builtin_return_address(0) ); }

void operator delete (void *p)      { return __release( p, __builtin_return_address(0) );     }
void operator delete[] (void *p)    { return __release( p, __builtin_return_address(0) );     }
void __wrap_free( void* p )         { return __release( p, __builtin_return_address(0) );     }

void* __wrap_realloc( void* p, int size ) 
{
#if 1
    return __real_realloc( p, size );
#else
    void* t = __allocate( size, __builtin_return_address(0) );

    if( t && p ) {
        _LOCK();
        int idx = find_allocation( p );
        if( idx >= 0 ) {
            // this should be safe here
            memcpy( t, p, allocation_table[idx]._len );
            _UNLOCK();
            __release( p, __builtin_return_address(0) );
        } else {
            _UNLOCK();
        }
    } else if( !t ) {
        __release( p, __builtin_return_address(0) );
    }
    
    return t;
#endif
}
void* __wrap_calloc(int nmemb, int size)
{
    int sz = nmemb * size;
    void* p;
    
    p = __allocate( sz, __builtin_return_address(0) );
    
    if( p ) {
        // calloc requires memory be zero initialized
        memset( p, 0, sz );
    }
    return p;
}

