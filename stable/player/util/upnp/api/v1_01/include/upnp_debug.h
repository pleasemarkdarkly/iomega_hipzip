#ifndef UPNP_DEBUG_H
#define UPNP_DEBUG_H

#include <util/debug/debug.h>

#if DEBUG_LEVEL != 0
#define DBGONLY(x) x
#else
#define DBGONLY(x)
#endif

#endif
