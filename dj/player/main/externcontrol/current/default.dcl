# default.dcl: default configuration for the externcontrol module
# robl@iobjects.com 12/5/01

name externcontrol
type main

build_flags -DENABLE_EXTERN_CONTROL

export ExternControl.h ExternInterface.h

compile ExternControl.cpp ExternInterface.cpp

