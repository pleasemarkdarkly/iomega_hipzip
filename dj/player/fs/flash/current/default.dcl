# default.dcl: default configuration for FIS flash manager tool
# danc@iobjects.com 6/06/01

name flash
type util

requires utils_util

export fis.h flashmanager.h

compile flashmanager.cpp
