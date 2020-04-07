# default.dcl: default configuration for hd, cd, and cf demo program on dharma
# edwardm@iobjects.com 8/10/01
# (c) Interactive Objects

name hdcdcf
type other

requires playmanager_core mediaplayer_core events_core
requires keyboard_dev audio_io manager_plformat
requires input_datastream output_datastream
requires outfilter_filter waveout_datastream
requires simpleplaylist_playlist
requires metakitcontentmanager_content
requires eventq_util debug_util tchar_util

compile main.cpp Events.cpp

dist src/main.cpp src/Events.cpp src/Events.h
