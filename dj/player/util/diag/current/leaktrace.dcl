# default.dcl: default configuration for diag tools on dharma
# danc@iobjects.com 6/06/01

name diag
type util

requires debug_util

export diag.h TinyProfile.h

build_flags -DENABLE_LEAK_TRACING
# wrap the malloc and free symbols and win the game
link_flags -Wl,--wrap,malloc,--wrap,free,--wrap,realloc,--wrap,calloc

compile diag.cpp LeakTracer.cpp
# symtable.c

#tests dumpstack.cpp

dist include/diag.h
