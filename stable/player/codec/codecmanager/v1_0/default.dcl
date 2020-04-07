# default.dcl: basic codecmanager configuration
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name codecmanager
type codec

requires common_codec registry_util debug_util

export CodecManager.h

compile CodecManager.cpp CodecManagerImp.cpp

## arch/dist support for packaging
arch CodecManager.o CodecManagerImp.o
dist include/CodecManager.h default.a
