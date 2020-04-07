/* 
 * Homepage: <http://www.andreasen.org/LeakTracer/>
 *
 * Authors:
 *  Erwin S. Andreasen <erwin@andreasen.org>
 *  Henner Zeller <foobar@to.com>
 *
 * This program is Public Domain
 */

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#include <util/debug/debug.h>
#include <util/diag/diag.h>
#include <main/main/AppSettings.h>
#include "LeakTracer.h"


DEBUG_MODULE_S(LEAKTRACER, DBGLEV_DEFAULT | DBGLEV_INFO);
DEBUG_USE_MODULE(ATA);


/*
 * underlying allocation, de-allocation used within 
 * this tool
 */
#ifdef TRACK_MALLOC
#define LT_MALLOC  __real_malloc
#define LT_FREE    __real_free
#define LT_REALLOC __real_realloc

// prototypes for actual definitions
extern "C" void* __real_malloc(int c);
extern "C" void  __real_free(void* p);
extern "C" void* __real_realloc(void* p, int c);
#else

#define LT_MALLOC  malloc
#define LT_FREE    free
#define LT_REALLOC realloc
#endif


/*
 * prime number for the address lookup hash table.
 * if you have _really_ many memory allocations, use a
 * higher value, like 343051 for instance.
 */
#define SOME_PRIME 35323
#define ADDR_HASH(addr) ((unsigned long) addr % SOME_PRIME)

/**
 * allocate a bit more memory in order to check if there is a memory
 * overwrite. Either 0 or more than size    of(unsigned int). Note, you can
 * only detect memory over_write_, not _reading_ beyond the boundaries. Better
 * use electric fence for these kind of bugs 
 *   <ftp://ftp.perens.com/pub/ElectricFence/>
 */
// import from header configuration
typedef MAGIC_TYPE magic_t;
#define MAGIC MAGIC_VALUE
static MAGIC_TYPE _magic = MAGIC;

/**
 * this may be more than sizeof(magic_t); if you want more, then
 * sepecify it like #define SAVESIZE (sizeof(magic_t) + 12)
 */
#define SAVESIZE  (sizeof(magic_t) + ALLOCATION_PAD)

/**
 * on 'new', initialize the memory with this value.
 * if not defined - uninitialized. This is very helpful because
 * it detects if you initialize your classes correctly .. if not,
 * this helps you faster to get the segmentation fault you're 
 * implicitly asking for :-). 
 *
 * Set this to some value which is likely to produce a
 * segmentation fault on your platform.
 */
#if defined(PREINITIALIZE_NEW)
#define SAVEVALUE_NEW   0xAA
#endif
#if defined(PREINITIALIZE_MALLOC)
#define SAVEVALUE_MALLOC 0xBB
#endif

/**
 * on 'delete', clean memory with this value.
 * if not defined - no memory clean.
 *
 * Set this to some value which is likely to produce a
 * segmentation fault on your platform.
 */
#if defined(CLEAN_DELETE)
#define MEMCLEAN_DELETE    0xEE
#endif
#if defined(CLEAN_FREE)
#define MEMCLEAN_FREE      0xFF
#endif

#ifdef TRACK_STALE_POINTERS
#define STALE_MEM_VAL      0x55
#endif

/**
 * Initial Number of memory allocations in our list.
 * Doubles for each re-allocation.
 */
#define INITIALSIZE 65536

//only works for non-signed ints
static void Simpleitoa(int Num, char* pString)
{
	int i;
	for(i = 0;Num > 0;i++)
	{
		pString[i] = char((Num % 10) + 48);
		Num/= 10;
	}
	pString[i] = '\0';
	for(int j = 0, k = i - 1;j < i/2;j++,k--)
	{
		char temp = pString[j];
		pString[j] = pString[k];
		pString[k] = temp;
	}
}

static char logbuffer[LOGBUFFER_SIZE];
static unsigned int lbi = 0;

static LeakTracer leakTracer;

