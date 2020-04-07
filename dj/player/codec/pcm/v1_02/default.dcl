# default.dcl: basic configuration for pcm codec
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name pcm
type codec

requires common_codec

export PCMCodec.h RAWCodec.h

compile PCMCodec.cpp RAWCodec.cpp

dist include/PCMCodec.h src/PCMCodec.cpp