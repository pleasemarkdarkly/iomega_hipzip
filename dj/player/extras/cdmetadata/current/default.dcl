# default.dcl: default configuration for the CD metadata module
# edwardm@fullplaymedia.com 4/25/02

name cdmetadata
type extras

requires common_content cd_datasource debug_util metakit_util

export CDMetadataEvents.h DiskInfo.h

compile DiskInfo.cpp

arch DiskInfo.o DiskInfoCache.o
dist include/CDMetadataEvents.h include/DiskInfo.h include/DiskInfoCache.h default.a