# default.dcl: default configuration for the CD data source module
# edwardm@iobjects.com 7/7/01

name cd
type datasource

requires iso_fs common_datasource storage_io
requires events_core playmanager_core 
requires cddafile_datastream isofile_datastream
requires common_codec codecmanager_codec
requires debug_util timer_util eventq_util

export CDDataSource.h

link default.a
