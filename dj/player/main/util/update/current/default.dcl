# default.dcl: default configuration for update management (image discovery, etc)
# temancl@iobjects.com 7/07/01

name update
type util

requires utils_util

export UpdateApp.h ParseConfig.h
compile UpdateApp.cpp ParseConfig.cpp
