# default.dcl: default configuration for upnpprotocols interface
# robl@iobjects.com 10/16/01
# (c) Interactive Objects

name upnpprotocols
type util

build_flags -DINCLUDE_CLIENT_APIS

requires upnpapi_util

export gena.h ssdplib.h

compile gena_callback.cpp gena_client.cpp gena_server.cpp soap.cpp ssdplib.cpp ssdpparser.cpp

