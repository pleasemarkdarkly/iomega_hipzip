# default.dcl: default configuration for fat layer on dharma
# danc@iobjects.com 6/06/01

name fat
type fs

requires storage_io thread_util

compile apiutil.c block.c chkmedia.c criterr.c
compile devio.c drobj.c errcode.c filesrvc.c
compile flapi.c flconst.c flutil.c format.c
compile fsapi.c ioconst.c iome_fat.c ioutil.c
compile longfn.c lowl.c pc_memry.c pckernel.c
compile report.c timer.c util.c

export sdapi.h sdtypes.h sdconfig.h

build_flags -DENABLE_FAT

tests fat_test.c lfn.cpp regress.c eth_test.c item_test.c

arch apiutil.o block.o chkmedia.o criterr.o
arch devio.o drobj.o errcode.o filesrvc.o
arch flapi.o flconst.o flutil.o format.o
arch fsapi.o ioconst.o iome_fat.o ioutil.o
arch longfn.o lowl.o pc_memry.o pckernel.o
arch report.o timer.o util.o

dist include/sdapi.h include/sdtypes.h include/sdconfig.h
dist tests/fat_test.c tests/lfn.cpp tests/regress.c tests/fat_test.c
dist default.a

header _fat_config.h start
// _fat_config.h - configuration for the fat layer

#define NUM_FAT_USERS           1
#define NUM_FAT_BLOCK_BUFFS    64
#define NUM_FAT_USER_FILES     16
#define NUM_FAT_TABLE_BUFFS    64

#define THREAD_SAFE_FS

header _fat_config.h end