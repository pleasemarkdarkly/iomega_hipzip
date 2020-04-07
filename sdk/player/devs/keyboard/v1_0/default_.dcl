# default.dcl: default configuration for keyboard
# danc@iobjects.com 07/13/01
# (c) Interactive Objects

name keyboard
type dev

requires eventq_util events_core

export Keyboard.h

link default.a

tests keyboard_test.cpp
