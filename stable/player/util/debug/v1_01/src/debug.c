// debug.c: basic debug stub

#include <cyg/infra/diag.h>

#include <util/debug/debug.h>

// We provide a default assert handler and debug handler
void _default_assert( const char* file, const char* func, int line, const char* module, const char* cond, const char* fmt, ... );
void _default_debug( const char* mod, int sev, int settings, const char* fmt, ...);

debug_handler_t _debug_handler = _default_debug;
assert_handler_t _assert_handler = _default_assert;

void dbg_set_debug_handler( debug_handler_t h ) 
{
    if( h == 0 ) {
        _debug_handler = _default_debug;
    }
    else {
        _debug_handler = h;
    }
}

void dbg_set_assert_handler( assert_handler_t h ) 
{
    if( h == 0 ) {
        _assert_handler = _default_assert;
    }
    else {
        _assert_handler = h;
    }
}

void _default_debug( const char* mod, int sev, int settings, const char* fmt, ... )
{
    va_list ap;
    va_start( ap, fmt );
    diag_vprintf( fmt, ap );
    va_end( ap );
}

//
// By default, the assert handler generates an infinite loop at the end.
//
void _default_assert( const char* file, const char* func, int line, const char* module, const char* cond, const char* fmt, ... )
{
    diag_printf(" %s:%d (in %s): ASSERTION FAILED\n   (%s)\n", file, line, func, cond );
    va_list ap;
    va_start( ap, fmt );

    diag_vprintf(fmt, ap);

    va_end(ap);

    diag_printf("\n");
    while(1);
}
