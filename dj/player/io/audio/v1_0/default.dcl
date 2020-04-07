# default.dcl: default configuration for io/audio
# danc@iobjects.com 07/31/01
# (c) Interactive Objects

name audio
type io

requires audio_dev debug_util registry_util

export VolumeControl.h

compile VolumeControl.cpp VolumeControlImp.cpp

arch VolumeControl.o VolumeControlImp.o

dist include/VolumeControl.h default.a

