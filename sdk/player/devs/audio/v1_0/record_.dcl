# record.dcl: play and record configuration for audio driver
# toddm@iobjects.com 08/15/01
# (c) Interactive Objects

name audio
type dev

export dai.h cs4343.h cs5332.h

link record.a

build_flags -DENABLE_DAC_CS4343 -DENABLE_ADC_CS5332 -DENABLE_DAI_RECORD

tests audio_test.cpp record_test.cpp