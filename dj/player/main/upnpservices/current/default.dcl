# default.dcl: default configuration for the DJ UPnP module
# robl@iobjects.com 12/5/01

name upnpservices
type main

build_flags -DINCLUDE_DEVICE_APIS -DINTERNAL_WEB_SERVER -DENABLE_DEBUG_ROUTING -DENABLE_EVENT_LOGGING

requires events_core debug_util upnpapi_util

export djservices.h DJServiceEvents.h XMLdocs.h 

compile djdevicedesc.cpp djservices.cpp XMLDocs.cpp  SCPD.cpp

arch djdevicedesc.o djservices.o XMLDocs.o SCPD.o

dist include/djservices.h include/DJServiceEvents.h include/XMLDocs.h default.a