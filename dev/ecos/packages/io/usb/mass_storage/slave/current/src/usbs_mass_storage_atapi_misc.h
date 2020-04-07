#ifndef CYGONCE_USBS_MASS_STORAGE_ATAPI_MISC_H_
#define  CYGONCE_USBS_MASS_STORAGE_ATAPI_MISC_H_
//==========================================================================
//
//      usbs_mass_storage_atapi_misc.h
//
//      Support for USB-mass storage ATAPI devices, slave-side.
//
//==========================================================================
//####COPYRIGHTBEGIN####
//                                                                          
// -------------------------------------------                              
// -------------------------------------------                              
//                                                                          
//####COPYRIGHTEND####
//==========================================================================
//#####DESCRIPTIONBEGIN####
//
// Author(s):    toddm@iobjects.com
// Contributors: toddm@iobjects.com
// Date:         2001-21-02
//
//####DESCRIPTIONEND####
//
//==========================================================================

#include <cyg/kernel/kapi.h>
#include <cyg/hal/hal_edb7xxx.h>

// TODO Get these from the right place
#define USE_IRQ
//#define USE_MEMORY_MODULE
//#define USE_CLIK
#define ETIME  605
#define EPHASE 606

#if defined(USE_MEMORY_MODULE)
#define ATA_IRQ CYGNUM_HAL_INTERRUPT_EINT3
#define CF_CARD_DETECT_IRQ CYGNUM_HAL_INTERRUPT_EINT1
#else /* USE_MEMORY_MODULE */
#define ATA_IRQ CYGNUM_HAL_INTERRUPT_EINT2
#endif /* USE_MEMORY_MODULE */

#define USE_MACROS 0
#define MAX_RETRIES 5

#define ATA_MEMCFG_MASK 0x0000ff00

/* Values to play with */
#define MEMCFG_BUS_WIDTH(n)   (n<<0)
#define MEMCFG_BUS_WIDTH_32   (0<<0)
#define MEMCFG_BUS_WIDTH_16   (1<<0)
#define MEMCFG_BUS_WIDTH_8    (2<<0)
#define MEMCFG_WAIT_STATES(n) (n<<2)
#define MEMCFG_SQAEN          (1<<6)
#define MEMCFG_CLKENB         (1<<7)

#define WAIT_STATES 8

#define ATA_8BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_8)<<8)
#define ATA_16BIT_MEMCFG ((MEMCFG_CLKENB|MEMCFG_WAIT_STATES((8-WAIT_STATES))|MEMCFG_BUS_WIDTH_16)<<8)

/* ATA task file register addresses */
#if defined(USE_MEMORY_MODULE)
#define ATA_DATA_REG        0xf0000010
#define ATA_ERR_REG         0xf0000030
#define ATA_FEATURES_REG    ATA_ERR_REG
#define ATA_SEC_CNT_REG     0xf0000012
#define ATA_SEC_NUM_REG     0xf0000032
#define ATA_CYL_LOW_REG     0xf0000014
#define ATA_CYL_HI_REG      0xf0000034
#define ATA_DEVICE_HEAD_REG 0xf0000016
#define ATA_STATUS_REG      0xf0000036
#define ATA_COMMAND_REG     ATA_STATUS_REG
#define ATA_ALT_STATUS_REG  0xf000000e
#define ATA_DEVICE_CTL_REG  ATA_ALT_STATUS_REG
#define ATA_DRIVE_ADDR_REG  0xf000002e
#else /* USE_MEMORY_MODULE */
#define ATA_DATA_REG        0x50000000
#define ATA_ERR_REG         0x50000001
#define ATA_FEATURES_REG    ATA_ERR_REG
#define ATA_SEC_CNT_REG     0x50000002
#define ATA_SEC_NUM_REG     0x50000003
#define ATA_CYL_LOW_REG     0x50000004
#define ATA_CYL_HI_REG      0x50000005
#define ATA_DEVICE_HEAD_REG 0x50000006
#define ATA_STATUS_REG      0x50000007
#define ATA_COMMAND_REG     ATA_STATUS_REG
#define ATA_DUP_ERR_REG     0x5000000d /* TODO This is only in the CF spec */
#define ATA_DUP_FEATURES_REG ATA_DUP_ERR_REG
#define ATA_ALT_STATUS_REG  0x5000000e
#define ATA_DEVICE_CTL_REG  ATA_ALT_STATUS_REG
#endif /* USE_MEMORY_MODULE */

/* Feature Register */
#define FEATURE_NODMA 0x00
#define FEATURE_DMA   0x01

/* Drive Select Register */
#define SELECT_MASTER  0x00
#define SELECT_SLAVE 0x10
#define SELECT_LBA 0x40

/* Error Register */
#define ERROR_WP   0x40
#define ERROR_UNC  0x40
#define ERROR_MC   0x20
#define ERROR_IDNF 0x10
#define ERROR_MCR  0x08
#define ERROR_ABRT 0x04
#define ERROR_NM   0x02

