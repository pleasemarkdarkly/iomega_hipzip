# default.dcl: default configuration for the cddb toolkit memory manager module
# edwardm@iobjects.com 4/04/2002

name cddb_toolkit_memmgr
type extras

requires cddb_extras

compile memmgr_core.c memmgr_debug.c memmgr_diag.c
