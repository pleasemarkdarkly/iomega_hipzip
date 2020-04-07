# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name screen
type simplegui

export Screen.h

compile Screen.cpp

arch Screen.o

dist include/Screen.h default.a