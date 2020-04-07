// symtable.h: support for the symbol table

#ifndef __SYMTABLE_H__
#define __SYMTABLE_H__

typedef struct symbol_s 
{
    unsigned int base_address;
    unsigned int func_length;
    unsigned int string_offset;
} symbol_t;

#ifdef __cplusplus
extern "C" {
#endif
    
// call to set up the global symtable
int symtable_init( const char* sym_table_ptr );

// call with an address and a symbol_t*, returns -1 on failure or 0 on success.
// on success symbol_data is set to point to the discovered symbol info
int symtable_lookup( unsigned int address, const symbol_t** symbol_data );

// call with a print routine (such as printf or diag_printf) to get a printed stack dump
// if limit == 0, dump_stack will run until it hits the stack base; otherwise it will
// only dump limit frames
int dump_stack( int limit, unsigned int sp );

#ifdef __cplusplus
};
#endif
    
#endif // __SYMTABLE_H__
