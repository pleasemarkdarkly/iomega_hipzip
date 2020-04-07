# default.dcl: default configuration for the playlist module
# edwardm@iobjects.com 7/8/01

name simpleplaylist
type playlist

requires common_playlist datastructures_util

export SimplePlaylist.h

compile SimplePlaylist.cpp

dist include/SimplePlaylist.h src/SimplePlaylist.cpp