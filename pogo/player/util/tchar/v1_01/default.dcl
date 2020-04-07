# default.dcl: default configuration for the ascii/unicode module
# edwardm@iobjects.com 7/16/01

name tchar
type util

export tchar.h

compile tchar.cpp

arch tchar.o
dist include/tchar.h default.a