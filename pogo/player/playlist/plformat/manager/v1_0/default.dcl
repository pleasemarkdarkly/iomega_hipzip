# default.dcl: basic playlist format manager configuration
# edwardm@iobjects.com 07/30/01
# (c) Interactive Objects

name manager
type plformat

requires common_plformat registry_util eresult_util common_content

export PlaylistFormatManager.h

compile PlaylistFormatManager.cpp PlaylistFormatManagerImp.cpp

arch PlaylistFormatManager.o PlaylistFormatManagerImp.o

dist include/PlaylistFormatManager.h default.a