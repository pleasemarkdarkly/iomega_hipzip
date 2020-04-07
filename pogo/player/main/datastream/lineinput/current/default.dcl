# default.dcl: default configuration for line input stream
# danc@fullplaymedia.com 08/29/01
# (c) Fullplay Media Systems

name lineinput
type datastream

requires input_datastream linein_datasource
requires audio_dev debug_util

export LineInputStream.h

compile LineInputStream.cpp

arch LineInputStream.o

dist include/LineInputStream.h default.a