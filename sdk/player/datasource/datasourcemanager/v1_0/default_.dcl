# default.dcl: default configuration for the data source module
# edwardm@iobjects.com 7/6/01

name datasourcemanager
type datasource

requires datastructures_util common_datasource
requires debug_util eventq_util events_core

export DataSourceManager.h

link default.a
