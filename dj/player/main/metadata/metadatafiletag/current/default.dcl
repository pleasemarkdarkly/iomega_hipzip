# default.dcl: default configuration for Pogo portable hard drive player application program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name metadatafiletag
type dj

requires id3v2_mp3_codec

compile MetadataFileTag.cpp

export MetadataFileTag.h
