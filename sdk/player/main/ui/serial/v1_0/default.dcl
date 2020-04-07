# default.dcl: default configuration for serial UI
# danc@iobjects.com 07/28/01
# (c) Interactive Objects

name serial
type ui

requires common_ui common_codec

export SerialUserInterface.h

compile SerialUserInterface.cpp

dist include/SerialUserInterface.h src/SerialUserInterface.cpp