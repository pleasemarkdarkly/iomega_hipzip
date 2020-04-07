//........................................................................................
//........................................................................................
//.. Copyright:(c) 1995-2000 Interactive Objects Inc.  									..
//..	 All rights reserved. This code may not be redistributed in source or linkable  ..
//.. 	 object form without the express written consent of Interactive Objects.        ..
//.. Contact Information: www.iobjects.com												..
//........................................................................................
//........................................................................................
/*==========================================================================
  File name:  Platform.h

  Description:
    This file is intended to define all the CPU and OS dependent types, as
    well as define higher level macros that describe capabilities. Always
    included from Primal.h

  Notes:
    This file can be included by both C and C++.

  Porting issues:
    New sections must be added for each platform supported
============================================================================
   Original author:  Alan Arndt
     Creation date:  <01 June 1999>
      Archive file:  $Archive$
  Last modified by:  $Author$
  Last modified on:  $Date$
============================================================================
  This software is provided pursuant to an agreement with InterTrust
  Technologies Corporation ("InterTrust"). This software may be used only
  as expressly allowed in such agreement.

  Copyright (c) 1999 by InterTrust. All rights reserved.
==========================================================================*/
#if !defined(Platform_H)
#define Platform_H

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Determine the platform & define platform host type
 *
 *  IT_HOST_PLATFORM_WIN32
 *  IT_HOST_PLATFORM_WINCE
 *  IT_HOST_PLATFORM_PALMOS
 *  IT_HOST_PLATFORM_MACOS
 *  IT_HOST_PLATFORM_UNIX
 *  IT_HOST_PLATFORM_SOLARIS
 *  IT_HOST_PLATFORM_LINUX
 *  IT_HOST_PLATFORM_EPOC32
 *  IT_HOST_PLATFORM_EP7211
 */

//#define IT_HOST_PLATFORM_EP7211
    
#if defined(__WINS__)
#   define IT_HOST_PLATFORM_EPOC32       1

#elif (defined(_WIN32) && !defined(_WIN32_WCE))
#   define IT_HOST_PLATFORM_WIN32        1

#elif defined(unix)
#   define IT_HOST_PLATFORM_UNIX         1
#   if defined(sun)
#       define IT_HOST_PLATFORM_SOLARIS  1
#   elif (defined(__LINUX__) || defined(__linux__))
#       define IT_HOST_PLATFORM_LINUX    1
#   else
#       error "Unsupported Unix Platform"
#   endif

#elif defined(_WIN32_WCE)
#   define IT_HOST_PLATFORM_WINCE        1

#elif defined(__PALMOS_TRAPS__)
#   define IT_HOST_PLATFORM_PALMOS       1

#elif defined(TARGET_OS_MAC)
#   define IT_HOST_PLATFORM_MACOS        1

#elif defined(__EPOC32__)
#   define IT_HOST_PLATFORM_EPOC32       1

#elif defined(__arm)
#   define IT_HOST_PLATFORM_EP7211       1

#elif (defined(_TMS320C5XX) || defined(_TMS320C6X))
#   define IT_HOST_PLATFORM_TMS320           1
#else
#   error "Unsupported Platform"
#endif

/*
 * Determine architecture type setting one the following:
 *
 *  IT_HOST_ARCHITECTURE_X86
 *  IT_HOST_ARCHITECTURE_PPC1
 *  IT_HOST_ARCHITECTURE_MIPS
 *  IT_HOST_ARCHITECTURE_SH
 *  IT_HOST_ARCHITECTURE_ARM
 *  IT_HOST_ARCHITECTURE_SPARC
 *  IT_HOST_ARCHITECTURE_ALPHA
 *  IT_HOST_ARCHITECTURE_M68K
 *  IT_HOST_ARCHITECTURE_TMS320
 */
#if (defined(i386) || defined(_M_IX86)) && !defined(__WINS__)
#   define IT_HOST_ARCHITECTURE_X86      1

#elif defined(sparc)
#   define IT_HOST_ARCHITECTURE_SPARC    1

#elif defined(alpha) || defined(_M_ALPHA)
#   define IT_HOST_ARCHITECTURE_ALPHA    1

