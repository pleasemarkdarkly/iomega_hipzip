# default.dcl: pls (WinAmp playlist) format configuration
# edwardm@iobjects.com 1/15/02
# (c) Interactive Objects

name pls
type plformat

requires common_plformat common_content playmanager_core
requires common_datasource datasourcemanager_datasource
requires tchar_util debug_util

compile PLSPlaylistFormat.cpp

arch PLSPlaylistFormat.o

dist default.a