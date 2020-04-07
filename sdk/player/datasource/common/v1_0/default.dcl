# default.dcl: default configuration for the data source module
# edwardm@iobjects.com 7/13/01

name common
type datasource

requires eresult_util common_content common_playlist
requires input_datastream output_datastream

export DataSource.h

dist include/DataSource.h