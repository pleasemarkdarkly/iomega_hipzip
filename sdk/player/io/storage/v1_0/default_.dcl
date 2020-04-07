# default.dcl: default configuration for block device interface
# danc@iobjects.com 6/18/01

name storage
type io

# this module is meaningless without a physical storage device
requires any_storage

export blk_dev.h drives.h

link default.a
