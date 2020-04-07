# default.dcl: default configuration for main program on dharma
# danc@iobjects.com 6/19/01
# (c) Interactive Objects

name main
type other

requires eventq_util

compile main.cpp Events.cpp

dist src/main.cpp src/Events.cpp src/Events.h
