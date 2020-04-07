# default.dcl: default configuration for the thread module
# philr@iobjects.com 9/19/2001

name thread
type util

# requires

export Mutex.h ThreadedObject.h SynchronousGate.h

compile Mutex.cpp ThreadedObject.cpp SynchronousGate.cpp

#tests