# default.dcl: default configuration for waveout class
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name waveout
type datastream

requires output_datastream audio_dev

export WaveOut.h WaveOutKeys.h

compile WaveOut.cpp

arch WaveOut.o

dist include/WaveOutKeys.h default.a