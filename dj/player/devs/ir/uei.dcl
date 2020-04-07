# uei.dcl: driver for UEI Remotes on Dharma2/DJ hardware
# toddm@iobjects.com 08/29/01
# (c) Interactive Objects

name ir_uei
type dev

requires debug_util eventq_util events_core

export IR_UEI.h

compile IR_UEI.cpp IRImp_UEI.cpp

arch IR_UEI.o IRImp_UEI.o

dist include/IR_UEI.h tests/ir_test_UEI.cpp default.a

tests ir_test_UEI.cpp 