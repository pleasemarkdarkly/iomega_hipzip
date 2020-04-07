# default.dcl: default configuration for upnpapi interface
# robl@iobjects.com 10/16/01
# (c) Interactive Objects

name upnpapi
type util

build_flags -DINCLUDE_CLIENT_APIS

requires upnpgenlib_util upnpprotocols_util upnpdom_util

export upnpapi.h upnptools.h upnp.h upnp_debug.h interface.h config.h

compile upnpapi.cpp config.cpp upnptools.cpp