/* Error Register for EXECUTE DRIVE DIAGNOSTIC */
#define NO_ERROR_DETECTED 0x01
/* NOTE the following entries may only be valid for CF */
#define FORMATTER_DEVICE_ERROR 0x02
#define SECTOR_BUFFER_ERROR 0x03
#define ECC_CIRCUITRY_ERROR 0x04
#define CONTROLLING_uP_ERROR 0x05

/* Status Register */
#define STATUS_BSY  0x80
#define STATUS_DRDY 0x40
#define STATUS_DRQ  0x08
#define STATUS_ERR  0x01
#define STATUS_SERV 0x10

/* Interrupt Reason Register */
#define INTERRUPT_COMMAND   0x01
#define INTERRUPT_IO        0x02
#define INTERRUPT_REL       0x04

/* Command Register */
#define COMMAND_DEVICE_RESET  0x08
#define COMMAND_ATAPI_PACKET  0xA0
#define COMMAND_ATAPI_ID      0xA1
#define COMMAND_SLEEP         0xE6

/* Control Register */
#define CONTROL_IRQ           0x00
#define CONTROL_POLLED        0x02
#define CONTROL_SRST          0x04

/* ATA Commands */
#define ATA_DEVICE_RESET              0x08
#define ATA_EXECUTE_DEVICE_DIAGNOSTIC 0x90
#define ATA_IDENTIFY_DEVICE           0xec
#define ATA_MEDIA_EJECT               0xed
#define ATA_IDENTIFY_PACKET_DEVICE    0xa1
#define ATA_PACKET                    0xa0
#define ATA_WRITE_SECTORS             0x30
#define ATA_READ_SECTORS              0x20
#define ATA_CHECK_POWER_MODE          0xe5

/* ATA I/O macros */
#define _ReadReg8(reg,var)             \
        var = *((volatile cyg_uint8 *)reg)

#define _WriteReg8(reg,val)            \
        *((volatile cyg_uint8 *)reg) = (val&0xff)

#define _ReadReg16(reg,var)            \
        var = *((volatile cyg_uint16 *)reg)

#define _WriteReg16(reg,val)           \
        *((volatile cyg_uint16 *)reg) = (val&0xffff)

#if USE_MACROS
#define _ReadDataBulk(address,count)   \
      {                               \
        int i;                        \
        for(i=0;i<count;i++)        \
          ((volatile cyg_uint16 *)address)[i]=*((volatile cyg_uint16 *)ATA_DATA_REG);  \
      }

#define _WriteDataBulk(address, count) \
      {                               \
	  int i;                      \
	  for(i=0;i<count;i++)        \
            *((volatile cyg_uint16 *)ATA_DATA_REG) = ((volatile cyg_uint16 *)address)[i];   \
      }
#else /* USE_MACROS */
void _ReadDataBulk(cyg_uint16 * Buffer, int Length);
void _WriteDataBulk(cyg_uint16 * Buffer, int Length);
#endif /* USE_MACROS */

#define _ReadError(Variable) _ReadReg8(ATA_ERR_REG,Variable)
#define _ReadSectorCount(Variable) _ReadReg8(ATA_SEC_CNT_REG,Variable)
#define _ReadSectorNumber(Variable) _ReadReg8(ATA_SEC_NUM_REG,Variable)
#define _ReadCylinderLow(Variable) _ReadReg8(ATA_CYL_LOW_REG,Variable)
#define _ReadCylinderHigh(Variable) _ReadReg8(ATA_CYL_HI_REG,Variable)
#define _ReadDeviceHead(Variable) _ReadReg8(ATA_DEVICE_HEAD_REG,Variable)
#define _ReadStatus(Variable) _ReadReg8(ATA_STATUS_REG,Variable)
#define _ReadAlternateStatus(Variable) _ReadReg8(ATA_ALT_STATUS_REG,Variable)

#define _WriteData(Variable) _WriteReg8(ATA_DATA_REG,Variable)
#define _WriteFeatures(Variable) _WriteReg8(ATA_FEATURES_REG,Variable)
#define _WriteSectorCount(Variable) _WriteReg8(ATA_SEC_CNT_REG,Variable)
#define _WriteSectorNumber(Variable) _WriteReg8(ATA_SEC_NUM_REG,Variable)
#define _WriteCylinderLow(Variable) _WriteReg8(ATA_CYL_LOW_REG,Variable)
#define _WriteCylinderHigh(Variable) _WriteReg8(ATA_CYL_HI_REG,Variable)
#define _WriteDeviceHead(Variable) _WriteReg8(ATA_DEVICE_HEAD_REG,Variable)
#define _WriteCommand(Variable) _WriteReg8(ATA_COMMAND_REG,Variable)
#define _WriteDeviceControl(Variable) _WriteReg8(ATA_DEVICE_CTL_REG,Variable)

#if defined(USE_MICRODRIVE)
#define TIMEOUT_5S 750
#else /* USE_MICRODRIVE */
#define TIMEOUT_5S 500
#endif /* USE_MICRODRIVE */
#define TIMEOUT_1S 100

#endif // CYGONCE_USBS_MASS_STORAGE_ATAPI_MISC_H_
