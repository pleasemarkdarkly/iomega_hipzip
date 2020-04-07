#*****************************************************************************
#
# MAKEFILE - Builds the download utility.
#
# Copyright (c) 2001 Cirrus Logic, Inc.
#
#*****************************************************************************

#
# The default rule, which builds the download utility.
#
all: downloader.exe

#
# A rule to build the download utility.
#
downloader.exe: download.c flasher.h
	gcc download.c -o downloader.exe

#
# A rule to create the FLASH code header file from the binary image.
#
flasher.h: makeboot.exe flasher.rom
	./makeboot

#
# A rule to create the utility which converts the FLASH code from a binary
# image into a C header file.
#
makeboot.exe: makeboot.c
	gcc makeboot.c -o makeboot.exe

#
# A rule to link the FLASH code and convert it into a binary image.
#
flasher.rom: flasher.o
	arm-elf-gcc -e ENTRY -Ttext 0x10000000 -nostdlib -o flasher.exe flasher.o
	arm-elf-objcopy -O binary flasher.exe flasher.rom
	@rm -f flasher.exe

#
# A rule to assemble the FLASH code.
#
flasher.o: flasher.S
	arm-elf-gcc -Wa,-mall,-marm7tdmi -c -o flasher.o flasher.S

#
# A rule to remove the build products.
#
clean:
	@rm -f downloader.exe
	@rm -f makeboot.exe
	@rm -f flasher.rom
	@rm -f flasher.h
	@rm -f *.obj
	@rm -f *.o
	@rm -f *~
