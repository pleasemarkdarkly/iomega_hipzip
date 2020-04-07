# default.dcl: default configuration for the CD data source module
# edwardm@iobjects.com 7/7/01

name dpdatasource
type datasource

requires dataplay_fs common_datasource

export DPDataSource.h

compile DPDataSource.cpp

#tests cd_data_source_test.cpp