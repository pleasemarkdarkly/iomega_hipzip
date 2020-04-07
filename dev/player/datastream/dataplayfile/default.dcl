# default.dcl: default configuration for cdda file interface
# edwardm@iobjects.com 07/11/01
# (c) Interactive Objects

name dataplayfile
type datastream

requires dataplay_fs input_datastream dpdatasource_datasource

export DPInputStream.h DPFile.h

compile DPInputStream.cpp DPFile.cpp
