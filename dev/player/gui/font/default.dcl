# default.dcl: default configuration for fat layer on dharma
# danc@iobjects.com 6/06/01

name font
type gui

requires storage_io

compile BmpFont.cpp FontData.cpp

export BmpFont.h
