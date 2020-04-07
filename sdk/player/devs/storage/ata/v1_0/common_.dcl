# common.dcl: dcl include with common parameters for ata driver

name ata
type storage

requires storage_io debug_util

export atadrv.h

build_flags -DENABLE_ATA

tests blkdrv_test.cpp rwc_test.cpp usb_test.c align_test.cpp copyc_test.cpp

####
# Headers
####

#
# _ata_dev.h
#

header _ata_dev.h start

// this controls the structure of the driver, and also
// which drive names are available to internal devices

#define DEV_ATA_THREADSAFE

#define BLOCK_DEV_HDA_NAME "/dev/hda/"
#define BLOCK_DEV_HDB_NAME "/dev/hdb/"
#define BLOCK_DEV_CDA_NAME "/dev/cda/"
#define BLOCK_DEV_CDB_NAME "/dev/cdb/"

#define BLOCK_DEV_NUM_DRIVES ENABLE_ATA_DRIVES

header _ata_dev.h end
