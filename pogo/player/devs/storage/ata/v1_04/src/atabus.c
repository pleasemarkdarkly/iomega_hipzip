// atabus.c: bus structure definitions
// danc@iobjects.com 07/03/01
// (c) Interactive Objects

#include <cyg/hal/hal_edb7xxx.h>

#include <devs/storage/ata/_ata_hw.h>
#include "atabus.h"
#include "busctrl.h"

//
// local functions
//

// Bus0 definition
#if defined(HW_ATA_ENABLE_BUS_0)

#if defined(HW_ATA_BUS_0_MASTER)
static Partition_T Bus0_Master_Partition =
{
  SectorSize: HW_ATA_BUS_0_MASTER_SECTOR_SIZE,
};
#endif // HW_ATA_BUS_0_MASTER

#if defined(HW_ATA_BUS_0_SLAVE)
static Partition_T Bus0_Slave_Partition =
{
  SectorSize: HW_ATA_BUS_0_SLAVE_SECTOR_SIZE,
};
#endif // HW_ATA_BUS_0_SLAVE

#if defined(HW_ATA_BUS_0_HARD_RESET_FUNC)
// prototype it
extern void HW_ATA_BUS_0_HARD_RESET_FUNC ( BusSpace_T* );
#endif

// Bus0 defintion
BusSpace_T Bus0 =
{
  Init:  BusInit,
  Reset: BusReset,
#if defined(HW_ATA_BUS_0_HARD_RESET_FUNC)
  HardReset: HW_ATA_BUS_0_HARD_RESET_FUNC,
#else
  HardReset: 0,
#endif
  PowerDown: BusPowerDown,
  PowerUp:   BusPowerUp,

#if defined(HW_ATA_BUS_0_MEMCFG_WIDTH_HACK)
  Write16: BusWrite16M,
  Write16Multiple: BusWrite16MultipleM,
  Read16Multiple: BusRead16MultipleM,
#else
  Write16: BusWrite16,
  Write16Multiple: BusWrite16Multiple,
  Read16Multiple: BusRead16Multiple,
#endif  // HW_ATA_BUS_0_MEMCFG_WIDTH_HACK
  
  Base:             HW_ATA_BUS_0_BASE,
  DataReg:          HW_ATA_BUS_0_REGISTERS,
#if 0
  ErrorReg:         HW_ATA_BUS_0_REGISTERS[1],
  SectorCountReg:   HW_ATA_BUS_0_REGISTERS[2],
  SectorNumberReg:  HW_ATA_BUS_0_REGISTERS[3],
  CylinderLowReg:   HW_ATA_BUS_0_REGISTERS[4],
  CylinderHighReg:  HW_ATA_BUS_0_REGISTERS[5],
  DeviceHeadReg:    HW_ATA_BUS_0_REGISTERS[6],
  CommandReg:       HW_ATA_BUS_0_REGISTERS[7],
  ControlReg:       HW_ATA_BUS_0_REGISTERS[8],
#endif
  
#ifndef NOKERNEL
  IRQ:  HW_ATA_BUS_0_IRQ,
  ISR:  BusInterruptISR,
  DSR:  BusInterruptDSR,
#endif

  MemcfgRegister: (volatile unsigned int*) HW_ATA_BUS_0_MEMCFG_REG,
  MemcfgMask: (0xff << HW_ATA_BUS_0_MEMCFG_OFFSET),
  MemcfgDefaultSetting: (MEMCFG_CLKENB(HW_ATA_BUS_0_MEMCFG_CLKENB)            |
			 MEMCFG_WAIT_STATES(8-HW_ATA_BUS_0_MEMCFG_WAITSTATES) |
			 MEMCFG_BUS_WIDTH(HW_ATA_BUS_0_MEMCFG_WIDTH)) << HW_ATA_BUS_0_MEMCFG_OFFSET,
#if defined(HW_ATA_BUS_0_MEMCFG_WIDTH_HACK)
  Memcfg16bitSetting:   (MEMCFG_CLKENB(HW_ATA_BUS_0_MEMCFG_CLKENB)            |
			 MEMCFG_WAIT_STATES(8-HW_ATA_BUS_0_MEMCFG_WAITSTATES) |
			 MEMCFG_BUS_WIDTH(MEMCFG_BUS_WIDTH_16)) << HW_ATA_BUS_0_MEMCFG_OFFSET,
#endif
  
  Initialized: false,
  IsReset: false,

#if defined(HW_ATA_BUS_0_MASTER) && defined(HW_ATA_BUS_0_SLAVE)
  DeviceCount: 2,
#else
  DeviceCount: 1,
#endif
  
  Device:
  {
#if defined(HW_ATA_BUS_0_MASTER)
    {
      Drive: 0,
      Bus: &Bus0,
      Partition: &Bus0_Master_Partition,
    },
#endif
#if defined(HW_ATA_BUS_0_SLAVE)
    {
      Drive: 1,
      Bus: &Bus0,
      Partition: &Bus0_Slave_Partition,
    },
#endif
  }
};

