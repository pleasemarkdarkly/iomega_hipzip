# all.dcl: all configuration for ata on dharma (ide bus + cf)
# danc@iobjects.com 6/06/01

include common.dcl

build_flags -DENABLE_ATA_DRIVES=3

tests carddetect_test.cpp

link all.a

####
# Headers
####

#
# _ata_hw.h
#

header _ata_hw.h start

// included for interrupt definition
#include <cyg/hal/hal_edb7xxx.h>

#include <pkgconf/system.h>

// included for device names
#include "_ata_dev.h"

// bus definitions

//
// ATA Bus 0 (IDE chain)
//
#define HW_ATA_ENABLE_BUS_0

// irq used for data xfer
#define HW_ATA_BUS_0_IRQ           CYGNUM_HAL_INTERRUPT_EINT2

// base address for access
#define HW_ATA_BUS_0_BASE          0x50000000

// register map
#define HW_ATA_BUS_0_REGISTERS     0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0e

// memcfg info
#define HW_ATA_BUS_0_MEMCFG_REG           MEMCFG2   // memcfg 2
#define HW_ATA_BUS_0_MEMCFG_OFFSET        8         // second byte
#define HW_ATA_BUS_0_MEMCFG_WAITSTATES    6         // 90mHz -> 6 wait states
#define HW_ATA_BUS_0_MEMCFG_WIDTH         2         // 0 = 32 bit, 1 = 16 bit, 2 = 8 bit
#define HW_ATA_BUS_0_MEMCFG_CLKENB        1         // 0 = disable clk, 1 = enable clk

// the PLD is hooked up to the cirrus processor in such a way
// that accesses to the (16 bit) data register require the bus
// be configured in 16 bit mode, whereas accesses to all other
// registers require the bus be in 8 bit mode. WIDTH_HACK
// just causes the bus width to be changed to 16 bit on data
// commands (hence the above _WIDTH setting should be 8 bit)
#define HW_ATA_BUS_0_MEMCFG_WIDTH_HACK
//#define HW_ATA_BUS_0_HARD_RESET_FUNC		  pld_hard_reset

// sector sizes for devices on this bus
// note that this must match the physical connections on the
// ide bus
#define HW_ATA_BUS_0_MASTER
#define HW_ATA_BUS_0_MASTER_ATAPI
#define HW_ATA_BUS_0_MASTER_NAME          BLOCK_DEV_CDA_NAME
#define HW_ATA_BUS_0_MASTER_SECTOR_SIZE   2048

#define HW_ATA_BUS_0_SLAVE
#define HW_ATA_BUS_0_SLAVE_ATA
#define HW_ATA_BUS_0_SLAVE_NAME           BLOCK_DEV_HDA_NAME
#define HW_ATA_BUS_0_SLAVE_SECTOR_SIZE   512


//
// ATA Bus 1 (memory module)
//
#define HW_ATA_ENABLE_BUS_1

// data irq for bus 1
#define HW_ATA_BUS_1_IRQ            CYGNUM_HAL_INTERRUPT_EINT3

// media change irq for bus 1
#ifndef CYGPKG_IO_USB
//#define HW_ATA_BUS_1_USE_CD_IRQ
#endif

#define HW_ATA_BUS_1_CD_IRQ         CYGNUM_HAL_INTERRUPT_EINT1
#define HW_ATA_BUS_1_CD_PORT        PDDR
#define HW_ATA_BUS_1_CD_DDR         PDDDR
#define HW_ATA_BUS_1_CD_PIN         0x04

#define HW_ATA_BUS_1_BASE           0xf0000000

// register map (note that proc a0 is tied to cf a5)
#define HW_ATA_BUS_1_REGISTERS      0x10,0x30,0x12,0x32,0x14,0x34,0x16,0x36,0x0e


// memcfg info
#define HW_ATA_BUS_1_MEMCFG_REG           MEMCFG1   // memcfg 2
#define HW_ATA_BUS_1_MEMCFG_OFFSET        8         // second byte
#define HW_ATA_BUS_1_MEMCFG_WAITSTATES    8         // 90mHz -> 3 wait states
#define HW_ATA_BUS_1_MEMCFG_WIDTH         1         // 0 = 32 bit, 1 = 16 bit, 2 = 8 bit
#define HW_ATA_BUS_1_MEMCFG_CLKENB        1         // 0 = disable clk, 1 = enable clk

//#define HW_ATA_BUS_1_MEMCFG_WIDTH_HACK
#define HW_ATA_BUS_1_HARD_RESET_FUNC		  cf_hard_reset
#define HW_ATA_BUS_1_HARD_RESET_PIN        0x10
#define HW_ATA_BUS_1_HARD_RESET_PORT       PBDR

// sector sizes for devices on this bus
#define HW_ATA_BUS_1_MASTER
#define HW_ATA_BUS_1_MASTER_ATA
#define HW_ATA_BUS_1_MASTER_NAME         BLOCK_DEV_HDB_NAME
#define HW_ATA_BUS_1_MASTER_SECTOR_SIZE  512


header _ata_hw.h end

