# default.dcl: default configuration for the cddb proto toolkit module
# edwardm@iobjects.com 4/04/2002

name cddb_toolkit_proto
type extras

build_flags -DGN_NO_CACHE

#requires cddb_abstract_layer_extras cddb_common_pubutils_extras cddb_common_pubutils_extras
#requires cddb_common_extras cddb_toolkit_commmgr_extras cddb_toolkit_proto_online_extras

compile embedded_database_proto.c hid.c system_proto.c toc_lookup_proto.c