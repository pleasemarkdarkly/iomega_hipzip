# default.dcl: basic configuration for pcm codec
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name pcm
type codec

requires pcm_codec

export WAVCodecKeys.h

compile WAVCodec.cpp

dist include/WAVCodecKeys.h src/WAVCodec.cpp src/WAVCodec.h