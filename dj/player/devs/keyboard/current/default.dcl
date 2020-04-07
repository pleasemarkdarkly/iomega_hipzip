# default.dcl: default configuration for keyboard
# danc@iobjects.com 07/13/01
# (c) Interactive Objects

name keyboard
type dev

requires eventq_util events_core

export Keyboard.h kbd_scan.h

compile Keyboard.cpp KeyboardImp.cpp kbd_scan.c

arch Keyboard.o KeyboardImp.o

dist include/Keyboard.h default.a tests/keyboard_test.cpp

tests keyboard_test.cpp
