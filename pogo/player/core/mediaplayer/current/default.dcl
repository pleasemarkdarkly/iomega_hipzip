# default.dcl: configuration for media player
# danc@iobjects.com 07/16/01
# (c) Interactive Objects

name mediaplayer
type core

requires codecmanager_codec datasourcemanager_datasource
requires eresult_util rbuf_util events_core filtermanager_datastream
requires input_datastream outputmanager_datastream common_playlist
requires outfilter_filter waveout_datastream
requires datastructures_util debug_util eventq_util rbuf_util


export MediaPlayer.h PlayStream.h

compile MediaPlayer.cpp MediaPlayerImp.cpp PlayStream.cpp


arch MediaPlayer.o MediaPlayerImp.o

dist include/MediaPlayer.h default.a
