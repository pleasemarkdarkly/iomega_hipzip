# default.dcl: default configuration BSD fdisk
# temancl@fullplaymedia.com 04/16/02

name fdisk
type fs

requires storage_io thread_util

compile fdisk.c

export fdisk.h

build_flags -DENABLE_FAT

tests fdisk_test.c

