# default.dcl: default configuration for the cddb proto online toolkit module
# edwardm@iobjects.com 4/04/2002

name cddb_toolkit_proto_online
type extras

#requires cddb_abstract_layer_extras cddb_common_pubutils_extras cddb_common_pubutils_extras
#requires cddb_toolkit_commmgr_extras

export proto_online.h

compile gn_cddbmsg.c gn_payload.c gn_transmit.c online_lookup.c