# default.dcl: default configuration for timer interface
# danc@iobjects.com 07/29/01
# (c) Interactive Objects

name timer
type util

requires eresult_util

export Timer.h

link default.a

tests TimerTest.cpp
