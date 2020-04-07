# default.dcl: configuration for Pogo bringup test
# temancl@iobjects.com 11/27/01
# (c) Interactive Objects

name bringup
type other

#compile rwc_test.cpp
compile factory.c testgui.cpp

tests allocate.cpp dumpflash.cpp
