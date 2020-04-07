# default.dcl: default configuration for net data source
# danc@iobjects.com 08/02/01
# (c) Interactive Objects

name net
type datasource

requires net_datastream common_datasource
requires codecmanager_codec timer_util

export NetDataSource.h

link default.a

