# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name textlabel
type screenelem

export TextLabel.h

compile TextLabel.cpp

arch TextLabel.o

dist include/TextLabel.h default.a
