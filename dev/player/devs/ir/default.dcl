# default.dcl: default configuration for the consumer IR
# toddm@iobjects.com 08/29/01
# (c) Interactive Objects

name ir
type dev

requires debug_util eventq_util events_core

export IR.h

compile IR.cpp IRImp.cpp

arch IR.o IRImp.o

dist include/IR.h tests/ir_test2.cpp default.a

tests ir_test.cpp ir_test2.cpp