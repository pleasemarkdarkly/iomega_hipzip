# default.djp: djp (DJ playlist) format configuration
# edwardm@iobjects.com 12/27/01
# (c) Interactive Objects

name djp
type plformat

requires common_plformat common_content
requires datasourcemanager_datasource common_playlist
requires cd_datasource fat_datasource net_datasource
requires debug_util

compile DJPlaylistFormat.cpp

arch DJPlaylistFormat.o

dist default.a