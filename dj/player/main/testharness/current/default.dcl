# default.dcl: default configuration for the testharness module
# robl@iobjects.com 12/5/01

name testharness
type main

export testharness.h

compile CDebugRouter.cpp CEventLogger.cpp CTestStimulator.cpp CDeviceState.cpp

