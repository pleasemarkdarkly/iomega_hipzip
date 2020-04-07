// alloclimit.cpp: test the memory allocation limit

#include <cyg/infra/diag.h>

#include <pkgconf/mlt_arm_edb7312_ram.h>

#define ALLOC_LIM CYGMEM_REGION_ram_SIZE
#define BLOCK_SIZE 4096
#define PROGRESS_RATE 50

//#define USE_NEW

#ifdef USE_NEW

#define create(i)  new char[i]
#define destroy(p) delete [] p
#else

#include <stdlib.h>
#define create(i)  (char*)malloc( sizeof(char) * i )
#define destroy(p) free( (void*)p )
#endif

int main( void ) {
    diag_printf("Start test alloclimit\n");
    diag_printf("Ram size %d bytes, block size %d bytes\n", ALLOC_LIM, BLOCK_SIZE );
    
    char* prog = "\\|/-";
    char* pos = prog;
    int rate = PROGRESS_RATE;
    
    int count = BLOCK_SIZE;
    char* p;
    
    while( count < ALLOC_LIM ) {
        p = create( count );

        if( !p ) break;

        count += BLOCK_SIZE;
        destroy( p );
        p = NULL;

        if( --rate == 0 ) {
            rate = PROGRESS_RATE;
            diag_printf("%d\n", count-BLOCK_SIZE);
            if( *++pos == 0 )  pos = prog;
        }
    }

    diag_printf("Maximum allocatable block: %d bytes\n", (count - BLOCK_SIZE));
    
    return 0;
}
