# default.dcl: default configuration for the DJ UPnP module
# edwardm@iobjects.com 11/8/01

name djupnp
type main

requires datastructures_util debug_util upnpapi_util

export DJUPnP.h UPnPEvents.h XMLDocs.h sample_util.h

compile DJUPnP.cpp sample_util.cpp XMLDocs.cpp

arch DJUPnP.o sample_util.o XMLDocs.o

dist include/DJUPnP.h include/UPnPEvents.h include/XMLDocs.h default.a