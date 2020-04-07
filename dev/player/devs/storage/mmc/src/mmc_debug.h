#ifndef __MMC_DEBUG_H__
#define __MMC_DEBUG_H__

#include <cyg/infra/diag.h>

//#define _DEBUG_

#ifdef _DEBUG_
#define MDEBUG(s...) diag_printf(##s)
#define MENTER()     diag_printf("+%s\n", __FUNCTION__)
#define MEXIT()      diag_printf("-%s\n", __FUNCTION__)
#else
#define MDEBUG(s...)
#define MENTER()
#define MEXIT()
#endif
#undef _DEBUG_
#endif // __MMC_DEBUG_H__
