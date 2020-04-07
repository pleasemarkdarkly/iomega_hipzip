# default.dcl: App-land usb support
# toddm@iobjects.com 10/9/01
# (c) Interactive Objects

name usb
type dev

export usb.h
compile usb.c

arch usb.o

dist include/usb.h default.a
