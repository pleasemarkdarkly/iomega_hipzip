# default.dcl: default configuration for iso9660 layer on dharma
# danc@iobjects.com 6/26/01

name iso
type fs

requires ata_storage

build_flags -DENABLE_ISOFS

link default.a

tests fileio1.cpp

