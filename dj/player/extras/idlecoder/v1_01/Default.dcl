# default.dcl: default configuration for the idlecoder module
# philr@iobjects.com 9/19/2001

name idlecoder
type extras

requires pem_codec thread_util debug_util registry_util

export IdleCoder.h

compile IdleCoder.cpp 

tests idlecoder_test.cpp
