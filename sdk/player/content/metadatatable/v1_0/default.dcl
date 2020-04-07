# default.dcl: default configuration for the metadata table module
# edwardm@iobjects.com 9/1/01

name metadatatable
type content

requires common_content

export MetadataTable.h

compile MetadataTable.cpp

arch MetadataTable.o

dist default.a include/MetadataTable.h