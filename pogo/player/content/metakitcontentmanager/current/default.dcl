# default.dcl: default configuration for the metakit content manager module
# edwardm@iobjects.com 7/23/01

name metakitcontentmanager
type content

requires common_content metakit_util input_datastream output_datastream metadatatable_content debug_util

export MetakitContentManager.h

compile MetakitContentManager.cpp MetakitContentManagerImp.cpp

arch MetakitContentManager.o MetakitContentManagerImp.o

dist include/MetakitContentManager.h default.a