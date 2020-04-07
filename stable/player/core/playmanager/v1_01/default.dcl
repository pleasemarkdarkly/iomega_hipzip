# default.dcl: default configuration for play manager
# danc@iobjects.com 07/27/01
# (c) Interactive Objects

name playmanager
type core

requires common_content common_playlist common_datasource eresult_util
requires debug_util mediaplayer_core events_core

export PlayManager.h

compile PlayManager.cpp PlayManagerImp.cpp

arch PlayManager.o PlayManagerImp.o

dist include/PlayManager.h default.a
