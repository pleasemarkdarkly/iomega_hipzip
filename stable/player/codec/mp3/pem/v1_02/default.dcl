# default.dcl: pem mp3 encoder default configuration
# philr@iobjects.com 09/24/2001

name pem
type codec

requires debug_util common_codec

export codec.h pem.h fpmp3.h

compile code.c codec.c count.c huffman.c hybrid.c 
compile memory.c out.c
compile polyphase.c psy.c quant.c reservoir.c ro.c
compile pem.cpp fpmp3.c
#fpmp3.c fpmp3stub.c 

tests LineIn.cpp pem_test.cpp

build_flags -Icodec/mp3/pem -mtune=arm8 -DPEM_ARM_ASM  -DPEM_PRECISION -DPEM_USE_SRAM
#-DPEM_DEMO_LIMITFRAMES=100

