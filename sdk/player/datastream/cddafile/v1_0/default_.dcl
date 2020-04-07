# default.dcl: default configuration for cdda file interface
# edwardm@iobjects.com 07/11/01
# (c) Interactive Objects

name cddafile
type datastream

requires input_datastream cd_datasource debug_util

export CDDAInputStream.h

link default.a
