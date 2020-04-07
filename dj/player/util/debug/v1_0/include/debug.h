// debug.h: debugging hooks
// danc@iobjects.com 6/19/01
// (c) Interactive Objects

#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <cyg/infra/diag.h>

//
// Basic concept of debugging:
//  code is organized into debugging modules. each module has a byte associated with
//  it which indicates which severity of errors are printed for that module.
//  you can specify -DDEBUG_LEVEL=x on the command line, which will force all modules
//  to only print errors of a certain type. -DDEBUG_LEVEL=0 will cause all debugging to
//  be preprocessed out
//
// Debug and assertion handlers
//  users can specify a debug handler instead of the standard 'printf' style output; this
//  allows debug info to be routed elsewhere if needed. additionally, an assertion handler
//  can be used to route assertions, or provide a more handy function to set breakpoints in
//

//
// Hooks for the debug/assert handlers
//
#ifdef __cplusplus
#define extC extern "C"
#else
#define extC extern
#endif

typedef void (*debug_handler_t)(const char* mod, int sev, int settings, const char* fmt, ...);
typedef void (*assert_handler_t)(const char* file, const char* func, int line, const char* module, const char* cond, const char* fmt, ... );

// These functions always exist; if DEBUG_LEVEL==0, they are stub routines
extC void dbg_set_debug_handler( debug_handler_t h );
extC void dbg_set_assert_handler( assert_handler_t h );

extC assert_handler_t _assert_handler;

//
// Debugging levels
//
#define DBGLEV_OFF      0x00  // No messages
#define DBGLEV_CHATTER  0x01  // Driver level chatter
#define DBGLEV_INFO     0x02  // Generic information/status

#define DBGLEV_TRACE    0x08  // Tracing
#define DBGLEV_WARNING  0x10  // Warnings, conditions that are handled

#define DBGLEV_ERROR    0x20  // Errors, possibly not handled
#define DBGLEV_FATAL    0x40  // Fatal errors

#define DBGLEV_ASSERT   0x80  // Assertions, these should probably be split off

#define DBGLEV_DEFAULT (DBGLEV_FATAL | DBGLEV_ERROR | DBGLEV_WARNING | DBGLEV_ASSERT)

#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL  DBGLEV_DEFAULT
#else
#define MANUAL_DEBUG_LEVEL DEBUG_LEVEL
#endif

#if DEBUG_LEVEL==0
#define DEBUG_OUTPUT(fmt,args...)

#define DEBUG_MODULE(name)
#define DEBUG_MODULE_S(name,severity)
#define DEBUG_USE_MODULE(name)

#define DEBUG_ENABLE(name,severity)
#define DEBUG_DISABLE(name,severity)
#define DEBUG_SET(name,severity)
#define DEBUG_TEST(name,severity)    (0)

#define DBG_LINE(sep)
#define DEBUG(name,severity,fmt,args...)
#define DEBUGP(name,severity,fmt,args...)

#define DBEN(name)
#define DBEX(name)
#define DBTR(name)

#define DBASSERT(name,cond,fmt,args...)  ({if(!(cond)) _assert_handler( __FILE__, __FUNCTION__, __LINE__, #name, #cond, fmt, ##args );})

#else    // DEBUG_LEVEL != 0

extC debug_handler_t _debug_handler;

#define DEBUG_OUTPUT(mod,sev,settings,fmt,args...) _debug_handler(mod,sev,settings,fmt, ##args)

// configuration
// most modules should use DEBUG_MODULE() to create their debugging
// instance. DEBUG_MODULE_S overrides the default debug level
#define DEBUG_USE_MODULE(name)    extern char __dbg_##name
#define DEBUG_MODULE(name)        char __dbg_##name = DEBUG_LEVEL
// dc- if the debug level was forced from the command line, override module defaults
#ifdef MANUAL_DEBUG_LEVEL
#define DEBUG_MODULE_S(name,sev)  char __dbg_##name = MANUAL_DEBUG_LEVEL
#else
#define DEBUG_MODULE_S(name,sev)  char __dbg_##name = sev
#endif

// debug level adjustment
// naturally these must be used in code
#define DEBUG_ENABLE(name, severity)  __dbg_##name |= (severity)
#define DEBUG_DISABLE(name, severity) __dbg_##name &= ~(severity)
#define DEBUG_SET(name, severities)   __dbg_##name = severities
#define DEBUG_TEST(name, severity)   (__dbg_##name & severity)

// general debug statement
// note the ({ }) around it allows it to appear as a single expression
#define DEBUG(name,sev,fmt,args...) \
   ({if(__dbg_##name & sev) { \
       DEBUG_OUTPUT(#name,sev,__dbg_##name," %s:%d (in %s): "fmt, __FILE__,__LINE__,__FUNCTION__,##args); }})

// debug statement without header (pure)
#define DEBUGP(name,sev,fmt,args...) \
   ({if(__dbg_##name & sev) { DEBUG_OUTPUT(#name,sev,__dbg_##name,fmt, ##args); }})

// utility macro
#define DBG_LINE(mod,sev,suff) DEBUG_OUTPUT(#mod,sev,__dbg_##mod," %s:%d (in %s)%s", __FILE__,__LINE__,__FUNCTION__,suff)

// trace style debug statements
#define DBEN(name) if(__dbg_##name & DBGLEV_TRACE) DBG_LINE(name,DBGLEV_TRACE,": enter\n")
#define DBEX(name) if(__dbg_##name & DBGLEV_TRACE) DBG_LINE(name,DBGLEV_TRACE,": exit\n")
#define DBTR(name) if(__dbg_##name & DBGLEV_TRACE) DBG_LINE(name,DBGLEV_TRACE,": trace\n");

#define DBASSERT(name,cond,fmt,args...)                 \
    ({if((__dbg_##name & DBGLEV_ASSERT) && !(cond)) {   \
        _assert_handler( __FILE__, __FUNCTION__, __LINE__, #name, #cond, fmt, ##args );   \
      }})

#endif // DEBUG_LEVEL==0

// Dont namespace pollute
#undef extC

#endif // __DEBUG_H__
