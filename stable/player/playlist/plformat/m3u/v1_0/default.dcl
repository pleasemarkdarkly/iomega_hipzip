# default.dcl: m3u (WinAmp playlist) format configuration
# edwardm@iobjects.com 8/8/01
# (c) Interactive Objects

name m3u
type plformat

requires common_plformat common_content playmanager_core
requires common_datasource datasourcemanager_datasource
requires tchar_util debug_util

compile M3UPlaylistFormat.cpp

arch M3UPlaylistFormat.o

dist default.a