# default.dcl: default configuration for event queue on dharma
# danc@iobjects.com 6/06/01
# (c) Interactive Objects

name eventq
type util

requires datastructures_util

export EventQueueAPI.h

compile EventQueue.cpp EventQueueImp.cpp

arch EventQueue.o EventQueueImp.o

dist include/EventQueueAPI.h default.a