#elif defined(ppc) || defined(_M_PPC) || defined(__POWERPC__)
#   define IT_HOST_ARCHITECTURE_PPC1     1

#elif defined (__PALMOS_TRAPS__)
#   define IT_HOST_ARCHITECTURE_M68K     1

#elif defined(_MIPS_)
#   define IT_HOST_ARCHITECTURE_MIPS     1

#elif defined(__EPOC32__) || defined(__arm) || defined(__WINS__)
#   define IT_HOST_ARCHITECTURE_ARM      1

#elif defined(_SH3_)
#   define IT_HOST_ARCHITECTURE_SH       1 
    
#elif (defined(_TMS320C5XX) || defined(_TMS320C6X))
#   define IT_HOST_ARCHITECTURE_TMS320   1

#else
#   error "Unsupported Architecture"
#endif


/*
 * determine platform-specific information setting the following:
 */
#if defined(IT_HOST_PLATFORM_WIN32)

    /* turn off a bunch of windows.h stuff we do not need, & disable Microsoft warnings */
#   pragma warning(disable : 4115)
#   define WIN32_LEAN_AND_MEAN
#   define NOATOM
#   define NOGDI
#   define NOGDICAPMASKS
#   define NOMETAFILE
#   define NOMINMAX
#   define NOMSG
#   define NOOPENFILE
#   define NORASTEROPS
#   define NOSCROLL
#   define NOSOUND
#   define NOSYSMETRICS
#   define NOTEXTMETRIC
#   define NOWH
#   define NOCOMM
#   define NOKANJI
#   define NOCRYPT
#   define NOMCX
#   include <windows.h>
#   pragma warning(default : 4115)

#   include <stdarg.h>
#   include <stdio.h>
#   include <limits.h>
#   include <string.h>
#   include <assert.h>
#   include <stdlib.h>

    typedef char               Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef __int64            Int64;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
    typedef unsigned __int64   Uint64;

    typedef unsigned char      Byte;
    
#   if defined(__cplusplus)
        typedef bool           Bool;
#   else
        typedef int            Bool;
#   endif
#   define TRUE               1
#   define FALSE              0

#   define BYTE_FORMAT        "%c"
#   define INT16_FORMAT       "%d"
#   define INT32_FORMAT       "%ld"
#   define INT64_FORMAT       "%I64d"
#   define INT64_XFORMAT      "%I64X"
#   define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE
        
#   pragma warning(disable : 4127)

#elif defined(IT_HOST_PLATFORM_WINCE)
#   if defined(IT_HOST_ARCHITECTURE_ARM)      /* Perhaps this should be reversed to check only those platforms that are KNOWN to work */
#       error "ARM NOT SUPPORTED BY WINDOWS CE"
#   endif

#   include <limits.h>
#   include <string.h>
#   include <windows.h>
#   include <stdarg.h>
#   include <stdlib.h>

    /* We need to have a FILE defined, but it is never really used. */
#   if !defined(stderr)
#       define FILE   void
#       define stderr (void*)0
        void fprintf(FILE* f, const char* fmt, ...);
#   endif
#   define assert(exp) ((void)0)

    typedef char               Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef __int64            Int64;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
    typedef unsigned __int64   Uint64;

    typedef unsigned char      Byte;
#   if defined(__cplusplus)
        typedef bool           Bool;
#   else
        typedef int            Bool;
#   endif
#   define TRUE               1
#   define FALSE              0

#   define BYTE_FORMAT        "%c"
#   define INT16_FORMAT       "%d"
#   define INT32_FORMAT       "%ld"
#   define INT64_FORMAT       "%I64d"
#   define INT64_XFORMAT      "%I64X"
    //#define IT_HAS_FPRINTF     TRUE

#   define IT_HAS_STREXCEPT   TRUE
    //#define IT_HAS_SETJMP      FALSE

#elif defined(IT_HOST_PLATFORM_EPOC32)
#   include <ASSERT.H>
#   include <LIMITS.H>
#   include <STDLIB.H>
#   include <STDIO.H>
#   include <STRING.H>
#   include <STDDEF.H>     /* definition of size_t */
#   include <STDARG.H>

    typedef char               Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
#   define IT_64_SIM

    typedef unsigned char      Byte;
    typedef int                Bool;
#   define TRUE               1
#   define FALSE              0