LeakTracer::LeakTracer()
{
    initialize();
}
LeakTracer::~LeakTracer() 
{
    //DEBUG(LEAKTRACER, DBGLEV_INFO, "LeakTracer::destroy()\n");
    //time_t t = time(NULL);
    ltlog("# LeakTracer finished\n");
    //    writeLeakReport();
    ltflush();
    free(leaks);
    destroylock();
    destroyed = true;
}
void LeakTracer::spcheck() 
{
#ifdef TRACK_STALE_POINTERS
    // step through the allocation table looking at all the entries
    // see 
    lock();
    int cnt = 0;

    for( int i = 0; i < leaksCount; i++ ) {
        if( leaks[i].type == _RELEASED ) {
            for( unsigned int z = 0; z < leaks[i].size; z++ ) {
                if( ((unsigned char*)leaks[i].addr)[z] != STALE_MEM_VAL ) {
                    ltlog(
                        "P %10p %10p %10p %d  "
                        "# stale pointer referenced; %d byte obj\n",
                        leaks[i].allocAddr,
                        leaks[i].freeAddr,
                        leaks[i].addr,
                        z,
                        leaks[i].size );
                    cnt++;
                    break;
                }
            }
        }
    }
    ltlog("# %d total entries, %d stale pointer references\n", leaksCount, cnt);
    unlock();
#endif
}

void LeakTracer::ltlog( const char* fmt, ... ) 
{
    va_list ap;
    va_start( ap, fmt );
    
    static char szFormatBuffer[512];
    diag_vsprintf(szFormatBuffer,fmt,ap);

    if( (lbi + strlen(szFormatBuffer)) > sizeof(logbuffer) ) {
        return ;
    }
    sprintf(&logbuffer[lbi], "%s", szFormatBuffer);
    lbi += strlen(szFormatBuffer);
    if ( lbi >= sizeof(logbuffer) ) {
        DEBUG(LEAKTRACER, DBGLEV_INFO, "lbi %d sizeof(logbuffer) %d strlen(szFormatBuffer) %d\n", lbi,
            sizeof(logbuffer), szFormatBuffer);
        unlock();
    }
    
    va_end( ap );
}
void LeakTracer::ltflush() {
    // diag_printf doesn't appear to handle really large strings, so do this one character at a time
    // this is so fucking ghetto
    for ( unsigned int i = 0; i < lbi; ++i ) {
        //        DEBUGP(LEAKTRACER, DBGLEV_INFO, "%c", logbuffer[i]);
        diag_printf("%c", logbuffer[i]);
    }
}
void LeakTracer::initialize() 
{
    // Unfortunately we might be called before our constructor has actualy fired
    if (initialized)
        return;

    //DEBUG(LEAKTRACER, DBGLEV_INFO, "LeakTracer::initialize()\n");
    initialized = true;
    newCount = 0;
    leaksCount = 0;
    firstFreeSpot = 1; // index '0' is special
    currentAllocated = 0;
    maxAllocated = 0;
    maxAllowableAllocations = MAX_ALLOCATION_LIMIT;
    totalAllocations = 0;
    abortOn =  ABORT_REASONS;
    report = -1;
    leaks = 0;
    leakHash = 0;


    //time_t t = time(NULL);
    ltlog("# LeakTracer starting\n");

    leakHash = (int*) LT_MALLOC(SOME_PRIME * sizeof(int));
    memset ((void*) leakHash, 0x00, SOME_PRIME * sizeof(int));

#ifdef MAGIC
    ltlog("# memory overrun protection of %d Bytes "
        "with magic ", SAVESIZE );
    hexdump( (unsigned char*)&_magic, sizeof(MAGIC_TYPE) );
#endif
		
#ifdef PREINITIALIZE_NEW
    ltlog("# initializing new memory with 0x%2X\n",
        SAVEVALUE_NEW);
#endif
#ifdef PREINITIALIZE_MALLOC
    ltlog("# initializing malloc memory with 0x%2X\n",
        SAVEVALUE_MALLOC);
#endif
#ifdef CLEAN_DELETE
    ltlog("# sweeping deleted memory with 0x%2X\n",
        MEMCLEAN_DELETE);
#endif
#ifdef CLEAN_FREE
    ltlog("# sweeping freed memory with 0x%2X\n",
        MEMCLEAN_FREE);
#endif

#define PRINTREASON(x) if (abortOn & x) ltlog("%s ", #x);
    ltlog("# aborts on ");
    PRINTREASON( OVERWRITE_MEMORY );
    PRINTREASON( DEALLOC_NONEXISTENT );
    PRINTREASON( TYPE_MISMATCH );
    PRINTREASON( ALLOCATION_LIMIT );
    ltlog("\n");
#undef PRINTREASON

#ifdef THREAD_SAFE
    ltlog("# thread safe\n");
#else
    ltlog("# not thread safe\n");
#endif
    
    initlock();
}

