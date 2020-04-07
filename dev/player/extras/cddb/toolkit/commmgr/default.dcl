# default.dcl: default configuration for the cddb toolkit comm manager module
# edwardm@iobjects.com 4/04/2002

name cddb_toolkit_commmgr
type extras

requires cddb_extras

export comm_core.h comm_native.h

compile comm_core.c comm_http.c
