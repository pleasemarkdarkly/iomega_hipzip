# default.dcl: arm mp3 codec configuration
# danc@iobjects.com 07/10/01
# (c) Interactive Objects

name arm_mp3
type codec

requires common_codec debug_util id3v2_mp3_codec tchar_util

compile mp3_codec.cpp swap_bytes.S
basic_link mpeg3lib.a

arch mp3vect.o mp3_codec.o swap_bytes.o
dist default.a