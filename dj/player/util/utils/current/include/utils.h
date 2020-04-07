// utils.h: commonly used utilities

#ifndef __UTILS_H__
#define __UTILS_H__

#ifdef __cplusplus
#define _extern_c_ extern "C"
#else
#define _extern_c_
#endif

// Return a copy of the passed in string (uses malloc)
_extern_c_ char* strdup( const char* str );

// Return a copy of the passed in string (uses new)
_extern_c_ char* strdup_new( const char* str );

// Return the crc32 of the given buffer
_extern_c_ unsigned int crc32( const unsigned char* buf, unsigned int len );


// hardware lock management
#ifdef NOKERNEL
#define DECLARE_HW_LOCK(abc) ;
#define USE_HW_LOCK(abc) ;
#define HW_LOCK(abc) ;
#define INIT_HW_LOCK(abc) ;
#define HW_UNLOCK(abc) ;
#else

#include <cyg/kernel/kapi.h>

#define DECLARE_HW_LOCK(abc)  cyg_mutex_t __LOCK_##abc;
#define USE_HW_LOCK(abc) extern cyg_mutex_t __LOCK_##abc;
#define INIT_HW_LOCK(abc) cyg_mutex_init(&__LOCK_##abc );
#define HW_LOCK(abc) cyg_mutex_lock(&__LOCK_##abc );
#define HW_UNLOCK(abc) cyg_mutex_unlock(&__LOCK_##abc );
#endif

#undef _extern_c_

#endif // __UTILS_H__
