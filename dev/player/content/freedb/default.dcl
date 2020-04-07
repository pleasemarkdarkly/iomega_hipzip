# default.dcl: default configuration for the freedb module
# edwardm@iobjects.com 11/09/01

name freedb
type content

requires common_content cd_datasource fat_fs debug_util metakit_util

export FreeDB.h DiskInfoCache.h

compile FreeDB.cpp DiskInfoCache.cpp

arch FreeDB.o DiskInfoCache.o
dist include/FreeDB.h include/DiskInfoCache.h default.a