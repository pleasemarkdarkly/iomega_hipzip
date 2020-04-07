# default.dcl: default configuration for the serialcontrol module
# robl@iobjects.com 12/5/01

name serialcontrol
type main

build_flags -DENABLE_SERIAL_CONTROL

export serialcontrol.h 

compile SerialControl.cpp 

