# default.dcl: default configuration for buffer management system
# danc@iobjects.com 08/26/01
# (c) Interactive Objects

name buffer
type datastream

requires input_datastream

export BufferedInputStream.h BufferThread.h BufferConfig.h

compile BufferedInputStream.cpp BufferThread.cpp

