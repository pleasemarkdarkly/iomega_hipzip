# default.dcl: default configuration for timer interface
# danc@iobjects.com 07/29/01
# (c) Interactive Objects

name timer
type util

requires eresult_util thread_util

export Timer.h

compile Timer.cpp

tests TimerTest.cpp

arch Timer.o

dist include/Timer.h tests/TimerTest.cpp default.a