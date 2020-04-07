# default.dcl: default configuration for FreeBSD fat layer port
# danc@iobjects.com 01/02/02
# (c) iobjects

name bsdfat
type fs

compile msdosfs_conv.c msdosfs_denode.c msdosfs_fat.c msdosfs_lookup.c msdosfs_vfsops.c msdosfs_vnops.c
compile msdosfs_fops.c

tests fs_test.cpp
