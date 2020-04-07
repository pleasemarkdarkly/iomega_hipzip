#ifndef __PEMINTERFACE_H__
#define __PEMINTERFACE_H__

#include <cyg/kernel/kapi.h>
#include <cyg/infra/diag.h>
#include <cyg/hal/hal_arch.h>

#ifdef __cplusplus
extern "C" {
#endif
#include "layer3.h"
#include "types.h"
#include "param.h"
#include "polyphase.h"
#include "psy.h"
#include "hybrid.h"
#include "code.h"
#include "huffman.h"
#include "tables.h"
#include "out.h"
#ifdef __cplusplus
};
#endif

// DD: used with Phil's GDB stub + audio samples
#define FLASH_SAMPLES_START (char*)0xe0208000
#define FLASH_SAMPLES_LENGTH  705600

#endif //  __PEMINTERFACE_H__
