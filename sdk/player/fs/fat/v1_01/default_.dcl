# default.dcl: default configuration for fat layer on dharma
# danc@iobjects.com 6/06/01

name fat
type fs

requires storage_io

export sdapi.h sdtypes.h sdconfig.h

build_flags -DENABLE_FAT

link default.a

tests fat_test.c lfn.cpp regress.c eth_test.c

header _fat_config.h start
// _fat_config.h - configuration for the fat layer
// do not modify these values

#define NUM_FAT_USERS           1
#define NUM_FAT_BLOCK_BUFFS     8
#define NUM_FAT_USER_FILES      8
#define NUM_FAT_TABLE_BUFFS  0x4d

#define THREAD_SAFE_FS

header _fat_config.h end