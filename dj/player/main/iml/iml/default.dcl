# default.dcl: default configuration for the IML module
# edwardm@iobjects.com 11/8/01

name iml
type iml

requires codecmanager_codec debug_util

export IML.h

compile IML.cpp

arch IML.o

dist include/IML.h default.a