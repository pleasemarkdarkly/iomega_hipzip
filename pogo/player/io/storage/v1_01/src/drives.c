// drives.c: runtime configurable drive naming
// danc@iobjects.com 6/28/01
// (c) Interactive Objects

// this should, more appropriately, be set up into
// some sort of table which has physical device to
// file system mappings

#if defined(ENABLE_MMC)
#include <devs/storage/mmc/_mmc_dev.h>
#elif defined(ENABLE_ATA)
#include <devs/storage/ata/_ata_dev.h>
#else
#error "Unknown block storage device"
#endif

#include <io/storage/drives.h>

const char* const block_drive_names[ BLOCK_DEV_NUM_DRIVES ] = 
{
#ifdef BLOCK_DEV_HDA_NAME
	BLOCK_DEV_HDA_NAME,
#else
	0,
#endif
#ifdef BLOCK_DEV_HDB_NAME
	BLOCK_DEV_HDB_NAME,
#else
	0,
#endif
#ifdef BLOCK_DEV_CDA_NAME
	BLOCK_DEV_HDA_NAME,
#else
	0,
#endif
#ifdef BLOCK_DEV_CDB_NAME
	BLOCK_DEV_HDA_NAME,
#else
	0,
#endif
};

