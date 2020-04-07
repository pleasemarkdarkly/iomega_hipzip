# default.dcl: default configuration for line in data source
# danc@fullplaymedia.com 08/29/01
# (c) Fullplay Media Systems

name linein
type datasource

requires common_datasource codecmanager_codec

export LineInDataSource.h

compile LineInDataSource.cpp LineInDataSourceImp.cpp

arch LineInDataSource.o LineInDataSourceImp.o

dist include/LineInDataSource.h default.a