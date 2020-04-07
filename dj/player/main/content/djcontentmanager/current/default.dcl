# default.dcl: default configuration for the dj content manager module
# edwardm@iobjects.com 6/01/02

name djcontentmanager
type content

build_flags -DINCREMENTAL_METAKIT

requires common_content metakit_util input_datastream output_datastream metadatatable_content debug_util

export DJContentManager.h

compile DJContentManager.cpp DJContentManagerImp.cpp DJContentManagerMetadata.cpp

arch DJContentManager.o DJContentManagerImp.o DJContentManagerMetadata.o

dist include/DJContentManager.h default.a