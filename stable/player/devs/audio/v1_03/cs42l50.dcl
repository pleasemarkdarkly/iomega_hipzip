# cs42l50.dcl: play and record configuration for audio driver
# cs42L50 is a 4343/5332 combination/variant
# define both, then define ENABLE_CS42L50 for changes
# toddm@iobjects.com 08/15/01
# temancl@fullplaymedia.com
# (c) Interactive Objects

name audio
type dev

export dai.h cs4343.h cs5332.h cs8405a.h

compile dai_fiq.S dai_fiq_monitor.S i2c.c dai.c
compile cs4343.c cs5332.c cs8405a.c

arch dai_fiq.o dai_fiq_monitor.o i2c.o

dist include/dai.h include/cs4343.h include/cs5332.h record.a
dist tests/audio_test.cpp tests/record_test.cpp
dist tests/long_audio_left.h tests/long_audio_right.h
dist tests/short_audio_left.h tests/short_audio_right.h

build_flags -DENABLE_CS42L50 -DENABLE_DAC_CS4343 -DENABLE_ADC_CS5332 -DENABLE_DAI_RECORD

tests audio_test.cpp record_test.cpp