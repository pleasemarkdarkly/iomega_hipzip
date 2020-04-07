# default.dcl: default configuration for peg on dharma
# ericg@iobjects.com 8/31/01

name screendriver
type peg

requires lcd_dev peg_gui

compile monoscrn.cpp

export monoscrn.hpp

arch monoscrn.o

dist include/monoscrn.hpp default.a
