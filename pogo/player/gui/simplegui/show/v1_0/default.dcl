# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name show
type simplegui

export Show.h

compile Show.cpp

arch Show.o

dist include/Show.h default.a
