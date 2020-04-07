# default.dcl: default configuration for output manager
# danc@iobjects.com 07/09/01
# (c) Interactive Objects

name outputmanager
type datastream

requires registry_util output_datastream

export OutputManager.h

compile OutputManager.cpp

arch OutputManager.o

dist include/OutputManager.h default.a