void LeakTracer::progAbort(abortReason_t reason)
{
    if (abortOn & reason) {
        ltlog( "# abort; DUMP of current state\n");
        //        writeLeakReport();
        spcheck();
        ltflush();
        DBASSERT(LEAKTRACER,0,"abort\n");
    }
    //else
    //ltflush();
}

void LeakTracer::hexdump(const unsigned char* area, int size) {
	ltlog( "# ");
	for (int j=0; j < size ; ++j) {
		ltlog("%02x ", *(area+j));
		if (j % 16 == 15) {
			ltlog( "  ");
			for (int k=-15; k < 0 ; k++) {
				char c = (char) *(area + j + k);
				ltlog("%c", /*isprint(c) ? c : '.'*/ c);
			}
			ltlog( "\n# ");
		}
	}
	ltlog( "\n");
}

void LeakTracer::writeLeakReport() {
	initialize();

	if (newCount > 0) {
		ltlog( "# LeakReport\n");
		ltlog( "# %10s | %9s  # Pointer Addr\n",
			"from new @", "size");
	}
	for (int i = 0; i <  leaksCount; i++)
		if (leaks[i].addr != NULL) {
			// This ought to be 64-bit safe?
			ltlog( "L %10p   %9ld  # %p\n",
				leaks[i].allocAddr,
				(long) leaks[i].size,
				leaks[i].addr);
		}
	ltlog( "# total allocation requests: %6ld ; max. mem used"
		" %d kBytes\n", totalAllocations, maxAllocated / 1024);
	ltlog( "# leak %6d Bytes\t:-%c\n", currentAllocated,
		(currentAllocated == 0) ? ')' : '(');
	if (currentAllocated > 50 * 1024) {
		ltlog( "# .. that is %d kByte!! A lot ..\n", 
			currentAllocated >> 10);
	}
}

