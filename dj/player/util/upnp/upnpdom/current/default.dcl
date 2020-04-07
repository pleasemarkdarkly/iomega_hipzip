# default.dcl: default configuration for upnpdom interface
# robl@iobjects.com 10/16/01
# (c) Interactive Objects

name upnpdom
type util

build_flags -DINCLUDE_CLIENT_APIS

export all.h Attr.h Document.h domCif.h DOMException.h Element.h NamedNodeMap.h Node.h
export NodeAct.h NodeList.h Parser.h

compile Attr.cpp Document.cpp domCif.cpp DOMException.cpp Element.cpp NamedNodeMap.cpp
compile Node.cpp NodeAct.cpp NodeList.cpp Parser.cpp

