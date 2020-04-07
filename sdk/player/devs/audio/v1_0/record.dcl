# record.dcl: play and record configuration for audio driver
# toddm@iobjects.com 08/15/01
# (c) Interactive Objects

name audio
type dev

export dai.h cs4343.h cs5332.h

compile dai_fiq.S dai_fiq_monitor.S i2c.c dai.c
compile cs4343.c cs5332.c

arch dai_fiq.o dai_fiq_monitor.o i2c.o

dist include/dai.h include/cs4343.h include/cs5332.h record.a
dist tests/audio_test.cpp tests/record_test.cpp
dist tests/long_audio_left.h tests/long_audio_right.h
dist tests/short_audio_left.h tests/short_audio_right.h

build_flags -DENABLE_DAC_CS4343 -DENABLE_ADC_CS5332 -DENABLE_DAI_RECORD

tests audio_test.cpp record_test.cpp