void* LeakTracer::registerAlloc (size_t size, allocationType_t type, void * allocAddr) {
	initialize();

    //DEBUG(LEAKTRACER, DBGLEV_INFO, "LeakTracer::registerAlloc(%p)\n", allocAddr);

	if (destroyed) {
		DEBUG(LEAKTRACER, DBGLEV_INFO, "Oops, registerAlloc called after destruction of LeakTracer (size=%d)\n", size);
		return LT_MALLOC(size);
	}


	void *p = LT_MALLOC(size + SAVESIZE);
	// Need to call the new-handler
	if (!p) {
		ltlog( "LeakTracer malloc %d\n", size + SAVESIZE );
        print_mem_usage();
        writeLeakReport();
        while(1);
        DBASSERT(LEAKTRACER,0,"LeakTracer unable to malloc %d bytes\n", size + SAVESIZE);
	}

#ifdef SAVEVALUE_NEW
    if( type == _ARRAY_NEW || type == _STANDARD_NEW ) {
        /* initialize with some defined pattern */
        memset(p, SAVEVALUE_NEW, size + SAVESIZE);
    }
#endif
#ifdef SAVEVALUE_MALLOC
    if( type == _MALLOC ) {
        memset(p, SAVEVALUE_MALLOC, size+SAVESIZE);
    }
#endif
#ifdef MAGIC
	/*
	 * the magic value is a special pattern which does not need
	 * to be uniform.
	 */
    /* dc- dont assume aligned pointers. duh.*/
	if (SAVESIZE >= sizeof(magic_t)) {
        memcpy( ((char*)p) + size, &_magic, sizeof( unsigned int ) );
	}
#endif

    lock();

	++newCount;
	++totalAllocations;
    if( newCount >= maxAllowableAllocations ) {
        if( abortOn & ALLOCATION_LIMIT ) {
            ltlog( "# hard limit of %d allocations hit\n", maxAllowableAllocations );
            progAbort( ALLOCATION_LIMIT );
        }
    }
    
	currentAllocated += size;
	if (currentAllocated > maxAllocated)
		maxAllocated = currentAllocated;
	
	for (;;) {
		for (int i = firstFreeSpot; i < leaksCount; i++)
			if (leaks[i].addr == NULL) {
				leaks[i].addr      = p;
				leaks[i].size      = size;
				leaks[i].type      = type;
				leaks[i].allocAddr = allocAddr;
				firstFreeSpot      = i+1;
				// allow to lookup our index fast.
				int *hashPos        = &leakHash[ ADDR_HASH(p) ];
				leaks[i].nextBucket = *hashPos;
				*hashPos            = i;
                unlock();
				return p;
			}
		
		// Allocate a bigger array
		// Note that leaksCount starts out at 0.
		int new_leaksCount = (leaksCount == 0) ? INITIALSIZE 
                             : leaksCount * 2;
        // dont call realloc here, since __wrap_realloc actually calls unwrapped malloc/free :/
        void* newp = LT_MALLOC( sizeof(Leak) * new_leaksCount );
		if (!newp) {
			ltlog( "# LeakTracer realloc failed %d\n", new_leaksCount * sizeof(Leak));
            unlock();
            dump_leak_trace();
            DBASSERT(LEAKTRACER,0,"LeakTracer realloc failed\n");
		}
		else {
            if( leaksCount ) {
                memcpy( newp, leaks, sizeof(Leak) * leaksCount );
            }
            LT_FREE( leaks );
            leaks = (LeakTracer::Leak*)newp;
			ltlog( "# internal buffer now %d\n", 
				new_leaksCount);
		}
		memset(leaks+leaksCount, 0x00,
            sizeof(Leak) * (new_leaksCount-leaksCount));
		leaksCount = new_leaksCount;
	}
}
void* LeakTracer::registerRealloc(void* p, size_t size, allocationType_t type, void* allocAddr)
{
    initialize();
    
    void* newp = NULL;
    lock();
    // 1) find the existing allocation
    int* lastPointer = &leakHash[ ADDR_HASH(p) ];
    int i = *lastPointer;

    while( i != 0 && leaks[i].addr != p) {
        lastPointer = &leaks[i].nextBucket;
        i = *lastPointer;
    }

    if( leaks[i].addr == p ) {
        // hit
        if( leaks[i].type != type ) {
            ltlog(
                "S %10p %10p  # realloc called on alloc of type %s "
                "; size %d\n",
                leaks[i].allocAddr,
                allocAddr,
                ALLOC_TYPE_TO_STR(leaks[i].type),
                leaks[i].size);
            progAbort( TYPE_MISMATCH );
        }
        unsigned int copysize = leaks[i].size;
        if( copysize > size ) copysize = size;
        unlock();
        newp = this->registerAlloc( size, type, allocAddr );
        memcpy( newp, p, copysize );
        this->registerFree( p, type, allocAddr );
    } else {
        // miss
        ltlog( "D %10p             # realloc non alloc or twice pointer %10p\n", 
            allocAddr, p);
        unlock();
    }
    return newp;
}
void LeakTracer::registerFree (void *p, allocationType_t type, void * allocAddr)
{
	initialize();

    //DEBUG(LEAKTRACER, DBGLEV_INFO, "LeakTracer::registerFree(%p)\n", allocAddr);
        
	if (p == NULL)
		return;

	if (destroyed) {
		DEBUG(LEAKTRACER, DBGLEV_INFO, "Oops, allocation destruction of LeakTracer (p=%p)\n", p);
		return;
	}

    lock();

	int *lastPointer = &leakHash[ ADDR_HASH(p) ];
	int i = *lastPointer;

	while (i != 0 && leaks[i].addr != p) {
		lastPointer = &leaks[i].nextBucket;
		i = *lastPointer;
	}

	if (leaks[i].addr == p) {
#ifdef TRACK_STALE_POINTERS
        leaks[i].type = _RELEASED;
        leaks[i].freeAddr = allocAddr;
        memset( p, STALE_MEM_VAL, leaks[i].size );
#else
        // we are actually releasing the memory, so clean up the entry
		*lastPointer = leaks[i].nextBucket; // detach.
		newCount--;
		leaks[i].addr = NULL;
		currentAllocated -= leaks[i].size;
		if (i < firstFreeSpot)
			firstFreeSpot = i;

		if (leaks[i].type != type) {
			ltlog( 
				"S %10p %10p  # %s but %s "
				"; size %d\n",
				leaks[i].allocAddr,
				allocAddr,
                ALLOC_TYPE_TO_STR(leaks[i].type),
                DEALLOC_TYPE_TO_STR(type),
				leaks[i].size);
			
			progAbort( TYPE_MISMATCH );
		}
#endif
#ifdef MAGIC
        unsigned int mag;
        char* magic_bytes = ((char*)p)+leaks[i].size;
        memcpy( &mag, magic_bytes, sizeof( unsigned int ) );
		if ((SAVESIZE >= sizeof(magic_t)) && mag != _magic ) {
			ltlog( "O %10p %10p %10p  "
				"# memory overwritten beyond allocated"
				" %d bytes\n",
				leaks[i].allocAddr,
				allocAddr,
                p,
				leaks[i].size);
			ltlog( "# %d byte beyond area (%c %c %c %c):\n",
				SAVESIZE, magic_bytes[0], magic_bytes[1], magic_bytes[2], magic_bytes[3]);
			hexdump((unsigned char*)p+leaks[i].size,
				SAVESIZE);
            if( leaks[i].size >= 8 ) {
                char* v = (char*)p;
                ltlog( "# 8 initial bytes (%c %c %c %c %c %c %c %c):\n",
                    v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7]);
                hexdump((unsigned char*)v,8);
            }
            else if( leaks[i].size >= 4 ) {
                char* v = (char*)p;
                ltlog( "# 4 initial bytes (%c %c %c %c): \n",
                    v[0],v[1],v[2],v[3]);
                hexdump((unsigned char*)v,4);
            }
            
			progAbort( OVERWRITE_MEMORY );
		}
#endif

        unlock();

#ifdef MEMCLEAN_DELETE
        if( type == _STANDARD_NEW || type == _ARRAY_NEW ) {
            int allocationSize = leaks[i].size;
            // set it to some garbage value.
            memset((unsigned char*)p, MEMCLEAN_DELETE, allocationSize + SAVESIZE);
        }
#endif
#ifdef MEMCLEAN_FREE
        if( type == _MALLOC ) {
            int allocationSize = leaks[i].size;
            memset((unsigned char*)p, MEMCLEAN_FREE, allocationSize + SAVESIZE);
        }
#endif
#ifndef TRACK_STALE_POINTERS
		LT_FREE(p);
#endif
		return;
	}

    unlock();

	ltlog( "D %10p             # dealloc non alloc or twice pointer %10p\n", 
		allocAddr, p);
	progAbort( DEALLOC_NONEXISTENT );
}

