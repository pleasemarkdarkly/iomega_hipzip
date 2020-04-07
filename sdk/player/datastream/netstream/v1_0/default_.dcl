# default.dcl: default configuration for netstream
# danc@iobjects.com 08/02/01
# (c) Interactive Objects

name net
type datastream

# the requirements for this module assume you are building
# HTTPInputStream in.

requires input_datastream

export NetStream.h HTTPInputStream.h

link default.a
