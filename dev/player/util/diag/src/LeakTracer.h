// LeakTracer.h: wow look! i can use headers!
// danc

#ifndef __LEAKTRACER_H__
#define __LEAKTRACER_H__

#include <cyg/kernel/kapi.h> // mutex


// Abort reasons
enum abortReason_t {
    OVERWRITE_MEMORY     = 0x01,
    DEALLOC_NONEXISTENT  = 0x02,
    TYPE_MISMATCH        = 0x04,
    ALLOCATION_LIMIT     = 0x08
};

enum allocationType_t 
{
    _RELEASED=0,
    _STANDARD_NEW,
    _ARRAY_NEW,
    _MALLOC
};
#define   ALLOC_TYPE_TO_STR(x) ((x)==_MALLOC?"malloc":(x)==_ARRAY_NEW?"new[]":(x)==_STANDARD_NEW?"new":"(released)")
#define DEALLOC_TYPE_TO_STR(x) ((x)==_MALLOC?"free":(x)==_ARRAY_NEW?"delete[]":(x)==_STANDARD_NEW?"delete":"(released)")

// configuration

// types of memory operations to cover
//  tracking malloc requires
//  "--wrap malloc" "--wrap free" and "--wrap realloc" be passed to ld
//   (if linking with gcc, "-Wl,--wrap,malloc,--wrap,free,--wrap,realloc")

#define TRACK_MALLOC
#define TRACK_NEW

// Magic number to use after the allocation for tracking memory overwrites
#define MAGIC_TYPE  unsigned long
#define MAGIC_VALUE 0xAABBCCDDLu

// Number of bytes to pad allocations with
#define ALLOCATION_PAD 0

// These operations boil down to memsets which:
//  1) initialize allocated memory prior to handing off a pointer
//  2) initialize released memory
// undefine these if you dont want this
//#define PREINITIALIZE_NEW
//#define PREINITIALIZE_MALLOC
//#define CLEAN_DELETE
//#define CLEAN_FREE

// Defining this will cause free/delete to NOT RELEASE MEMORY
//  Instead, they will initialize the memory to a predetermined value
//  this is useful for tracking stale pointers
//#define TRACK_STALE_POINTERS

// Allows an assertion to be forced after a specified number of total allocations
//  ignored if ALLOCATION_LIMIT not set in ABORT_REASONS
#define MAX_ALLOCATION_LIMIT 16*1024

// Bitfield controlling what things we abort on (from abortReason_t)
#define ABORT_REASONS (OVERWRITE_MEMORY)

// Size of the log buffer in bytes
#define LOGBUFFER_SIZE (64*1024)

// Define for thread safety
#define THREAD_SAFE


// C api for dumping leak trace
#ifndef __cplusplus
void dump_leak_trace( void );
#else
// class structure
extern "C" void dump_leak_trace( void );

class LeakTracer {
public:
    LeakTracer();
    ~LeakTracer();

    // if stale pointer checking is enabled, do a sweep here
    void spcheck();
    
    void ltlog(const char *fmt, ...);
    void ltflush();
    
    void initialize();
	
	/*
	 * the workhorses:
	 */
	void *registerAlloc(size_t size, allocationType_t type, void * allocAddr);
    void *registerRealloc(void* p, size_t size, allocationType_t type, void* allocAddr);
	void  registerFree (void *p, allocationType_t type, void * allocAddr);

	/**
	 * write a hexdump of the given area.
	 */
	void  hexdump(const unsigned char* area, int size);
	
	/**
	 * Terminate current running progam.
	 */
	void progAbort(abortReason_t reason);

	/**
	 * write a Report over leaks, e.g. still pending deletes
	 */
	void writeLeakReport();

private:
    struct Leak {
		const void *addr;
		size_t      size;
		const void *allocAddr;
#ifdef TRACK_STALE_POINTERS
        const void *freeAddr;
#endif
        allocationType_t type;
		int         nextBucket;
	};
	
	int  newCount;      // how many memory blocks do we have
	int  leaksCount;    // amount of entries in the leaks array
	int  firstFreeSpot; // Where is the first free spot in the leaks array?
	int  currentAllocated; // currentAllocatedMemory
	int  maxAllocated;     // maximum Allocated
    int  maxAllowableAllocations;  // hard limit for allocation totals
	unsigned long totalAllocations; // total number of allocations. stats.
	unsigned int  abortOn;  // resons to abort program (see abortReason_t)


	int report;       // filedescriptor to write to

	/**
	 * pre-allocated array of leak info structs.
	 */
	Leak *leaks;

	/**
	 * fast hash to lookup the spot where an allocation is 
	 * stored in case of an delete. map<void*,index-in-leaks-array>
	 */
	int  *leakHash;     // fast lookup

#ifdef THREAD_SAFE
	cyg_mutex_t mutex;
    void initlock() 
        { cyg_mutex_init(&mutex);    }
    void destroylock()
        { cyg_mutex_destroy(&mutex); }
    void lock()
        { cyg_mutex_lock(&mutex);    }
    void unlock()
        { cyg_mutex_unlock(&mutex);  }
#else
    void initlock()    {}
    void destroylock() {}
    void lock()        {}
    void unlock()      {}
#endif

	/**
	 * Have we been initialized yet?  We depend on this being
	 * false before constructor has been called!  
	 */
	bool initialized;	
	bool destroyed;		// Has our destructor been called?

};


#endif // __cplusplus
#endif // __LEAKTRACER_H__