/** -- The actual new/delete operators -- **/
#ifdef TRACK_NEW
void* operator new(size_t size) {
	return leakTracer.registerAlloc(size,_STANDARD_NEW,__builtin_return_address(0));
}
void* operator new[] (size_t size) {
	return leakTracer.registerAlloc(size,_ARRAY_NEW,__builtin_return_address(0));
}
void operator delete (void *p) {
	leakTracer.registerFree(p,_STANDARD_NEW,__builtin_return_address(0));
}
void operator delete[] (void *p) {
	leakTracer.registerFree(p,_ARRAY_NEW,__builtin_return_address(0));
}
#endif

#ifdef TRACK_MALLOC
extern "C" void* __wrap_malloc(int size);
extern "C" void  __wrap_free(void* p);
extern "C" void* __wrap_realloc(void* p, int size);
extern "C" void* __wrap_calloc(int nmemb, int size);

void* __wrap_malloc( int size ) 
{
    return leakTracer.registerAlloc( size, _MALLOC, __builtin_return_address(0) );
}
void __wrap_free( void* p )
{
    return leakTracer.registerFree( p,_MALLOC,__builtin_return_address(0) );
}
void* __wrap_realloc( void* p, int size ) 
{
    if( p ) {
        return leakTracer.registerRealloc( p, size, _MALLOC, __builtin_return_address(0) );
    }
    else {
        return leakTracer.registerAlloc( size, _MALLOC, __builtin_return_address(0) );
    }
}
void* __wrap_calloc(int nmemb, int size)
{
    int sz = nmemb * size;
    void* p = leakTracer.registerAlloc( sz, _MALLOC, __builtin_return_address(0) );
    if( p ) {
        // calloc requires memory be zero initialized
        memset( p, 0, sz );
    }
    return p;
}
#endif

/* external interface to dump trace */
void dump_leak_trace( void )
{
    diag_printf("Dumping leak trace\n");
    leakTracer.spcheck();
    leakTracer.writeLeakReport();
    leakTracer.ltflush();
}
