# default.dcl: default configuration for the playlist module
# edwardm@iobjects.com 7/8/01

name common
type playlist

requires tchar_util common_content

export Playlist.h

tests playlist_test.cpp

dist include/Playlist.h