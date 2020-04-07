# default.dcl: configuration for SRC filter
# danc@iobjects.com

name src
type filter

requires filter_datastream

export SRCFilterKeys.h

compile SRCFilter.cpp

arch SRCFilter.o

dist include/SRCFilterKeys.h default.a
