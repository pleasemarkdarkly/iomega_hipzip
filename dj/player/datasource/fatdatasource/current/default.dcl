# default.dcl: default configuration for the FAT data source module
# edwardm@iobjects.com 7/17/01

name fat
type datasource

requires fat_fs fatfile_datastream common_datasource
requires common_codec codecmanager_codec events_core playmanager_core
requires debug_util timer_util eventq_util

export FatDataSource.h

compile FatDataSource.cpp FatDataSourceImp.cpp

tests fat_data_source_test.cpp

arch FatDataSource.o FatDataSourceImp.o
dist include/FatDataSource.h default.a