#   define BYTE_FORMAT        "%c"
#   define INT16_FORMAT       "%d"
#   define INT32_FORMAT       "%ld"
#   define INT64_FORMAT       "%I64d"
#   define INT64_XFORMAT      "%I64X"
    //#define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE

#elif defined(IT_HOST_PLATFORM_EP7211)
#   include <stdlib.h>
#   include <stdio.h>
  //#   include <limits.h>

    typedef signed char        Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef long long          Int64;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
    typedef unsigned long long Uint64;

// Dadio OS by Interactive Objects, Inc.
/*
#include <cyg/infra/cyg_type.h>
	
	typedef Int8				cyg_int8;
    typedef Int16				cyg_int16;
    typedef Int32				cyg_int32;
    typedef Int64				cyg_int64;
    typedef Uint16				cyg_uint16;
    typedef Uint32				cyg_uint32;
    typedef Uint64				cyg_uint64;
*/
    typedef unsigned char      Byte;
    typedef int                Bool;
#   define TRUE               1
#   define FALSE              0

#   define TINY_HEAP
#   define assert(exp) ((void)0)
    //#define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE

#elif defined(IT_HOST_PLATFORM_TMS320)
#   include <stdlib.h>
#   include <limits.h>
#   include <assert.h>
#   include <stdio.h>    
    typedef signed char        Int8;
    typedef short              Int16;
    typedef long               Int32;
    typedef unsigned int       Uint16;
    typedef unsigned long      Uint32;
      
    typedef unsigned char      Byte;
    typedef int                Bool;
#   define TRUE               1
#   define FALSE              0

#   define IT_64_SIM
#   define TINY_HEAP

#elif defined(IT_HOST_PLATFORM_PALMOS)

#   include "PalmStdIO.h"
#   define IGNORE_STDIO_STUBS
#   define stderr 0
#   define stdout 0
    typedef unsigned char    FILE ;

#   include <PalmOS.h>
#   include <string.h>
#   include <sys_types.h>
#   include <unix_stdlib.h>
#   include <unix_stdarg.h>
#   include <unix_string.h>
#   include "PalmLimits.h"

    /*typedef char               Int8;  Already defined in PalmOS */
    /*typedef int                Int16;    Already defined in PalmOS */
    /*typedef long               Int32;    Already defined in PalmOS */
    typedef long long          Int64;
    typedef unsigned int       Uint16;
    typedef unsigned long      Uint32;
    typedef unsigned long long Uint64;

    typedef unsigned char      Byte;
    typedef     int            Bool;

    
#   define     TRUE           true    /* true & false already defined */
#   define     FALSE          false

#   define strncmp(x, y, z) StrNCompare(x, y, z)

    /* This next line can be moved as soon as runtime.c in no longer calls memmove */
#   define memmove(x, y, z)    MemMove(x, y, z)
    
#   define assert(condition) ErrFatalDisplayIf(!(condition), "assertion failed: " # condition);
/*
#       if !defined(_SIZE_T_DEFINED)
#           !defined(__size_t__)
                typedef unsigned long size_t;
#               define _SIZE_T_DEFINED
#           endif
#       endif
*/
#   define BYTE_FORMAT        "%c"
#   define INT16_FORMAT       "%d"
#   define INT32_FORMAT       "%ld"
#   define INT64_FORMAT       "%ul" /*!CQ are these two right??? */
#   define INT64_XFORMAT      "%lx" /*!LV no...but there don't seem to be any*/
    //#define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE

#elif defined(IT_HOST_PLATFORM_MACOS)
    /* Metrowerks CodeWarrior for Apple Macintosh types */
#   include "types.h"            /* Metrowerks CodeWarrior types */
#   include <assert.h>
#   include <limits.h>
#   include <string.h>
#   include <stdlib.h>
#   include "stdio.h"
//#   include "unix.h" - causes type conflict with GUSI

    typedef char               Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef long long          Int64;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
    typedef unsigned long long Uint64;

    /*typedef unsigned char       Byte;        Already defined in CodeWarrior */
    typedef int                 Bool;
#   if !defined(CALLIB_RUN)
#       define TRUE            true
#       define FALSE           false
#   endif

