# default.dcl: default configuration for the pogoplaylist module
# ericg@iobjects.com 10/15/01

name playlist
type pogo

requires common_playlist

export PogoPlaylist.h

compile PogoPlaylist.cpp
