# default.dcl: default configuration for iso file interface
# edwardm@iobjects.com 07/21/01
# (c) Interactive Objects

name isofile
type datastream

requires iso_fs input_datastream debug_util

export IsoFileInputStream.h

compile IsoFileInputStream.cpp

arch IsoFileInputStream.o

dist include/IsoFileInputStream.h default.a