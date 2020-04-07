# default.dcl: default configuration for the simple content manager module
# edwardm@iobjects.com 7/10/01

name simplecontentmanager
type content

requires common_content debug_util

export SimpleContentManager.h

compile SimpleContentManager.cpp
