# default.dcl: default configuration for iso9660 layer on dharma
# danc@iobjects.com 6/26/01

name iso
type fs

requires ata_storage

compile cd9660_bio.c cd9660_dops.c cd9660_fops.c
compile cd9660_fsops.c cd9660_internal.c cd9660_lookup.c
compile cd9660_node.c cd9660_rrip.c cd9660_support.c

build_flags -DENABLE_ISOFS

tests fileio1.cpp

arch cd9660_bio.o cd9660_dops.o cd9660_fops.o
arch cd9660_fsops.o cd9660_internal.o cd9660_lookup.o
arch cd9660_node.o cd9660_rrip.o cd9660_support.o

dist tests/fileio1.cpp default.a
