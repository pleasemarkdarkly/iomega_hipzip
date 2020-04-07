# default.dcl: default configuration for the simple metadata module
# edwardm@iobjects.com 7/31/01

name simplemetadata
type content

requires common_content debug_util

export SimpleMetadata.h

compile SimpleMetadata.cpp

arch SimpleMetadata.o

dist include/SimpleMetadata.h default.a