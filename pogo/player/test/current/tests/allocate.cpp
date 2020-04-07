// allocate.cpp: regress the memory allocator/deallocator

#include <cyg/infra/diag.h>

#define USE_NEW

#ifndef USE_NEW
#include <stdlib.h>
#endif

int main( void ) {
    diag_printf("Start test allocate\n");
    for( int i = 0; i < 2000; i++ ) {
        diag_printf("iteration\n");
#ifdef USE_NEW
        char* p = new char[4096];
        delete [] p;
#else
        char* p = (char*) malloc( 4096 );
        free( p );
#endif
    }
    diag_printf("Done\n");
    return 0;
}
