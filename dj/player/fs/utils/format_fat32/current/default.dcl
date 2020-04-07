# default.dcl: default configuration BSD fdisk
# temancl@fullplaymedia.com 04/16/02

name format
type fsutil

requires storage_io thread_util

compile format_fat32.c

export format.h

tests format_test.c

