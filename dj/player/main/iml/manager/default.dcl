# default.dcl: default configuration for the IML manager module
# edwardm@iobjects.com 11/8/01

name manager
type iml

requires iml_iml playmanager_core net_datasource debug_util

export IMLManager.h

compile IMLManager.cpp

arch IMLManager.o

dist include/IMLManager.h default.a