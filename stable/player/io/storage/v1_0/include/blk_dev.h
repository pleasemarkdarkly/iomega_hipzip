//
// blk_dev.h
//
// Copyright (c) 1998 - 2001 Interactive Objects (TM). All rights reserved
//
// This file was distributed as part of the Dadio (TM) Software Development Kit
//     under the Dadio (TM) Software Development Kit license. Please see the file
//     "Dadio Software Development Kit license agreement.pdf" contained in the
//     "Legal" folder on the Dadio Distribution CD.
//

//@{

#ifndef __BLK_DEV_H__
#define __BLK_DEV_H__

#include <cyg/io/io.h>
#include <cyg/hal/drv_api.h>
#include <cyg/infra/cyg_type.h>

//! This structure is used in our block device interface to specify
//! information about a physical disk. In situations where the physical
//! layout of a device does not match the C/H/S style hard drive layout,
//! it is acceptable to specify the device as having 1 cylinder, 1 head,
//! and a number of sectors equal to the total blocks.
typedef struct drive_geometry_s
{
    //! Number of cylinders
    cyg_uint16  cyl;
    //! Number of heads
    cyg_uint16  hd;
    //! Number of sectors
    cyg_uint16  sec;
    //! Bytes per sector
    cyg_uint16  bytes_p_sec;
    //! Total blocks (C*H*S)
    cyg_uint32  num_blks;
    
    //! Length of the serial number in bytes
    cyg_uint32      serial_len;
    //! Serial number for the drive
    unsigned char   serial_num[64];
    
    //! Length of the model number in bytes
    cyg_uint32      model_len;
    //! Model number for the drive
    unsigned char   model_num[64];
} drive_geometry_t;

/* TODO these defines should be moved to cyg/io/config_keys.h */
#define IO_BLK_GET_CONFIG_GEOMETRY           0x0301 /* buf = (drive_geometry_t *) */
#define IO_BLK_GET_CONFIG_MEDIA_STATUS       0x0302 /* buf = (int*)             */
#define IO_BLK_GET_CONFIG_LBA_LAST_ERROR     0x0303 /* buf = (unsigned int *) */

#define IO_BLK_SET_CONFIG_EXEC_COMMAND       0x0381 /* buf = (ATAPIPacket_T *) */
#define IO_BLK_SET_CONFIG_RESET              0x0382
#define IO_BLK_SET_CONFIG_SLEEP              0x0383
#define IO_BLK_SET_CONFIG_WAKEUP             0x0384
#define IO_BLK_SET_CONFIG_POWER_DOWN         0x0385
#define IO_BLK_SET_CONFIG_POWER_UP           0x0386
#define IO_BLK_SET_CONFIG_MEDIA_STATUS_CB    0x0387
#define IO_BLK_SET_CONFIG_LOOKAHEAD_DISABLE  0x0388

#ifndef IO_PM_SET_CONFIG_REGISER
#define IO_PM_SET_CONFIG_REGISTER       0x0681
#endif
#ifndef IO_PM_SET_CONFIG_UNREGISTER
#define IO_PM_SET_CONFIG_UNREGISTER     0x0682
#endif

/* Block device error codes - these should be standardized */
#define ESLEEP     602
#define ERDIO      603
#define EWRIO      604
#define ETIME      605
#define EPHASE     606
#define EOVRFLW    607
#define EUNDRFLW   608
/* The following are ASC codes */
#define ERESET     0x29
#define EMEDCHG	   0x28
#define ENOMED	   0x3a

// Legacy ecos support
#ifndef BLOCK_DEVTAB_ENTRY

//!\if doxygen_should_ignore_this
typedef struct blk_request_s
{
  cyg_uint32 lba;
  cyg_uint32 num_blks;
  void * buf;
} blk_request_t;
//!\endif

#endif

//@}

#endif /* __BLK_DEV_H__ */

