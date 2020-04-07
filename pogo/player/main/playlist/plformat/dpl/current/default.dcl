# default.dcl: dpl (Dadio playlist) format configuration
# edwardm@fullplaymedia.com 07/30/01
# (c) Fullplay Media Systems

name dpl
type plformat

requires common_plformat common_content
requires datasourcemanager_datasource common_playlist
requires debug_util

compile DPLPlaylistFormat.cpp

arch DPLPlaylistFormat.o

dist default.a