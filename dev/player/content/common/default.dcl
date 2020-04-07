# default.dcl: default configuration for the content module
# edwardm@iobjects.com 7/8/01

name common
type content

requires tchar_util datastructures_util common_datasource common_playlist eresult_util

export ContentManager.h QueryableContentManager.h Metadata.h

dist include/ContentManager.h include/QueryableContentManager.h include/Metadata.h
