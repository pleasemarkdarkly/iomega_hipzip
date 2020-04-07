# default.dcl: default configuration for netstream
# danc@iobjects.com 08/02/01
# (c) Interactive Objects

name net
type datastream

# the requirements for this module assume you are building
# HTTPInputStream in.

requires input_datastream debug_util

export NetStream.h HTTPInputStream.h

compile NetStream.cpp HTTPInputStream.cpp

arch NetStream.o HTTPInputStream.o

dist include/NetStream.h include/HTTPInputStream.h default.a