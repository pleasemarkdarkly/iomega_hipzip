# default.dcl: default configuration for audio driver (cs4343 only)
# danc@iobjects.com 07/05/01
# (c) Interactive Objects

name audio
type dev

requires debug_util

export dai.h cs4343.h

link default.a

build_flags -DENABLE_DAC_CS4343

tests audio_test.cpp