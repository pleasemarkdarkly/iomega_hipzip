# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name drawregion
type screenelem

export DrawRegion.h

compile DrawRegion.cpp

arch DrawRegion.o

dist include/DrawRegion.h default.a
