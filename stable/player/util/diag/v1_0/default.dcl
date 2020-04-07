# default.dcl: default configuration for diag tools on dharma
# danc@iobjects.com 6/06/01

name diag
type util

requires debug_util

export diag.h TinyProfile.h

compile diag.cpp
# symtable.c

#tests dumpstack.cpp

dist include/diag.h
