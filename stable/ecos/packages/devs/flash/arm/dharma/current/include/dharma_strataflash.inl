#ifndef CYGONCE_DEVS_FLASH_DHARMA_STRATAFLASH_INL
#define CYGONCE_DEVS_FLASH_DHARMA_STRATAFLASH_INL

// Dharma has two 16bit 4mb J3 strata parts
// The iPAQ system has two 16-bit devices.
// Doc says: a StrataFlash 28F320J3A. The 320 means 32Mbit, so 4Mbyte.
// Reality:  a StrataFlash 28F640J3A. The 640 means 64Mbit, so 8Mbyte.

#include <pkgconf/devs_flash_dharma.h>

#define CYGNUM_FLASH_BASE       (0xE0000000u)
#define CYGNUM_FLASH_WIDTH      (16)
#define CYGNUM_FLASH_BLANK      (1)

#define CYGNUM_FLASH_DEVICES    (CYGNUM_DEVS_FLASH_DHARMA_NUM_DEVICES)

#define CYGNUM_FLASH_SIZE       ((CYGNUM_FLASH_DEVICES)*(CYGDAT_DEVS_FLASH_DHARMA_DEVICE_SIZE/8)*(1024*1024))

// TODO factor device width into below part
#if CYGDAT_DEVS_FLASH_DHARMA_DEVICE_SIZE == 128
#define CYGNUM_FLASH_BASE_MASK  (0xFC000000u) // 128Mb devices
#elif CYGDAT_DEVS_FLASH_DHARMA_DEVICE_SIZE == 32
#define CYGNUM_FLASH_BASE_MASK  (0xFE000000u) // 32Mb devices
#else
#error "Unknown flash base mask - is the j3 part defined ?"
#endif

#endif  // CYGONCE_DEVS_FLASH_IPAQ_STRATAFLASH_INL
// ------------------------------------------------------------------------
// EOF ipaq_strataflash.inl
