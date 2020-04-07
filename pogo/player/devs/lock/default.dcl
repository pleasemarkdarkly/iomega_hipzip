# default.dcl: default configuration for pogo switch
# temancl@iobjects.com 01/02/02
# (c) Interactive Objects

name lockswitch
type dev

export LockSwitch.h
compile LockSwitch.cpp
tests lock_test.cpp