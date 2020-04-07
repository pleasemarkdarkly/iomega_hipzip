# default.dcl: default configuration BSD chkdsk
# temancl@fullplaymedia.com 04/16/02

name chkdsk
type fsutil

requires storage_io thread_util

compile boot.c check.c dir.c fat.c

export chkdsk.h

tests chkdsk_test.c

