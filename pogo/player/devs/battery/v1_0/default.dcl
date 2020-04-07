# default.dcl: default configuration for the AA battery
# danc@iobjects.com 07/18/01
# (c) Interactive Objects

name battery
type dev

export battery.h

compile aa_battery.c

arch aa_battery.o

dist include/battery.h tests/battery_test.cpp default.a

tests battery_test.cpp
