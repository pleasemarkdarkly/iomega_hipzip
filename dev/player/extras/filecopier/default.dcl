# default.dcl: default configuration for the file copier module
# edwardm@iobjects.com 2/6/2002

name filecopier
type extras

requires thread_util debug_util

export FileCopier.h

compile FileCopier.cpp 
