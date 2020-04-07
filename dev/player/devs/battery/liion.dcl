# liion.dcl: configuration for the li-ion battery
# toddm@iobjects.com 08/15/01
# (c) Interactive Objects

name battery
type dev

export battery.h

compile liion_battery.c

arch liion_battery.o

dist include/battery.h tests/battery_test.cpp liion.a

tests battery_test.cpp
