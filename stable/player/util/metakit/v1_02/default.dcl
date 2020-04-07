# default.dcl: default configuration for metakit on dharma
# edwardm@iobjects.com 7/23/01
# (c) Interactive Objects

name metakit
type util

requires debug_util fatfile_datastream

export mk4.h mk4.inl mk4dll.h mk4io.h mk4str.h mk4str.inl

compile column.cpp custom.cpp derived.cpp field.cpp fileio.cpp
compile format.cpp gnuc.cpp handler.cpp persist.cpp remap.cpp std.cpp
compile store.cpp string.cpp table.cpp univ.cpp view.cpp viewx.cpp

export regress.h
#uncomment the following compile to build the regression tests.
#compile tbasics.cpp tcusto1.cpp tcusto2.cpp tdiffer.cpp textend.cpp tformat.cpp tlimits.cpp tmapped.cpp tnotify.cpp tresize.cpp tstore1.cpp tstore2.cpp tstore3.cpp tstore4.cpp # tbasics.cpp 

tests regress.cpp

arch column.o custom.o derived.o field.o fileio.o
arch format.o handler.o persist.o remap.o std.o
arch store.o string.o table.o univ.o view.o viewx.o


dist include/mk4.h include/mk4.inl include/mk4dll.h include/mk4io.h
dist include/mk4str.h include/mk4str.inl
dist default.a
