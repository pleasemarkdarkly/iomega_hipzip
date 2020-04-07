# default.dcl: default configuration for net data source
# danc@iobjects.com 08/02/01
# (c) Interactive Objects

name net
type datasource

requires net_datastream common_datasource
requires codecmanager_codec timer_util net_io

export NetDataSource.h

compile NetDataSource.cpp NetDataSourceImp.cpp

arch NetDataSource.o NetDataSourceImp.o

dist include/NetDataSource.h default.a
