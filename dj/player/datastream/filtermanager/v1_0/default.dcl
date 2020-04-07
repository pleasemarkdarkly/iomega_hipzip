# default.dcl: default configuration for filter manager
# danc@iobjects.com 07/09/01
# (c) Interactive Objects

name filtermanager
type datastream

requires filter_datastream

export FilterManager.h

compile FilterManager.cpp

arch FilterManager.o

dist include/FilterManager.h default.a