#   define BYTE_FORMAT        "%c"
#   define INT16_FORMAT       "%d"
#   define INT32_FORMAT       "%ld"
#   define INT64_FORMAT       "%lld"
#   define INT64_XFORMAT      "%llX"
#   define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE

#elif defined(IT_HOST_PLATFORM_LINUX)
#   include <assert.h>
#   include <fcntl.h>
#   include <limits.h>
#   include <stdarg.h>
#   include <stdint.h>
#   include <stdio.h>
#   include <stdlib.h>
#   include <string.h>
#   include <sys/stat.h>
#   include <unistd.h>
#   include <errno.h>

#   define _MAX_PATH PATH_MAX
    
    typedef char               Int8;
    typedef short              Int16;
    typedef int                Int32;
    typedef long long          Int64;
    typedef unsigned short     Uint16;
    typedef unsigned int       Uint32;
    typedef unsigned long long Uint64;

    typedef unsigned char      Byte;
    typedef int                Bool;
#   define TRUE (0 == 0)
#   define FALSE (!TRUE)

#   define INT16_FORMAT       "%dh"
#   define INT32_FORMAT       "%d"
#   define INT64_FORMAT       "%lld"
#   define INT64_XFORMAT      "%llX"
#   define IT_HAS_FPRINTF     TRUE

    //#define IT_HAS_STREXCEPT   TRUE
#   define IT_HAS_SETJMP      TRUE
#endif


/* Some architecture specific definitions
 */
#if defined(IT_HOST_ARCHITECTURE_X86)
#   define IT_LITTLE_ENDIAN 1
#   define IT_WORDSIZE 32

#elif defined(IT_HOST_ARCHITECTURE_PPC1)
#   define IT_BIG_ENDIAN 1
#   define IT_WORDSIZE 32

#elif defined(IT_HOST_ARCHITECTURE_MIPS)
#   if IT_HOST_PLATFORM_WINCE
#       define IT_LITTLE_ENDIAN 1
#   else
#       define IT_BIG_ENDIAN 1
#   endif
#   define IT_WORDSIZE 32

#elif defined(IT_HOST_ARCHITECTURE_M68K)
#   define IT_BIG_ENDIAN 1
#   define IT_WORDSIZE 32

#elif defined(IT_HOST_ARCHITECTURE_ARM)
#   define IT_LITTLE_ENDIAN 1
#   define IT_WORDSIZE 32

#elif defined(IT_HOST_ARCHITECTURE_SH)
#   define IT_LITTLE_ENDIAN 1
#   define IT_WORDSIZE 32          

#elif defined(IT_HOST_ARCHITECTURE_TMS320)
#   define IT_BIG_ENDIAN 1
#   define IT_WORDSIZE 32

#endif


/* exact types
 */
#if defined(IT_HOST_PLATFORM_TMS320)
#   define OCTETS_PER_BYTE 2
#   define BYTE_SIZE   1
#   define INT16_SIZE  1
#   define UINT16_SIZE 1
#   define INT32_SIZE  2
#   define UINT32_SIZE 2
#else
#   define OCTETS_PER_BYTE 1
#   define BYTE_SIZE   1
#   define INT16_SIZE  2
#   define UINT16_SIZE 2
#   define INT32_SIZE  4
#   define UINT32_SIZE 4
#endif
    
#if !defined(IT_HOST_PLATFORM_LINUX)
#   define INT16_MAX  0x7FFF
#   define UINT16_MAX 0xFFFF
#   define INT32_MAX  0x7FFFFFFF
#   define UINT32_MAX 0xFFFFFFFF
#   define INT64_MAX  0x7FFFFFFFFFFFFFFF
#   define UINT64_MAX 0xFFFFFFFFFFFFFFFF
#endif
    
/* Error Checking
 */
#if !(defined(IT_LITTLE_ENDIAN) || defined(IT_BIG_ENDIAN))
#   error "Endian type not defined"
#elif defined(IT_LITTLE_ENDIAN) && defined(IT_BIG_ENDIAN)
#   error "BOTH Endian types defined"
#endif


/* If platform doesn't need SIM, then it must have NATIVE support
 */
#if !defined(IT_64_SIM)
#   define IT_64_NATIVE
#endif


#if defined(__cplusplus)
}
#endif
#endif
