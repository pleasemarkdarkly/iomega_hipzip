# default.dcl: default configuration for the cddb pubutils module
# edwardm@iobjects.com 4/04/2002

name cddb_pubutils
type extras

requires cddb_extras

compile base64.c gn_dyn_buf.c gn_utils.c gnfs_readln.c toc_util.c
