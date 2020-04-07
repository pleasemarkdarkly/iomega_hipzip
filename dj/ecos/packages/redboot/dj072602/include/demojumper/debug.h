#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <cyg/infra/diag.h>

#define DEBUG_DEMOJUMPER(s...) SHUNT_PRINT(##s)
//#define DEBUG_DEMOJUMPER(s...) /**/

#endif
