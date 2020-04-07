# default.dcl: default configuration for the simpler content manager module
# edwardm@iobjects.com 3/20/02

name simplercontentmanager
type content

requires common_content debug_util

export SimplerContentManager.h

compile SimplerContentManager.cpp SimplerMetadata.cpp

dist include/SimplerContentManager.h src/SimplerContentManager.cpp
