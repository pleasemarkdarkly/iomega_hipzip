# default.dcl: default configuration for fat layer on dharma
# danc@iobjects.com 6/06/01

name font
type simplegui

compile BmpFont.cpp FontData.cpp

export BmpFont.h

arch BmpFont.o FontData.o

dist include/BmpFont.h default.a