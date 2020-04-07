# default.dcl: default configuration for registry
# danc@iobjects.com 07/08/01
# (c) Interactive Objects

name registry
type util

requires debug_util eresult_util datastructures_util
requires input_datastream output_datastream

export Registry.h

compile Registry.cpp RegistryImp.cpp

arch Registry.o RegistryImp.o
dist include/Registry.h default.a