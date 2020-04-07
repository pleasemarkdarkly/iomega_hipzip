# default.dcl: default configuration for MMC on dharma
# danc@iobjects.com 6/06/01

name mmc
type storage

requires storage_io

compile dev_mmc.c mmc_api.c mmc_fast_c0.S mmc_fast_c1.S mmc_lowl.c mmc_proto.c mmc_slow_c0.c mmc_slow_c1.c mmc_util.c

build_flags -DENABLE_MMC -DENABLE_MMC_DRIVES=2

tests mmctest.c regress.c usb_test.c

####
## Headers
####

#
# _mmc_dev.h
#
header _mmc_dev.h start

// make the eCos device layer thread safe
#define DEV_MMC_THREADSAFE

#define BLOCK_DEV_HDA_NAME  "/dev/hda/"

#if ENABLE_MMC_DRIVES==2
#define BLOCK_DEV_HDB_NAME  "/dev/hdb/"
#endif // ENABLE_MMC_DRIVES==2

#define BLOCK_DEV_NUM_DRIVES ENABLE_MMC_DRIVES

header _mmc_dev.h end


#
# _mmc_hw.h
#
header _mmc_hw.h start
// this header must be ASM safe.

// included for port definitions
#include <cyg/hal/hal_edb7xxx.h>

// internal define for number of available drives
#define HW_MMC_NUM_DRIVES  ENABLE_MMC_DRIVES

// media change support
#define HW_MMC_MEDIA_CHANGE_IRQ
#define HW_MMC_CD_INT      CYGNUM_HAL_INTERRUPT_EINT3

// controller 0 definitions
#define HW_MMC_HARD_RESET_PORT0  PBDR
#define HW_MMC_HARD_RESET_DDR0   PBDDR
#define HW_MMC_HARD_RESET_PIN0   0x04
#define HW_MMC_PORT0             PBDR
#define HW_MMC_DDR0              PBDDR
#define HW_MMC_CLK0              0x10
#define HW_MMC_CMD0              0x20
#define HW_MMC_DATA0             0x40
#define HW_MMC_CARD0             0x80

// controller 1 definitions
#define HW_MMC_HARD_RESET_PORT1  PBDR
#define HW_MMC_HARD_RESET_DDR1   PBDDR
#define HW_MMC_HARD_RESET_PIN1   0x04
#define HW_MMC_PORT1             PDDR
#define HW_MMC_DDR1              PDDDR
#define HW_MMC_CLK1              0x01
#define HW_MMC_CMD1              0x02
#define HW_MMC_DATA1             0x04
#define HW_MMC_CARD1             0x08

header _mmc_hw.h end


