// symtable.c: symbol table lookups
// dj lovin. fresh diggity dog stylin

#include <cyg/kernel/kapi.h>
#include "symtable.h"

#define SYMTABLE_SIGNATURE 0x5a5a5a5a


static symbol_t* symbol_table;
static char* string_table;
static int num_entries = 0;
static unsigned int lower_addr_limit, upper_addr_limit;

// arbitrarily allocate 300k for the table; some tables may be larger
// this is abusive of the format of the linker directive script, which places
// the ".2ram.*" sections last
static char _symbol_table_block[300*1024] __attribute__((section(".2ram.symtbl")));

int symtable_init( const char* sym_table_ptr ) 
{
    unsigned int* p;
    if( num_entries > 0 ) {
        // already init
        return 0;
    }
    if( !sym_table_ptr ) {
        sym_table_ptr = _symbol_table_block;
    }
    
    // round to next word
    p = (unsigned int*) (((int)sym_table_ptr + 3) & ~0x03);
    if( p[0] != SYMTABLE_SIGNATURE ) {
        return -1;
    }
    num_entries = p[1];
    symbol_table = (symbol_t*) &( p[2] );
    string_table = (char*)(symbol_table + num_entries); // added in units of size symbol_t

    lower_addr_limit = symbol_table[0].base_address;
    upper_addr_limit = symbol_table[num_entries-1].base_address + symbol_table[num_entries-1].func_length;
    
    return 0;
}

int symtable_lookup( unsigned int address, const symbol_t** symbol_data ) 
{
    int i,lower_bound,upper_bound;
    
    // check to see if the address falls in range
    if( address < lower_addr_limit || address > upper_addr_limit ) {
        return -1;
    }

    // binary search
    lower_bound = 0;
    upper_bound = num_entries-1;
    i = upper_bound/2;
    do {
        if( address >= symbol_table[i].base_address ) {
            if( (i < (num_entries-1) && symbol_table[i+1].base_address > address) ||
                symbol_table[i].base_address + symbol_table[i].func_length > address) {
                // match
                *symbol_data = &( symbol_table[i] );
                return 0;
            } else { // address > current function
                lower_bound = i+1;
                i += (upper_bound-i)/2;
            }
        } else { // address < current function
            upper_bound = i-1;
            if( i == 1 ) {
                i = 0;
            } else {
                i = (i+1)/2;
            }
        }
    } while(1);
}

// arm specific code
int dump_stack( int limit, unsigned int sp ) 
{
    int i, frame = 0;
    unsigned int* p = (unsigned int*)sp;
    const symbol_t* s;
    unsigned int stack_base = cyg_thread_get_stack_base( cyg_thread_self() );
    unsigned int stack_size = cyg_thread_get_stack_size( cyg_thread_self() );

    printf("dumpstack running at sp 0x%08x\n", sp );
    printf("base: 0x%08x size: 0x%08x", stack_base, stack_size );
#ifdef CYGFUN_KERNEL_THREADS_STACK_MEASUREMENT
    {
        unsigned int stack_usage = cyg_thread_measure_stack_usage( cyg_thread_self() );
        printf(" usage: 0x%08x", stack_usage );
    }
#endif
    printf("\n");

    printf("Frame\tBase Address\tCurrent Address\tFunction name\n");
    for( i = 0; (p+i) < (stack_base+stack_size); i++ ) {
        if( symtable_lookup( p[i], &s ) < 0 ) {
            printf(" failed lookup for 0x%08x\n", p[i] );
        } else {
            printf("%2d\t0x%08x\t0x%08x\t%s\n", frame, s->base_address, p[i], string_table + s->string_offset );
            frame++;
        }
    }
    printf("\nDone\n");
    return 0;
}

#if 0
// app code
#include <stdio.h>
#include <string.h>

void usage( void ) 
{
    printf("usage: symtable <symbol file>\n");
}

int main( int argc, char** argv ) 
{
    FILE* fp;
    long len;
    unsigned int addr;
    char* ptr;

    const symbol_t* symptr;
    
    if( argc != 2 ) {
        usage();
        return -1;
    }

    fp = fopen( argv[1], "r" );
    if( !fp ) {
        usage();
        return -1;
    }

    fseek( fp, 0, SEEK_END );
    len = ftell( fp );
    fseek( fp, 0, SEEK_SET );

    ptr = (char*) malloc( len );

    if( !ptr ) {
        return -1;
    }

    if( len != fread( ptr, 1, len, fp ) ) {
        usage();
        return -1;
    }
    fclose( fp );

    if( symtable_init( ptr ) == -1 ) {
        printf("failed to init symbol table\n");
        return -1;
    }

    addr = 0x1222f0;
    if( symtable_lookup( addr, &symptr ) < 0 ) {
        printf(" bad address %d\n", addr);
        return -1;
    }
    printf(" symptr: base_addr = 0x%08x, length = 0x%08x, name = %s\n",
        symptr->base_address, symptr->func_length, string_table + symptr->string_offset );
    return 0;
}

#endif
