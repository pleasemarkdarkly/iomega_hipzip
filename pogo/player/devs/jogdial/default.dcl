# default.dcl: default configuration for jogdial
# temancl@iobjects.com 01/02/02
# (c) Interactive Objects

name jogdial
type dev

requires eventq_util events_core

export JogDial.h

compile JogDial.cpp

tests jogdial_test.cpp
