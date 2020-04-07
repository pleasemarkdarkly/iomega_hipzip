# default.dcl: default configuration for dataplay on dharma

type storage
name dataplay
compile dfs.c dp_hw.c dp_proto.c
export dfs.h dp.h dpi_host.h



####
# Headers
####

#
# _dp_hw.h
#

header _dp_hw.h start

#include <cyg/hal/hal_edb7xxx.h>

// MEMCFG settings
#define DP_MEMCFG_REG    (MEMCFG1)
#define DP_MEMCFG_MASK   (0x0000ff00)
#define DP_MEMCFG_VAL    (0x00009e00)

// Device interrupt
#define DP_INTERRUPT     CYGNUM_HAL_INTERRUPT_EINT1

// Hardware base address
#define DP_BASE_ADDRESS  (0xf0000000)

header _dp_hw.h end

