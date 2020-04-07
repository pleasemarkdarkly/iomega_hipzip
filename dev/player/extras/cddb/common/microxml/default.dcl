# default.dcl: default configuration for the cddb microxml module
# edwardm@iobjects.com 4/04/2002

name cddb_microxml
type extras

requires cddb_extras

compile microxml.c parser.c renderer.c
