# default.dcl: default configuration for audio driver (cs4343 only)
# danc@iobjects.com 07/05/01
# (c) Interactive Objects

name audio
type dev

requires debug_util

export dai.h cs4343.h

compile dai_fiq.S dai_fiq_monitor.S  i2c.c dai.c
compile cs4343.c

arch dai_fiq.o dai_fiq_monitor.o i2c.o dai.o cs4343.o

dist include/dai.h include/cs4343.h tests/audio_test.cpp default.a
dist tests/long_audio_left.h tests/long_audio_right.h
dist tests/short_audio_left.h tests/short_audio_right.h

build_flags -DENABLE_DAC_CS4343

tests audio_test.cpp