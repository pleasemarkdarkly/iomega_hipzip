# default.dcl: default configuration for fat layer on dharma
# danc@iobjects.com 6/06/01

name dataplay
type fs

requires storage_io

compile dfs.c dp.c dp_hw.c 

export dfs.h dp.h dp_hw.h dpcodes.h dptypes.h

build_flags -DENABLE_DATAPLAY
