# incremental.dcl: incremental configuration for the metakit content manager module
# edwardm@iobjects.com 11/08/01

name metakitcontentmanager
type content

build_flags -DINCREMENTAL_METAKIT

requires common_content metakit_util input_datastream output_datastream metadatatable_content debug_util

export MetakitContentManager.h

compile MetakitContentManager.cpp MetakitContentManagerImp.cpp

arch MetakitContentManager.o MetakitContentManagerImp.o

dist include/MetakitContentManager.h default.a