# default.dcl: default configuration for line input stream
# danc@iobjects.com 08/29/01
# (c) Interactive Objects

name lineinput
type datastream

requires input_datastream linein_datasource
requires audio_dev debug_util

export LineInputStream.h

link default.a
