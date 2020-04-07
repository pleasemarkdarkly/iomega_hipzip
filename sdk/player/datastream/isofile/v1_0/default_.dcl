# default.dcl: default configuration for iso file interface
# edwardm@iobjects.com 07/21/01
# (c) Interactive Objects

name isofile
type datastream

requires iso_fs input_datastream

export IsoFileInputStream.h

link default.a
