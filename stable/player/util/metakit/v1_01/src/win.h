// win.h --
// $Id: win.h,v 1.4 2001/11/03 23:40:01 wcvs Exp $
// This is part of MetaKit, the homepage is http://www.equi4.com/metakit/

/** @file
 * Configuration header for Windows builds
 */

#if defined (_MSDOS)
#define q4_DOS 1
#endif

#if defined (_WINDOWS)
#define q4_WIN 1
#endif

#if defined (_WIN32)
#define q4_WIN32 1
#endif

#if q4_WIN32                    // WIN32 implies WIN
#undef q4_WIN
#define q4_WIN 1
#endif

#if q4_WIN                      // WIN implies not DOS, even for Win3
#undef q4_DOS
#endif
