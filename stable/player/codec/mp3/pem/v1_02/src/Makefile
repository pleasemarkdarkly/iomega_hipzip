DEFINES = -DSTANDALONE -DPEM_PRECISION -DPEM_USE_SRAM -DPEM_ARM_ASM -DPEM_DEMO_LIMITFRAMES=1150 -DPRIVATE= -DSTATIC=static

PREFIX := arm-elf-

CC = $(PREFIX)gcc -I../include 
LD = $(PREFIX)ld
NM = $(PREFIX)nm
AR = $(PREFIX)ar
STRIP = $(PREFIX)strip
OBJCOPY = $(PREFIX)objcopy

BASESOURCES := code.c codec.c count.c fpmp3.c huffman.c \
	hybrid.c memory.c out.c polyphase.c psy.c quant.c \
	reservoir.c ro.c
LIBSOURCES := $(BASESOURCES) fpmp3support.c fpmp3asm.S
APPSOURCES :=  $(BASESOURCES) in.c main.c

APPOBJS := $(APPSOURCES:.c=.o)

LIBOBJS := $(LIBSOURCES:.c=.o)
LIBOBJS := $(LIBOBJS:.S=.o)

BASERAW := $(BASESOURCES:.c=.i)

ASMS = $(SOURCES:.c=.s)

# (dd, 23 Jan 2002)
CC_SWITCHES = -O2 -Wall -mcpu=arm7tdmi  -mtune=arm8 -ffunction-sections -fdata-sections
LIBS        = -lm

all: pem

cpp:	$(BASERAW)

asm:	$(ASMS)

static:	$(APPOBJS)
	$(CC) -static -o pem $(APPOBJS) $(LIBS)

pem:	$(APPOBJS)
	$(CC) -o pem $(APPOBJS) $(LIBS)

%.i: %.c #Makefile
	$(CC) $(DEFINES) $(CC_SWITCHES) -E $< -o $@

%.o: %.c #Makefile
	$(CC) $(DEFINES) $(CC_SWITCHES) -c $< -o $@

%.o: %.S #Makefile
	$(CC) $(DEFINES) $(CC_SWITCHES) -c $< -o $@

%.s: %.c #Makefile
	$(CC) $(DEFINES) $(CC_SWITCHES) -fverbose-asm -S $< -o $@

clean:
	rm -f $(APPOBJS) $(LIBOBJS) core $(PGM) $(BASERAW)

libfpmp3.o: $(LIBOBJS) Makefile standalone.ld
	rm -f keepsymbols.txt
	$(NM) --defined-only fpmp3.o | cut "-d " -f 3 | grep ^fpmp3_ > keepsymbols.txt
	$(LD) -static -O9 -e 0x0 -Tstandalone.ld -o libfpmp3.o $(LIBOBJS) /usr/local/arm-elf/lib/gcc-lib/arm-elf/3.0.3/libgcc.a
	$(OBJCOPY) --keep-global-symbol JUMPTABLE --keep-global-symbols keepsymbols.txt -x libfpmp3.o

libfpmp3.a: libfpmp3.o
	rm -f libfpmp3.a
	$(AR) rcs libfpmp3.a libfpmp3.o

libfpmp3.bin: libfpmp3.o
	$(OBJCOPY) -O binary libfpmp3.o libfpmp3.bin

depend:
	makedepend $(SOURCES)

everything.o: $(LIBSOURCES:.o=.c)
	$(CC) $(DEFINES) $(CC_SWITCHES) -c everything.c -o everything.o
	$(STRIP) --strip-unneeded everything.o

# DO NOT DELETE THIS LINE -- make depend depends on it.

