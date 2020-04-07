# default.dcl: basic configuration for vorbis I codec
# monty@xiph.org 20020104
# (c) Interactive Objects

name vorbis
type codec

requires common_codec

export VorbisCodec.h 

compile VorbisCodec.cpp framing.cpp mdct.cpp sharedbook.cpp window.cpp	bitwise.cpp info.cpp synthesis.cpp block.cpp time0.cpp codebook.cpp registry.cpp vorbisfile.cpp floor0.cpp floor1.cpp mapping0.cpp res0.cpp misc.cpp
