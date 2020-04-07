//  Copyright (C) 1996-2001 Jean-Claude Wippler <jcw@equi4.com>

/** @file
 * Configuration header for GNU C++
 */

#define q4_GNUC 1

#if !defined (q4_BOOL)
#define q4_BOOL 1
#endif

#if defined (__DHARMA)
#define q4_DHARMA 1
#define q4_MULTI 1
#define q4_TINY 1
#define q4_CHECK 1

#if q4_CHECK

#include <util/debug/debug.h>

DEBUG_USE_MODULE(METAKIT);

#undef d4_assert
#define d4_assert(x) DBASSERT(METAKIT, x, #x)

#endif  // q4_CHECK

#endif  // __DHARMA
