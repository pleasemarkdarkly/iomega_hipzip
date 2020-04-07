# default.dcl: default configuration for debug tools on dharma
# danc@iobjects.com 6/06/01

name debug
type util

export debug.h

compile debug.c

dist include/debug.h src/debug.c
