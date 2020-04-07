# incremental.dcl: incremental configuration for the data source content manager module
# edwardm@iobjects.com 3/01/02

name datasourcecontentmanager
type content

build_flags -DINCREMENTAL_METAKIT

requires common_content metakit_util input_datastream output_datastream metadatatable_content debug_util

export DataSourceContentManager.h

compile DataSourceContentManager.cpp DataSourceContentManagerImp.cpp DataSourceContentManagerMetadata.cpp

arch DataSourceContentManager.o DataSourceContentManagerImp.o DataSourceContentManagerMetadata.o

dist include/DataSourceContentManager.h default.a