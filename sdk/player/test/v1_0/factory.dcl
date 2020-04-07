# factory.dcl: configuration for Dharma v2 factory test
# danc@iobjects.com 08/09/01
# (c) Interactive Objects

name factory
type other

requires common_simplegui

compile factory.c testgui.cpp

tests allocate.cpp dumpflash.cpp

dist src/atabus.h src/ataproto.h src/busctrl.h src/cs8900.h testgui.h
dist src/factory.c src/testgui.cpp
dist tests/allocate.cpp tests/dumpflash.cpp
