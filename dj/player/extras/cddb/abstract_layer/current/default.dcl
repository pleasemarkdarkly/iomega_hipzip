# default.dcl: default configuration for the cddb abstract layer module
# edwardm@iobjects.com 4/04/2002

name cddb_abstract_layer
type extras

#requires cddb_extras fat_fs
requires cddb_extras

compile gn_build.c gn_configmgr.c gn_ctype.c gn_error_code_strings.c gn_errors.c
compile gn_log.c gn_string.c
