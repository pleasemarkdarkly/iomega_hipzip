# default.dcl: default configuration for the dj playlist module
# edwardm@iobjects.com 6/13/02

name djplaylist
type playlist

requires common_playlist datastructures_util

export DJPlaylist.h

compile DJPlaylist.cpp

dist include/DJPlaylist.h src/DJPlaylist.cpp