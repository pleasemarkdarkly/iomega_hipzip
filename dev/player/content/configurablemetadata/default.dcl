# default.dcl: default configuration for the configurable metadata module
# edwardm@iobjects.com 7/31/01

name configurablemetadata
type content

requires common_content metadatatable_content

export ConfigurableMetadata.h

compile ConfigurableMetadata.cpp


arch ConfigurableMetadata.o
dist include/ConfigurableMetadata.h default.a