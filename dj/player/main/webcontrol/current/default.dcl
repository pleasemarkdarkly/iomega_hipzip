# default.dcl: default configuration for the webcontrol module
# robl@iobjects.com 12/5/01

name webcontrol
type main

export webcontrol.h FunctionInterface.h

compile WebControlServer.cpp FunctionInterface.cpp

