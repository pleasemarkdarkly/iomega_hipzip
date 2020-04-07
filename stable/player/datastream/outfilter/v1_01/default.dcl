# default.dcl: default configuration for outfilter
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name outfilter
type filter

requires filter_datastream output_datastream

export OutFilterKeys.h

compile OutFilter.cpp

arch OutFilter.o

dist include/OutFilterKeys.h default.a