#endif // HW_ATA_ENABLE_BUS_0


// Bus0 definition
#if defined(HW_ATA_ENABLE_BUS_1)

#if defined(HW_ATA_BUS_1_MASTER)
static Partition_T Bus1_Master_Partition =
{
  SectorSize: HW_ATA_BUS_1_MASTER_SECTOR_SIZE,
};
#endif // HW_ATA_BUS_1_MASTER

#if defined(HW_ATA_BUS_1_SLAVE)
static Partition_T Bus1_Slave_Partition =
{
  SectorSize: HW_ATA_BUS_1_SLAVE_SECTOR_SIZE,
};
#endif // HW_ATA_BUS_1_SLAVE

#if defined(HW_ATA_BUS_1_HARD_RESET_FUNC)
// prototype it
extern void HW_ATA_BUS_1_HARD_RESET_FUNC ( BusSpace_T* );
#endif


// Bus1 defintion
BusSpace_T Bus1 =
{
  Init:  BusInit,
  Reset: BusReset,
#if defined(HW_ATA_BUS_1_HARD_RESET_FUNC)
  HardReset: HW_ATA_BUS_1_HARD_RESET_FUNC,
#else
  HardReset: 0,
#endif
  PowerDown: BusPowerDown,
  PowerUp:   BusPowerUp,

#if defined(HW_ATA_BUS_1_MEMCFG_WIDTH_HACK)
  Write16: BusWrite16M,
  Write16Multiple: BusWrite16MultipleM,
  Read16Multiple: BusRead16MultipleM,
#else
  Write16: BusWrite16,
  Write16Multiple: BusWrite16Multiple,
  Read16Multiple: BusRead16Multiple,
#endif  // HW_ATA_BUS_1_MEMCFG_WIDTH_HACK
  
  Base:             HW_ATA_BUS_1_BASE,
  DataReg:          HW_ATA_BUS_1_REGISTERS,
#if 0
  ErrorReg:         HW_ATA_BUS_1_REGISTERS[1],
  SectorCountReg:   HW_ATA_BUS_1_REGISTERS[2],
  SectorNumberReg:  HW_ATA_BUS_1_REGISTERS[3],
  CylinderLowReg:   HW_ATA_BUS_1_REGISTERS[4],
  CylinderHighReg:  HW_ATA_BUS_1_REGISTERS[5],
  DeviceHeadReg:    HW_ATA_BUS_1_REGISTERS[6],
  CommandReg:       HW_ATA_BUS_1_REGISTERS[7],
  ControlReg:       HW_ATA_BUS_1_REGISTERS[8],
#endif
  
  IRQ:  HW_ATA_BUS_1_IRQ,
  ISR:  BusInterruptISR,
  DSR:  BusInterruptDSR,

  MemcfgRegister: (volatile unsigned int*)HW_ATA_BUS_1_MEMCFG_REG,
  MemcfgMask: (0xff << HW_ATA_BUS_1_MEMCFG_OFFSET),
  MemcfgDefaultSetting: (MEMCFG_CLKENB(HW_ATA_BUS_1_MEMCFG_CLKENB)            |
			 MEMCFG_WAIT_STATES(8-HW_ATA_BUS_1_MEMCFG_WAITSTATES) |
			 MEMCFG_BUS_WIDTH(HW_ATA_BUS_1_MEMCFG_WIDTH)) << HW_ATA_BUS_1_MEMCFG_OFFSET,
#if defined(HW_ATA_BUS_1_MEMCFG_WIDTH_HACK)
  Memcfg16bitSetting:   (MEMCFG_CLKENB(HW_ATA_BUS_1_MEMCFG_CLKENB)            |
			 MEMCFG_WAIT_STATES(8-HW_ATA_BUS_1_MEMCFG_WAITSTATES) |
			 MEMCFG_BUS_WIDTH(MEMCFG_BUS_WIDTH_16)) << HW_ATA_BUS_1_MEMCFG_OFFSET,
#endif
  
  Initialized: false,
  IsReset: false,

#if defined(HW_ATA_BUS_1_MASTER) && defined(HW_ATA_BUS_1_SLAVE)
  DeviceCount: 2,
#else
  DeviceCount: 1,
#endif
  
  Device:
  {
#if defined(HW_ATA_BUS_1_MASTER)
    {
      Drive: 0,
      Bus: &Bus1,
      Partition: &Bus1_Master_Partition,
    },
#endif
#if defined(HW_ATA_BUS_1_SLAVE)
    {
      Drive: 1,
      Bus: &Bus1,
      Partition: &Bus1_Slave_Partition,
    },
#endif
  }
};

#endif // HW_ATA_ENABLE_BUS_1




