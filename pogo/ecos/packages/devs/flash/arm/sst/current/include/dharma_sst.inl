#ifndef CYGONCE_DEVS_FLASH_DHARMA_SST_INL
#define CYGONCE_DEVS_FLASH_DHARMA_SST_INL

// Dharma has two 16bit 4mb J3 strata parts
// The iPAQ system has two 16-bit devices.
// Doc says: a StrataFlash 28F320J3A. The 320 means 32Mbit, so 4Mbyte.
// Reality:  a StrataFlash 28F640J3A. The 640 means 64Mbit, so 8Mbyte.

#define CYGNUM_FLASH_DEVICES 	(1)
#define CYGNUM_FLASH_BASE 	(0xE0000000u)
#define CYGNUM_FLASH_BASE_MASK  (0xE0000000u) // 32Mb devices
#define CYGNUM_FLASH_WIDTH 	(16)
#define CYGNUM_FLASH_BLANK      (1)

#endif  // CYGONCE_DEVS_FLASH_DHARMA_SST_INL
// ------------------------------------------------------------------------
// EOF ipaq_strataflash.inl
